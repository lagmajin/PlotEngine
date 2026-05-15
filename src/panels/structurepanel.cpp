#include "structurepanel.h"
import PlotEngine.Core.NovelProject;
#include <QHeaderView>
#include <QAction>
#include <QVBoxLayout>
#include <QAbstractItemView>
#include <QColor>
#include <QFont>
#include <QSignalBlocker>
#include <QMenu>
#include <QIcon>
#include <QSizePolicy>
#include <QApplication>
#include <QPainter>
#include <QPixmap>
#include <QRectF>
#include <QPointF>
#include <QToolButton>
#include <QFrame>

namespace {
QIcon documentIcon(const QString &kind, bool dirty = false)
{
    QPixmap pix(16, 16);
    pix.fill(Qt::transparent);

    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);

    if (kind == "chapter") {
        p.setBrush(QColor("#4f8cff"));
        p.drawRoundedRect(QRectF(1.5, 3.5, 12.5, 10.0), 2, 2);
        p.setBrush(QColor("#7cc0ff"));
        p.drawRoundedRect(QRectF(3.0, 1.5, 5.5, 3.5), 1, 1);
    } else if (kind == "episode") {
        p.setBrush(QColor(dirty ? "#ff6b6b" : "#cdd6f4"));
        p.drawRoundedRect(QRectF(3, 1.5, 9, 13), 2, 2);
        p.setBrush(QColor("#1e1e2e"));
        p.drawRect(QRectF(5, 4, 5, 1));
        p.drawRect(QRectF(5, 7, 4, 1));
        p.drawRect(QRectF(5, 10, 3, 1));
    } else if (kind == "character") {
        p.setBrush(QColor("#ffb86c"));
        p.drawEllipse(QPointF(8, 5), 3, 3);
        p.setBrush(QColor("#ffb86c"));
        p.drawRoundedRect(QRectF(4.5, 8, 7, 5), 2, 2);
    } else if (kind == "location") {
        p.setBrush(QColor("#50fa7b"));
        p.drawEllipse(QPointF(8, 6), 4, 4);
        p.drawRoundedRect(QRectF(7, 7, 2, 6), 1, 1);
        p.setBrush(QColor("#1e1e2e"));
        p.drawEllipse(QPointF(8, 6), 1.7, 1.7);
    } else {
        p.setBrush(QColor("#94a3b8"));
        p.drawRoundedRect(QRectF(2.5, 2.5, 11, 11), 2, 2);
    }

    if (dirty) {
        p.setBrush(QColor("#ff6b6b"));
        p.drawEllipse(QPointF(13.5, 2.5), 1.8, 1.8);
    }

    return QIcon(pix);
}

void styleExplorerTree(QTreeView *tree, int paddingLeft, const QString &selectionColor)
{
    tree->setStyleSheet(QStringLiteral(
        "QTreeView { background: #1e1e1e; border: none; outline: none; }"
        "QTreeView::item { padding: 5px 6px; margin-left: %1px; }"
        "QTreeView::item:selected { background: %2; color: #ffffff; }"
        "QTreeView::item:hover { background: #2a2d2e; }"
    ).arg(paddingLeft).arg(selectionColor));
}
}

