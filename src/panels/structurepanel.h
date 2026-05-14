#pragma once

#include <QTreeView>
#include <QStandardItemModel>
#include <QMenu>
#include <QStandardItem>
#include "core/novelproject.h"

class StructurePanel : public QWidget {
    Q_OBJECT
public:
    explicit StructurePanel(QWidget *parent = nullptr);

    void loadProject(const NovelProject &project);

signals:
    void episodeSelected(const QString &chapterId, const QString &episodeId);
    void episodeAddRequested(const QString &chapterId);
    void chapterRenamed(const QString &chapterId, const QString &title);
    void episodeRenamed(const QString &chapterId, const QString &episodeId, const QString &title);

private slots:
    void onTreeDoubleClick(const QModelIndex &index);
    void onCustomContextMenu(const QPoint &pos);
    void onItemChanged(QStandardItem *item);

private:
    QTreeView *m_tree = nullptr;
    QStandardItemModel *m_model = nullptr;
    NovelProject m_project;
};
