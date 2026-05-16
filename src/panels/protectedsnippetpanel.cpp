#include "protectedsnippetpanel.h"
#include "wobjectimpl.h"

#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

W_OBJECT_IMPL(ProtectedSnippetPanel)

DockPaneSpec ProtectedSnippetPanel::dockSpec()
{
    return { QStringLiteral("保護ブロック"), DockPlacement::Right, true };
}

ProtectedSnippetPanel::ProtectedSnippetPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    m_titleLabel = new QLabel(QStringLiteral("現在の文書: なし"), this);
    m_titleLabel->setWordWrap(true);
    layout->addWidget(m_titleLabel);

    m_list = new QListWidget(this);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_list, 1);

    m_detail = new QPlainTextEdit(this);
    m_detail->setReadOnly(true);
    m_detail->setPlaceholderText(QStringLiteral("保護ブロックの本文"));
    layout->addWidget(m_detail, 1);

    auto *buttons = new QHBoxLayout();
    m_addButton = new QPushButton(QStringLiteral("選択を追加"), this);
    m_removeButton = new QPushButton(QStringLiteral("削除"), this);
    m_clearButton = new QPushButton(QStringLiteral("全削除"), this);
    buttons->addWidget(m_addButton);
    buttons->addWidget(m_removeButton);
    buttons->addWidget(m_clearButton);
    layout->addLayout(buttons);

    connect(m_list, &QListWidget::itemSelectionChanged, this, &ProtectedSnippetPanel::onSelectionChanged);
    connect(m_addButton, &QPushButton::clicked, this, &ProtectedSnippetPanel::onAddClicked);
    connect(m_removeButton, &QPushButton::clicked, this, &ProtectedSnippetPanel::onRemoveClicked);
    connect(m_clearButton, &QPushButton::clicked, this, &ProtectedSnippetPanel::onClearClicked);

    clearPanel();
}

void ProtectedSnippetPanel::setDocumentTitle(const QString &title)
{
    m_titleLabel->setText(title.trimmed().isEmpty()
        ? QStringLiteral("現在の文書: なし")
        : QStringLiteral("現在の文書: %1").arg(title.trimmed()));
}

void ProtectedSnippetPanel::setSnippets(const QVector<Snippet> &snippets)
{
    m_snippets = snippets;
    m_list->clear();
    for (int i = 0; i < m_snippets.size(); ++i) {
        const auto &snippet = m_snippets.at(i);
        const QString label = snippet.label.trimmed().isEmpty()
            ? QStringLiteral("保護ブロック %1").arg(i + 1)
            : snippet.label.trimmed();
        auto *item = new QListWidgetItem(label, m_list);
        item->setData(Qt::UserRole, i);
        item->setToolTip(snippet.text.left(400));
        m_list->addItem(item);
    }
    if (m_list->count() > 0)
        m_list->setCurrentRow(0);
    refreshDetail();
    m_removeButton->setEnabled(m_list->count() > 0);
    m_clearButton->setEnabled(m_list->count() > 0);
}

void ProtectedSnippetPanel::clearPanel()
{
    m_snippets.clear();
    m_titleLabel->setText(QStringLiteral("現在の文書: なし"));
    m_list->clear();
    m_detail->clear();
    m_removeButton->setEnabled(false);
    m_clearButton->setEnabled(false);
}

void ProtectedSnippetPanel::onSelectionChanged()
{
    refreshDetail();
}

void ProtectedSnippetPanel::onAddClicked()
{
    emit addCurrentSelectionRequested();
}

void ProtectedSnippetPanel::onRemoveClicked()
{
    auto *item = m_list->currentItem();
    if (!item)
        return;
    emit removeSnippetRequested(item->data(Qt::UserRole).toInt());
}

void ProtectedSnippetPanel::onClearClicked()
{
    emit clearSnippetsRequested();
}

void ProtectedSnippetPanel::refreshDetail()
{
    auto *item = m_list->currentItem();
    if (!item) {
        m_detail->clear();
        return;
    }

    const int index = item->data(Qt::UserRole).toInt();
    if (index < 0 || index >= m_snippets.size()) {
        m_detail->clear();
        return;
    }

    m_detail->setPlainText(m_snippets.at(index).text);
}
