#include "reviewpanel.h"
#include "widgets/textcompareview.h"
#include "wobjectimpl.h"

#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

W_OBJECT_IMPL(ReviewPanel)

DockPaneSpec ReviewPanel::dockSpec()
{
    return { QStringLiteral("AI レビュー"), DockPlacement::Right, true };
}

ReviewPanel::ReviewPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    m_titleLabel = new QLabel(QStringLiteral("AI 編集レビュー"), this);
    m_titleLabel->setStyleSheet("QLabel { color: #ffffff; font-weight: 700; font-size: 15px; }");
    layout->addWidget(m_titleLabel);

    m_rationaleLabel = new QLabel(QStringLiteral("レビュー待ちの編集はありません。"), this);
    m_rationaleLabel->setWordWrap(true);
    m_rationaleLabel->setStyleSheet("QLabel { color: #cbd5e1; }");
    layout->addWidget(m_rationaleLabel);

    auto *contentTabs = new QTabWidget(this);
    contentTabs->setDocumentMode(true);

    auto *actionsPage = new QWidget(contentTabs);
    auto *actionsLayout = new QVBoxLayout(actionsPage);
    actionsLayout->setContentsMargins(0, 0, 0, 0);
    actionsLayout->setSpacing(6);

    m_actionList = new QListWidget(actionsPage);
    m_actionList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_actionList->setAlternatingRowColors(true);
    actionsLayout->addWidget(m_actionList, 2);

    m_actionDetail = new QPlainTextEdit(actionsPage);
    m_actionDetail->setReadOnly(true);
    m_actionDetail->setPlaceholderText(QStringLiteral("アクションを選ぶと詳細を表示します。"));
    actionsLayout->addWidget(m_actionDetail, 1);
    contentTabs->addTab(actionsPage, QStringLiteral("アクション"));

    m_diffSummary = new QPlainTextEdit(contentTabs);
    m_diffSummary->setReadOnly(true);
    m_diffSummary->setPlaceholderText(QStringLiteral("差分の概要"));
    contentTabs->addTab(m_diffSummary, QStringLiteral("差分"));

    m_compareView = new TextCompareView(contentTabs);
    contentTabs->addTab(m_compareView, QStringLiteral("比較"));

    m_rawResponse = new QPlainTextEdit(contentTabs);
    m_rawResponse->setReadOnly(true);
    m_rawResponse->setPlaceholderText(QStringLiteral("AI 応答"));
    contentTabs->addTab(m_rawResponse, QStringLiteral("AI 応答"));

    layout->addWidget(contentTabs, 1);

    auto *buttons = new QHBoxLayout();
    buttons->addStretch(1);
    m_discardButton = new QPushButton(QStringLiteral("破棄"), this);
    m_applyButton = new QPushButton(QStringLiteral("選択を適用"), this);
    buttons->addWidget(m_discardButton);
    buttons->addWidget(m_applyButton);
    layout->addLayout(buttons);

    connect(m_actionList, &QListWidget::itemSelectionChanged, this, &ReviewPanel::onActionSelectionChanged);
    connect(m_applyButton, &QPushButton::clicked, this, &ReviewPanel::onApplyClicked);
    connect(m_discardButton, &QPushButton::clicked, this, &ReviewPanel::onDiscardClicked);

    clearReview();
}

void ReviewPanel::clearReview()
{
    m_actions.clear();
    m_overallDiffSummary.clear();
    m_titleLabel->setText(QStringLiteral("AI 編集レビュー"));
    m_rationaleLabel->setText(QStringLiteral("レビュー待ちの編集はありません。"));
    m_actionList->clear();
    m_actionDetail->clear();
    m_diffSummary->clear();
    m_compareView->clear();
    m_rawResponse->clear();
    m_applyButton->setEnabled(false);
    m_discardButton->setEnabled(false);
}

void ReviewPanel::setReviewTitle(const QString &title)
{
    m_titleLabel->setText(title.trimmed().isEmpty() ? QStringLiteral("AI 編集レビュー") : title.trimmed());
}

void ReviewPanel::setRationale(const QString &rationale)
{
    m_rationaleLabel->setText(rationale.trimmed().isEmpty()
        ? QStringLiteral("AI が返した編集内容を確認してください。")
        : rationale.trimmed());
}

void ReviewPanel::setActions(const QVector<ActionItem> &actions)
{
    m_actions = actions;
    m_actionList->clear();

    for (int i = 0; i < m_actions.size(); ++i) {
        const auto &action = m_actions.at(i);
        auto *item = new QListWidgetItem(action.title);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(action.checked ? Qt::Checked : Qt::Unchecked);
        item->setData(Qt::UserRole, i);
        m_actionList->addItem(item);
    }

    if (m_actionList->count() > 0)
        m_actionList->setCurrentRow(0);

    m_applyButton->setEnabled(m_actionList->count() > 0);
    m_discardButton->setEnabled(m_actionList->count() > 0);
    refreshActionDetail();
}

void ReviewPanel::setDiffSummary(const QString &summary)
{
    m_overallDiffSummary = summary.trimmed();
    m_diffSummary->setPlainText(m_overallDiffSummary);
}

void ReviewPanel::setRawResponse(const QString &response)
{
    m_rawResponse->setPlainText(response.trimmed());
}

QVector<int> ReviewPanel::selectedActionIndices() const
{
    QVector<int> indices;
    for (int i = 0; i < m_actionList->count(); ++i) {
        auto *item = m_actionList->item(i);
        if (item && item->checkState() == Qt::Checked)
            indices.append(item->data(Qt::UserRole).toInt());
    }
    return indices;
}

QString ReviewPanel::rawResponse() const
{
    return m_rawResponse->toPlainText();
}

void ReviewPanel::onActionSelectionChanged()
{
    refreshActionDetail();
}

void ReviewPanel::onApplyClicked()
{
    emit applyRequested();
}

void ReviewPanel::onDiscardClicked()
{
    emit discardRequested();
}

void ReviewPanel::refreshActionDetail()
{
    auto *item = m_actionList->currentItem();
    if (!item) {
        m_actionDetail->clear();
        m_compareView->clear();
        return;
    }

    const int index = item->data(Qt::UserRole).toInt();
    if (index < 0 || index >= m_actions.size()) {
        m_actionDetail->clear();
        m_diffSummary->setPlainText(m_overallDiffSummary);
        m_compareView->clear();
        return;
    }

    const auto &action = m_actions.at(index);
    m_actionDetail->setPlainText(action.detail.trimmed());
    m_diffSummary->setPlainText(action.diffSummary.trimmed().isEmpty() ? m_overallDiffSummary : action.diffSummary.trimmed());
    m_compareView->setComparison(action.compareLeftTitle, action.compareLeftContent,
                                 action.compareRightTitle, action.compareRightContent);
}
