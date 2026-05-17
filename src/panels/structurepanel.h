#pragma once

#include <QWidget>
#include <QVector>
#include <QString>
#include <QTreeView>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QToolButton>
#include "wobjectdefs.h"

#include "ui/dockpane.h"
#include "core/novelproject.h"

class StructurePanel : public QWidget {
    W_OBJECT(StructurePanel)
public:
    struct OpenDocumentEntry {
        QString kind;
        QString id;
        QString title;
        QString detail;
        bool dirty = false;
    };

    explicit StructurePanel(QWidget *parent = nullptr);

    static DockPaneSpec dockSpec();

    void loadProject(const NovelProject &project);
    void selectChapter(const QString &chapterId);
    void selectEpisode(const QString &chapterId, const QString &episodeId);
    void setOpenDocuments(const QVector<OpenDocumentEntry> &documents);
    void setCurrentDocument(const OpenDocumentEntry &document);
    void selectOpenDocument(const QString &kind, const QString &id);

public:
    void episodeSelected(const QString &chapterId, const QString &episodeId)
    W_SIGNAL(episodeSelected, (const QString &, const QString &), chapterId, episodeId)
    void episodeAddRequested(const QString &chapterId)
    W_SIGNAL(episodeAddRequested, (const QString &), chapterId)
    void chapterRenamed(const QString &chapterId, const QString &title)
    W_SIGNAL(chapterRenamed, (const QString &, const QString &), chapterId, title)
    void episodeRenamed(const QString &chapterId, const QString &episodeId, const QString &title)
    W_SIGNAL(episodeRenamed, (const QString &, const QString &, const QString &), chapterId, episodeId, title)
    void openDocumentRequested(const QString &kind, const QString &id)
    W_SIGNAL(openDocumentRequested, (const QString &, const QString &), kind, id)
    void openDocumentCloseRequested(const QString &kind, const QString &id)
    W_SIGNAL(openDocumentCloseRequested, (const QString &, const QString &), kind, id)
    void openOtherDocumentsRequested(const QString &kind, const QString &id)
    W_SIGNAL(openOtherDocumentsRequested, (const QString &, const QString &), kind, id)

private:
    void onStructureDoubleClick(const QModelIndex &index);
    W_SLOT(onStructureDoubleClick, (const QModelIndex &))
    void onStructureContextMenu(const QPoint &pos);
    W_SLOT(onStructureContextMenu, (const QPoint &))
    void onOpenDocumentsDoubleClick(const QModelIndex &index);
    W_SLOT(onOpenDocumentsDoubleClick, (const QModelIndex &))
    void onOpenDocumentsContextMenu(const QPoint &pos);
    W_SLOT(onOpenDocumentsContextMenu, (const QPoint &))
    void onItemChanged(QStandardItem *item);
    W_SLOT(onItemChanged, (QStandardItem *))

private:
    void selectStructureIndex(const QModelIndex &index);
    QWidget *createSection(const QString &title, QWidget *content, QToolButton **headerButton = nullptr, QWidget **frameWidget = nullptr);
    void setSectionExpanded(QToolButton *button, QWidget *content, bool expanded);
    void loadSectionState();
    void saveSectionState(const QString &key, bool expanded) const;

    QToolButton *m_structureHeader = nullptr;
    QToolButton *m_openDocumentsHeader = nullptr;
    QToolButton *m_currentDocumentHeader = nullptr;
    QWidget *m_structureSection = nullptr;
    QWidget *m_structureFrame = nullptr;
    QWidget *m_openDocumentsSection = nullptr;
    QWidget *m_openDocumentsFrame = nullptr;
    QWidget *m_currentDocumentSection = nullptr;
    QWidget *m_currentDocumentFrame = nullptr;
    QTreeView *m_structureTree = nullptr;
    QTreeView *m_openDocumentsTree = nullptr;
    QTreeView *m_currentDocumentTree = nullptr;
    QStandardItemModel *m_structureModel = nullptr;
    QStandardItemModel *m_openDocumentsModel = nullptr;
    QStandardItemModel *m_currentDocumentModel = nullptr;
    NovelProject m_project;
};