StructurePanel::StructurePanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    auto *structurePage = new QWidget(this);
    auto *structureLayout = new QVBoxLayout(structurePage);
    structureLayout->setContentsMargins(0, 0, 0, 0);

    m_structureModel = new QStandardItemModel(this);
    m_structureModel->setHorizontalHeaderLabels({"構造"});
    connect(m_structureModel, &QStandardItemModel::itemChanged,
            this, &StructurePanel::onItemChanged);

    m_structureTree = new QTreeView(structurePage);
    m_structureTree->setModel(m_structureModel);
    m_structureTree->setHeaderHidden(true);
    m_structureTree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_structureTree->setAnimated(true);
    m_structureTree->setEditTriggers(QAbstractItemView::DoubleClicked);
    m_structureTree->setRootIsDecorated(true);
    m_structureTree->setIndentation(14);
    m_structureTree->setAlternatingRowColors(true);
    m_structureTree->setUniformRowHeights(true);
    styleExplorerTree(m_structureTree, 0, "#094771");
    m_structureTree->header()->setStretchLastSection(true);
    structureLayout->addWidget(m_structureTree);

    m_structureSection = createSection("構造", structurePage, &m_structureHeader);
    layout->addWidget(m_structureSection);

    auto *openPage = new QWidget(this);
    auto *openLayout = new QVBoxLayout(openPage);
    openLayout->setContentsMargins(0, 0, 0, 0);

    m_openDocumentsModel = new QStandardItemModel(this);
    m_openDocumentsModel->setHorizontalHeaderLabels({"開いているエディタ"});

    m_openDocumentsTree = new QTreeView(openPage);
    m_openDocumentsTree->setModel(m_openDocumentsModel);
    m_openDocumentsTree->setHeaderHidden(true);
    m_openDocumentsTree->setAnimated(true);
    m_openDocumentsTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_openDocumentsTree->setRootIsDecorated(false);
    m_openDocumentsTree->setIndentation(0);
    m_openDocumentsTree->setAlternatingRowColors(true);
    m_openDocumentsTree->setUniformRowHeights(true);
    styleExplorerTree(m_openDocumentsTree, 0, "#094771");
    m_openDocumentsTree->header()->setStretchLastSection(true);
    openLayout->addWidget(m_openDocumentsTree);

    m_openDocumentsSection = createSection("開いているエディタ", openPage, &m_openDocumentsHeader);
    layout->addWidget(m_openDocumentsSection);

    auto *currentPage = new QWidget(this);
    auto *currentLayout = new QVBoxLayout(currentPage);
    currentLayout->setContentsMargins(0, 0, 0, 0);

    m_currentDocumentModel = new QStandardItemModel(this);
    m_currentDocumentModel->setHorizontalHeaderLabels({"現在のファイル"});

    m_currentDocumentTree = new QTreeView(currentPage);
    m_currentDocumentTree->setModel(m_currentDocumentModel);
    m_currentDocumentTree->setHeaderHidden(true);
    m_currentDocumentTree->setAnimated(true);
    m_currentDocumentTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_currentDocumentTree->setRootIsDecorated(false);
    m_currentDocumentTree->setIndentation(0);
    m_currentDocumentTree->setAlternatingRowColors(true);
    m_currentDocumentTree->setUniformRowHeights(true);
    styleExplorerTree(m_currentDocumentTree, 0, "#1f3b52");
    m_currentDocumentTree->header()->setStretchLastSection(true);
    currentLayout->addWidget(m_currentDocumentTree);

    m_currentDocumentSection = createSection("現在のファイル", currentPage, &m_currentDocumentHeader);
    layout->addWidget(m_currentDocumentSection);
    layout->addStretch(1);

    connect(m_structureTree, &QTreeView::doubleClicked, this, &StructurePanel::onStructureDoubleClick);
    connect(m_structureTree, &QTreeView::customContextMenuRequested, this, &StructurePanel::onStructureContextMenu);
    connect(m_openDocumentsTree, &QTreeView::doubleClicked, this, &StructurePanel::onOpenDocumentsDoubleClick);
    connect(m_currentDocumentTree, &QTreeView::doubleClicked, this, &StructurePanel::onOpenDocumentsDoubleClick);
    connect(m_currentDocumentTree, &QTreeView::clicked, this, &StructurePanel::onOpenDocumentsDoubleClick);
}

QWidget *StructurePanel::createSection(const QString &title, QWidget *content, QToolButton **headerButton)
{
    auto *container = new QWidget(this);
    auto *containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);

    auto *button = new QToolButton(container);
    button->setText(title);
    button->setCheckable(true);
    button->setChecked(true);
    button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    button->setArrowType(Qt::DownArrow);
    button->setIcon(documentIcon(title == "構造" ? "chapter" : title == "開いているエディタ" ? "episode" : "character"));
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    button->setStyleSheet(R"(
        QToolButton {
            background: #252526;
            color: #e5e7eb;
            border: 1px solid #3c3c3c;
            padding: 8px 10px;
            text-align: left;
            font-weight: 600;
        }
        QToolButton:hover {
            background: #2a2d2e;
        }
        QToolButton:checked {
            background: #2d2d2d;
        }
    )");

    auto *frame = new QFrame(container);
    frame->setFrameShape(QFrame::StyledPanel);
    frame->setFrameShadow(QFrame::Plain);
    frame->setStyleSheet("QFrame { border: 1px solid #333333; background: #1e1e1e; }");
    auto *frameLayout = new QVBoxLayout(frame);
    frameLayout->setContentsMargins(0, 0, 0, 0);
    frameLayout->setSpacing(0);
    frameLayout->addWidget(content);

    containerLayout->addWidget(button);
    containerLayout->addWidget(frame);

    connect(button, &QToolButton::toggled, this, [frame, button](bool checked) {
        frame->setVisible(checked);
        button->setArrowType(checked ? Qt::DownArrow : Qt::RightArrow);
    });

    if (headerButton)
        *headerButton = button;

    return container;
}

