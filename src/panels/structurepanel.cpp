#include "structurepanel.h"
import PlotEngine.Core.NovelProject;
#include <QHeaderView>
#include <QAction>
#include <QVBoxLayout>
#include <QColor>
#include <QSignalBlocker>

StructurePanel::StructurePanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_model = new QStandardItemModel(this);
    m_model->setHorizontalHeaderLabels({"構造"});
    connect(m_model, &QStandardItemModel::itemChanged,
            this, &StructurePanel::onItemChanged);

    m_tree = new QTreeView(this);
    m_tree->setModel(m_model);
    m_tree->setHeaderHidden(true);
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_tree->setAnimated(true);
    m_tree->setEditTriggers(QAbstractItemView::DoubleClicked);
    m_tree->header()->setStretchLastSection(true);
    layout->addWidget(m_tree);

    connect(m_tree, &QTreeView::doubleClicked, this, &StructurePanel::onTreeDoubleClick);
    connect(m_tree, &QTreeView::customContextMenuRequested, this, &StructurePanel::onCustomContextMenu);
}

void StructurePanel::loadProject(const NovelProject &project)
{
    m_project = project;
    QSignalBlocker blocker(m_model);
    m_model->clear();
    m_model->setHorizontalHeaderLabels({"構造"});

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
        m_model->appendRow(chItem);
    }

    m_tree->expandAll();
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

void StructurePanel::onTreeDoubleClick(const QModelIndex &index)
{
    QString type = index.data(Qt::UserRole + 2).toString();
    if (type != "episode") return;

    QString episodeId = index.data(Qt::UserRole + 1).toString();
    QString chapterId = index.data(Qt::UserRole + 3).toString();
    emit episodeSelected(chapterId, episodeId);
}

void StructurePanel::onCustomContextMenu(const QPoint &pos)
{
    QModelIndex index = m_tree->indexAt(pos);
    if (!index.isValid()) return;

    QString type = index.data(Qt::UserRole + 2).toString();
    if (type == "chapter") {
        QMenu menu;
        menu.addAction("エピソード追加", this, [this, index]() {
            QString chId = index.data(Qt::UserRole + 1).toString();
            emit episodeAddRequested(chId);
        });
        menu.exec(m_tree->viewport()->mapToGlobal(pos));
    }
}
