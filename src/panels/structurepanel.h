#pragma once

#include <QWidget>
#include <QVector>
#include <QString>
#include <QTreeView>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QToolButton>
#include "core/novelproject.h"

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
    void setCurrentDocument(const OpenDocumentEntry &document);
    void selectOpenDocument(const QString &kind, const QString &id);

signals:
    void episodeSelected(const QString &chapterId, const QString &episodeId);
    void episodeAddRequested(const QString &chapterId);
    void chapterRenamed(const QString &chapterId, const QString &title);
    void episodeRenamed(const QString &chapterId, const QString &episodeId, const QString &title);
    void openDocumentRequested(const QString &kind, const QString &id);
    void openDocumentCloseRequested(const QString &kind, const QString &id);
    void openOtherDocumentsRequested(const QString &kind, const QString &id);

private slots:
    void onStructureDoubleClick(const QModelIndex &index);
    void onStructureContextMenu(const QPoint &pos);
    void onOpenDocumentsDoubleClick(const QModelIndex &index);
    void onOpenDocumentsContextMenu(const QPoint &pos);
    void onItemChanged(QStandardItem *item);

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
