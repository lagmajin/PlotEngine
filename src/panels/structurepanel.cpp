#include "structurepanel.h"
import PlotEngine.Core.NovelProject;
#include <QHeaderView>
#include <QAction>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QAbstractItemView>
#include <QColor>
#include <QSignalBlocker>
#include <QMenu>

StructurePanel::StructurePanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_tabs = new QTabWidget(this);
    m_tabs->setDocumentMode(true);
    m_tabs->setMovable(false);
    layout->addWidget(m_tabs);

    auto *structurePage = new QWidget(m_tabs);
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
    m_structureTree->header()->setStretchLastSection(true);
    structureLayout->addWidget(m_structureTree);

    m_tabs->addTab(structurePage, "構造");

    auto *openPage = new QWidget(m_tabs);
    auto *openLayout = new QVBoxLayout(openPage);
    openLayout->setContentsMargins(0, 0, 0, 0);

    m_openDocumentsModel = new QStandardItemModel(this);
    m_openDocumentsModel->setHorizontalHeaderLabels({"開いているエディタ"});

    m_openDocumentsTree = new QTreeView(openPage);
    m_openDocumentsTree->setModel(m_openDocumentsModel);
    m_openDocumentsTree->setHeaderHidden(true);
    m_openDocumentsTree->setAnimated(true);
    m_openDocumentsTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_openDocumentsTree->header()->setStretchLastSection(true);
    openLayout->addWidget(m_openDocumentsTree);

    m_tabs->addTab(openPage, "開いているエディタ");

    connect(m_structureTree, &QTreeView::doubleClicked, this, &StructurePanel::onStructureDoubleClick);
    connect(m_structureTree, &QTreeView::customContextMenuRequested, this, &StructurePanel::onStructureContextMenu);
    connect(m_openDocumentsTree, &QTreeView::doubleClicked, this, &StructurePanel::onOpenDocumentsDoubleClick);
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

        for (const auto &ep : ch.episodes) {
            auto *epItem = new QStandardItem(ep.title);
            epItem->setData(ep.id, Qt::UserRole + 1);
            epItem->setData("episode", Qt::UserRole + 2);
            epItem->setData(ch.id, Qt::UserRole + 3);
            epItem->setForeground(QColor("#a6adc8"));
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
        item->setForeground(doc.dirty ? QColor("#f38ba8") : QColor("#cdd6f4"));
        if (!doc.detail.isEmpty())
            item->setToolTip(doc.detail + (doc.dirty ? " (未保存)" : ""));
        else if (doc.dirty)
            item->setToolTip("未保存");
        m_openDocumentsModel->appendRow(item);
    }

    m_openDocumentsTree->expandAll();
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
