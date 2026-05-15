#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <QLabel>
#include <QStatusBar>
#include <QMenuBar>
#include <QToolBar>
#include <QInputDialog>
#include <QLineEdit>
#include "core/novelproject.h"
#include "editor/noveleditor.h"
#include "panels/structurepanel.h"
#include "panels/notepanel.h"

namespace ads {
class CDockManager;
class CDockWidget;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void newProject();
    void openProject();
    bool saveProject();
    bool saveProjectAs();
    void onEpisodeSelected(const QString &chapterId, const QString &episodeId);
    void onCharacterSelected(const QString &characterId);
    void onLocationSelected(const QString &locationId);
    void updateStatusBar();
    void updateCursorStatus();
    void addChapter();
    void addEpisode(const QString &chapterId);
    void renameChapter(const QString &chapterId, const QString &title);
    void renameEpisode(const QString &chapterId, const QString &episodeId, const QString &title);
    void addCharacter();
    void addLocation();
    void polishCurrentEpisode();
    void quickOpen();
    void commandPalette();

private:
    void setupMenuBar();
    void setupToolBar();
    void setupDockManager();
    void setupDockWidgets();
    void setupStatusBar();
    void setupEditorTabs();
    void connectSignals();
    void setCurrentProject(const NovelProject &project);
    void applyStyleSheet();
    bool confirmSaveChanges();
    bool maybeSave();
    void updateTabDecorations();
    void refreshExplorerOpenDocuments();
    void closeOpenDocument(const QString &kind, const QString &id);
    void closeOtherOpenDocuments(const QString &kind, const QString &id);
    void markAllTabsDirty();
    void markAllTabsClean();
    void refreshTabTitle(QWidget *widget);
    QString baseTabTitle(QWidget *widget) const;
    QString currentBreadcrumbText() const;
    void updateBreadcrumbStatus();
    void openQuickOpenResult(const QString &kind, const QString &id);

    NovelProject m_project;
    bool m_dirty = false;

    ads::CDockManager *m_dockManager = nullptr;
    QTabWidget *m_editorTabs = nullptr;
    StructurePanel *m_structurePanel = nullptr;
    NotePanel *m_notePanel = nullptr;
    ads::CDockWidget *m_structureDock = nullptr;
    ads::CDockWidget *m_noteDock = nullptr;
    ads::CDockWidget *m_editorDock = nullptr;
    QLabel *m_statusFile = nullptr;
    QLabel *m_statusWords = nullptr;
    QLabel *m_statusCursor = nullptr;
    QLabel *m_statusBreadcrumb = nullptr;
};
