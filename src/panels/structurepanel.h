#pragma once

#include <QWidget>
#include <QVector>
#include <QString>
#include <QTreeView>
#include <QStandardItemModel>
#include <QStandardItem>
#include "core/novelproject.h"

class QTabWidget;

class StructurePanel : public QWidget {
    Q_OBJECT
public:
    struct OpenDocumentEntry {
        QString kind;
        QString id;
        QString title;
        QString detail;
        bool dirty = false;
    };

    explicit StructurePanel(QWidget *parent = nullptr);

    void loadProject(const NovelProject &project);
    void selectChapter(const QString &chapterId);
    void selectEpisode(const QString &chapterId, const QString &episodeId);
    void setOpenDocuments(const QVector<OpenDocumentEntry> &documents);
    void selectOpenDocument(const QString &kind, const QString &id);

signals:
    void episodeSelected(const QString &chapterId, const QString &episodeId);
    void episodeAddRequested(const QString &chapterId);
    void chapterRenamed(const QString &chapterId, const QString &title);
    void episodeRenamed(const QString &chapterId, const QString &episodeId, const QString &title);
    void openDocumentRequested(const QString &kind, const QString &id);

private slots:
    void onStructureDoubleClick(const QModelIndex &index);
    void onStructureContextMenu(const QPoint &pos);
    void onOpenDocumentsDoubleClick(const QModelIndex &index);
    void onItemChanged(QStandardItem *item);

private:
    void selectStructureIndex(const QModelIndex &index);

    QTabWidget *m_tabs = nullptr;
    QTreeView *m_structureTree = nullptr;
    QTreeView *m_openDocumentsTree = nullptr;
    QStandardItemModel *m_structureModel = nullptr;
    QStandardItemModel *m_openDocumentsModel = nullptr;
    NovelProject m_project;
};
