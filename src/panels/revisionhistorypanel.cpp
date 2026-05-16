#include "revisionhistorypanel.h"
#include "widgets/textcompareview.h"
#include "wobjectimpl.h"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QTreeWidget>
#include <QVBoxLayout>

W_OBJECT_IMPL(RevisionHistoryPanel)

DockPaneSpec RevisionHistoryPanel::dockSpec()
{
    return { QStringLiteral("リビジョン履歴"), DockPlacement::Right, true };
}

RevisionHistoryPanel::RevisionHistoryPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    m_titleLabel = new QLabel(QStringLiteral("リビジョン履歴"), this);
    m_titleLabel->setStyleSheet("QLabel { color: #ffffff; font-weight: 700; font-size: 15px; }");
    layout->addWidget(m_titleLabel);

    m_tree = new QTreeWidget(this);
    m_tree->setHeaderLabels({QStringLiteral("時点"), QStringLiteral("種別")});
    m_tree->setRootIsDecorated(false);
    m_tree->setAlternatingRowColors(true);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tree->header()->setStretchLastSection(false);
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    layout->addWidget(m_tree, 1);

    m_contentTabs = new QTabWidget(this);
    m_contentTabs->setDocumentMode(true);

    m_detail = new QPlainTextEdit(m_contentTabs);
    m_detail->setReadOnly(true);
    m_detail->setPlaceholderText(QStringLiteral("履歴の詳細"));
    m_contentTabs->addTab(m_detail, QStringLiteral("本文"));

    m_diff = new QPlainTextEdit(m_contentTabs);
    m_diff->setReadOnly(true);
    m_diff->setPlaceholderText(QStringLiteral("現在との差分"));
    m_contentTabs->addTab(m_diff, QStringLiteral("差分"));

    m_compareView = new TextCompareView(m_contentTabs);
    m_contentTabs->addTab(m_compareView, QStringLiteral("比較"));
    layout->addWidget(m_contentTabs, 1);

    m_restoreButton = new QPushButton(QStringLiteral("この履歴に戻す"), this);
    layout->addWidget(m_restoreButton);

    connect(m_tree, &QTreeWidget::itemSelectionChanged, this, &RevisionHistoryPanel::onSelectionChanged);
    connect(m_restoreButton, &QPushButton::clicked, this, &RevisionHistoryPanel::onRestoreClicked);

    setEmptyState(QStringLiteral("エピソードを開くと履歴を表示します。"));
}

void RevisionHistoryPanel::setEmptyState(const QString &message)
{
    m_entries.clear();
    m_tree->clear();
    auto *item = new QTreeWidgetItem({message, QString()});
    item->setData(0, Qt::UserRole, QString());
    item->setDisabled(true);
    m_tree->addTopLevelItem(item);
    m_detail->clear();
    m_diff->clear();
    m_compareView->clear();
    m_contentTabs->setTabEnabled(1, false);
    m_contentTabs->setTabEnabled(2, false);
    m_contentTabs->setCurrentIndex(0);
    m_restoreButton->setEnabled(false);
}

void RevisionHistoryPanel::setEntries(const QVector<Entry> &entries)
{
    m_entries = entries;
    m_tree->clear();

    for (int i = 0; i < m_entries.size(); ++i) {
        const auto &entry = m_entries.at(i);
        auto *item = new QTreeWidgetItem({entry.title, entry.current ? QStringLiteral("現在") : entry.detail});
        item->setData(0, Qt::UserRole, entry.id);
        if (entry.current) {
            QFont font = item->font(0);
            font.setBold(true);
            item->setFont(0, font);
            item->setFont(1, font);
        }
        m_tree->addTopLevelItem(item);
    }

    if (m_tree->topLevelItemCount() > 0)
        m_tree->setCurrentItem(m_tree->topLevelItem(0));
    refreshDetail();
}

QString RevisionHistoryPanel::selectedRevisionId() const
{
    if (auto *item = m_tree->currentItem())
        return item->data(0, Qt::UserRole).toString();
    return QString();
}

void RevisionHistoryPanel::onSelectionChanged()
{
    refreshDetail();
}

void RevisionHistoryPanel::onRestoreClicked()
{
    const QString revisionId = selectedRevisionId();
    if (!revisionId.isEmpty())
        emit restoreRequested(revisionId);
}

void RevisionHistoryPanel::refreshDetail()
{
    auto *item = m_tree->currentItem();
    if (!item) {
        m_detail->clear();
        m_diff->clear();
        m_restoreButton->setEnabled(false);
        return;
    }

    const QString revisionId = item->data(0, Qt::UserRole).toString();
    for (const auto &entry : m_entries) {
        if (entry.id != revisionId && !(entry.current && revisionId.isEmpty()))
            continue;
        m_detail->setPlainText(entry.content.trimmed());
        m_diff->setPlainText(entry.diffSummary.trimmed());
        m_compareView->setComparison(entry.compareLeftTitle, entry.compareLeftContent,
                                     entry.compareRightTitle, entry.compareRightContent);
        m_restoreButton->setEnabled(!entry.current && !entry.id.isEmpty());
        m_contentTabs->setTabEnabled(1, !entry.diffSummary.trimmed().isEmpty());
        const bool hasCompare = !entry.compareLeftContent.isEmpty() || !entry.compareRightContent.isEmpty();
        m_contentTabs->setTabEnabled(2, hasCompare);
        if (entry.diffSummary.trimmed().isEmpty() && m_contentTabs->currentIndex() == 1)
            m_contentTabs->setCurrentIndex(0);
        if (!hasCompare && m_contentTabs->currentIndex() == 2)
            m_contentTabs->setCurrentIndex(0);
        return;
    }

    m_detail->clear();
    m_diff->clear();
    m_compareView->clear();
    m_contentTabs->setTabEnabled(1, false);
    m_contentTabs->setTabEnabled(2, false);
    m_restoreButton->setEnabled(false);
}
