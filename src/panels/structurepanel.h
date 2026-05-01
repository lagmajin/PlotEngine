#pragma once

#include <QTreeView>
#include <QStandardItemModel>
#include <QMenu>
#include "core/novelproject.h"

class StructurePanel : public QWidget {
    Q_OBJECT
public:
    explicit StructurePanel(QWidget *parent = nullptr);

    void loadProject(const NovelProject &project);

signals:
    void sceneSelected(const QString &chapterId, const QString &sceneId);

private slots:
    void onTreeDoubleClick(const QModelIndex &index);
    void onCustomContextMenu(const QPoint &pos);

private:
    QTreeView *m_tree = nullptr;
    QStandardItemModel *m_model = nullptr;
    NovelProject m_project;
};