void StructurePanel::setSectionExpanded(QToolButton *button, QWidget *content, bool expanded)
{
    if (!button || !content)
        return;
    button->setChecked(expanded);
    content->setVisible(expanded);
}

void StructurePanel::loadProject(const NovelProject &project)
{
    m_project = project;
    QSignalBlocker blocker(m_structureModel);
    m_structureModel->clear();
    m_structureModel->setHorizontalHeaderLabels({"構造"});

    for (const auto &ch : project.chapters) {
        auto *chItem = new QStandardItem(ch.title);
        chItem->setData(ch.id, Qt::UserRole + 1);
        chItem->setData("chapter", Qt::UserRole + 2);
        chItem->setEditable(true);
        chItem->setForeground(QColor("#cdd6f4"));
        chItem->setIcon(documentIcon("chapter"));

        for (const auto &ep : ch.episodes) {
            auto *epItem = new QStandardItem(ep.title);
            epItem->setData(ep.id, Qt::UserRole + 1);
            epItem->setData("episode", Qt::UserRole + 2);
            epItem->setData(ch.id, Qt::UserRole + 3);
            epItem->setForeground(QColor("#a6adc8"));
            epItem->setIcon(documentIcon("episode"));
            chItem->appendRow(epItem);
        }
        m_structureModel->appendRow(chItem);
    }

    m_structureTree->expandAll();
}

void StructurePanel::selectChapter(const QString &chapterId)
{
    for (int i = 0; i < m_structureModel->rowCount(); ++i) {
        auto *item = m_structureModel->item(i);
        if (!item) continue;
        if (item->data(Qt::UserRole + 1).toString() != chapterId) continue;

        QModelIndex index = item->index();
        selectStructureIndex(index);
        return;
    }
}

void StructurePanel::selectEpisode(const QString &chapterId, const QString &episodeId)
{
    for (int i = 0; i < m_structureModel->rowCount(); ++i) {
        auto *chapterItem = m_structureModel->item(i);
        if (!chapterItem) continue;
        if (chapterItem->data(Qt::UserRole + 1).toString() != chapterId) continue;

        QModelIndex chapterIndex = chapterItem->index();
        m_structureTree->expand(chapterIndex);
        for (int j = 0; j < chapterItem->rowCount(); ++j) {
            auto *episodeItem = chapterItem->child(j);
            if (!episodeItem) continue;
            if (episodeItem->data(Qt::UserRole + 1).toString() != episodeId) continue;

            QModelIndex episodeIndex = episodeItem->index();
            selectStructureIndex(episodeIndex);
            return;
        }
    }
}

void StructurePanel::setOpenDocuments(const QVector<OpenDocumentEntry> &documents)
{
    QSignalBlocker blocker(m_openDocumentsModel);
    m_openDocumentsModel->clear();
    m_openDocumentsModel->setHorizontalHeaderLabels({"開いているエディタ"});

    for (const auto &doc : documents) {
        auto *item = new QStandardItem(doc.title);
        item->setData(doc.kind, Qt::UserRole + 1);
        item->setData(doc.id, Qt::UserRole + 2);
        item->setData(doc.detail, Qt::UserRole + 3);
        item->setEditable(false);
        item->setIcon(documentIcon(doc.kind, doc.dirty));
        QFont font = item->font();
        font.setBold(doc.dirty);
        item->setFont(font);
        item->setForeground(doc.dirty ? QColor("#ff8fa3") : QColor("#cdd6f4"));
        if (doc.dirty)
            item->setBackground(QColor("#3a1f25"));
        if (!doc.detail.isEmpty())
            item->setToolTip(doc.detail + (doc.dirty ? " - 未保存" : ""));
        else if (doc.dirty)
            item->setToolTip("未保存");
        m_openDocumentsModel->appendRow(item);
    }

    m_openDocumentsTree->expandAll();
}

