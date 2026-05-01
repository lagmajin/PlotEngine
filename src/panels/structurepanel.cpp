#include "structurepanel.h"
#include <QHeaderView>
#include <QAction>

StructurePanel::StructurePanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_model = new QStandardItemModel(this);
    m_model->setHorizontalHeaderLabels({"構造"});

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
    m_model->clear();
    m_model->setHorizontalHeaderLabels({"構造"});

    for (const auto &ch : project.chapters) {
        auto *chItem = new QStandardItem(ch.title);
        chItem->setData(ch.id, Qt::UserRole + 1);
        chItem->setData("chapter", Qt::UserRole + 2);
        chItem->setEditable(true);
        chItem->setForeground(QColor("#cdd6f4"));

        for (const auto &sc : ch.scenes) {
            auto *scItem = new QStandardItem(sc.title);
            scItem->setData(sc.id, Qt::UserRole + 1);
            scItem->setData("scene", Qt::UserRole + 2);
            scItem->setData(ch.id, Qt::UserRole + 3);
            scItem->setForeground(QColor("#a6adc8"));
            chItem->appendRow(scItem);
        }
        m_model->appendRow(chItem);
    }

    m_tree->expandAll();
}

void StructurePanel::onTreeDoubleClick(const QModelIndex &index)
{
    QString type = index.data(Qt::UserRole + 2).toString();
    if (type != "scene") return;

    QString sceneId = index.data(Qt::UserRole + 1).toString();
    QString chapterId = index.data(Qt::UserRole + 3).toString();
    emit sceneSelected(chapterId, sceneId);
}

void StructurePanel::onCustomContextMenu(const QPoint &pos)
{
    QModelIndex index = m_tree->indexAt(pos);
    if (!index.isValid()) return;

    QString type = index.data(Qt::UserRole + 2).toString();
    if (type == "chapter") {
        QMenu menu;
        menu.addAction("シーン追加", this, [this, index]() {
            QString chId = index.data(Qt::UserRole + 1).toString();
            QStandardItem *chItem = m_model->itemFromIndex(index);
            auto *scItem = new QStandardItem("新シーン");
            scItem->setData(QUuid::createUuid().toString(QUuid::WithoutBraces), Qt::UserRole + 1);
            scItem->setData("scene", Qt::UserRole + 2);
            scItem->setData(chId, Qt::UserRole + 3);
            scItem->setForeground(QColor("#a6adc8"));
            chItem->appendRow(scItem);
            m_tree->expand(index);
        });
        menu.exec(m_tree->viewport()->mapToGlobal(pos));
    }
}