void StructurePanel::setCurrentDocument(const OpenDocumentEntry &document)
{
    QSignalBlocker blocker(m_currentDocumentModel);
    m_currentDocumentModel->clear();
    m_currentDocumentModel->setHorizontalHeaderLabels({"現在のファイル"});

    if (document.id.isEmpty()) {
        auto *item = new QStandardItem("現在のファイルはありません");
        item->setEditable(false);
        item->setForeground(QColor("#6b7280"));
        item->setBackground(QColor("#1e1e1e"));
        m_currentDocumentModel->appendRow(item);
        return;
    }

    auto *item = new QStandardItem(document.title);
    item->setData(document.kind, Qt::UserRole + 1);
    item->setData(document.id, Qt::UserRole + 2);
    item->setData(document.detail, Qt::UserRole + 3);
    item->setEditable(false);
    item->setIcon(documentIcon(document.kind, document.dirty));
    QFont font = item->font();
    font.setBold(true);
    item->setFont(font);
    item->setForeground(document.dirty ? QColor("#ff8fa3") : QColor("#ffffff"));
    item->setBackground(document.dirty ? QColor("#3a1f25") : QColor("#1f2937"));
    if (!document.detail.isEmpty())
        item->setToolTip(document.detail + (document.dirty ? " - 未保存" : ""));
    else if (document.dirty)
        item->setToolTip("未保存");
    m_currentDocumentModel->appendRow(item);
    m_currentDocumentTree->expandAll();
    m_currentDocumentTree->setCurrentIndex(item->index());
    m_currentDocumentTree->scrollTo(item->index());
}

void StructurePanel::selectOpenDocument(const QString &kind, const QString &id)
{
    for (int i = 0; i < m_openDocumentsModel->rowCount(); ++i) {
        auto *item = m_openDocumentsModel->item(i);
        if (!item) continue;
        if (item->data(Qt::UserRole + 1).toString() != kind) continue;
        if (item->data(Qt::UserRole + 2).toString() != id) continue;

        const QModelIndex index = item->index();
        m_openDocumentsTree->setCurrentIndex(index);
        m_openDocumentsTree->scrollTo(index);
        return;
    }
}

void StructurePanel::onItemChanged(QStandardItem *item)
{
    if (!item) return;

    QString type = item->data(Qt::UserRole + 2).toString();
    if (type == "chapter") {
        emit chapterRenamed(item->data(Qt::UserRole + 1).toString(), item->text().trimmed());
        return;
    }

    if (type == "episode") {
        emit episodeRenamed(item->data(Qt::UserRole + 3).toString(),
                            item->data(Qt::UserRole + 1).toString(),
                            item->text().trimmed());
    }
}

void StructurePanel::selectStructureIndex(const QModelIndex &index)
{
    if (!index.isValid()) return;
    if (index.parent().isValid())
        m_structureTree->expand(index.parent());
    else
        m_structureTree->expand(index);
    m_structureTree->setCurrentIndex(index);
    m_structureTree->scrollTo(index);
}

void StructurePanel::onStructureDoubleClick(const QModelIndex &index)
{
    QString type = index.data(Qt::UserRole + 2).toString();
    if (type != "episode") return;

    QString episodeId = index.data(Qt::UserRole + 1).toString();
    QString chapterId = index.data(Qt::UserRole + 3).toString();
    emit episodeSelected(chapterId, episodeId);
}

void StructurePanel::onStructureContextMenu(const QPoint &pos)
{
    QModelIndex index = m_structureTree->indexAt(pos);
    if (!index.isValid()) return;

    QString type = index.data(Qt::UserRole + 2).toString();
    if (type == "chapter") {
        QMenu menu;
        menu.addAction("エピソード追加", this, [this, index]() {
            QString chId = index.data(Qt::UserRole + 1).toString();
            emit episodeAddRequested(chId);
        });
        menu.exec(m_structureTree->viewport()->mapToGlobal(pos));
    }
}

void StructurePanel::onOpenDocumentsDoubleClick(const QModelIndex &index)
{
    if (!index.isValid()) return;
    emit openDocumentRequested(index.data(Qt::UserRole + 1).toString(),
                               index.data(Qt::UserRole + 2).toString());
}
