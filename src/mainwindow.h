#pragma once

#include <QByteArray>
#include <QCloseEvent>
#include <QLabel>
#include <QMainWindow>
#include <QMenuBar>
#include <QPushButton>
#include <QHash>
#include <QSettings>
#include <QStringList>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolBar>
#include <QVector>

#include "wobjectdefs.h"

class NovelProject;

namespace PlotEngine::AI {
struct EditPlan;
}

namespace ads {
class CDockManager;
class CDockWidget;
}

class NovelEditor;
class StructurePanel;
class NotePanel;
class SearchPanel;
class ReviewPanel;
class RevisionHistoryPanel;
class ProtectedSnippetPanel;
class SceneBoardPanel;

class MainWindow : public QMainWindow {
    W_OBJECT(MainWindow)
public:
    struct ProtectedSnippetRecord {
        QString label;
        QString text;
        int start = -1;
        int length = 0;
    };

    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void newProject();
    W_SLOT(newProject)
    void openProject();
    W_SLOT(openProject)
    bool saveProject();
    W_SLOT(saveProject)
    bool saveProjectAs();
    W_SLOT(saveProjectAs)
    void onEpisodeSelected(const QString &chapterId, const QString &episodeId);
    W_SLOT(onEpisodeSelected, (const QString &, const QString &))
    void onCharacterSelected(const QString &characterId);
    W_SLOT(onCharacterSelected, (const QString &))
    void onLocationSelected(const QString &locationId);
    W_SLOT(onLocationSelected, (const QString &))
    void updateStatusBar();
    W_SLOT(updateStatusBar)
    void updateCursorStatus();
    W_SLOT(updateCursorStatus)
    void addChapter();
    W_SLOT(addChapter)
    void addEpisode(const QString &chapterId);
    W_SLOT(addEpisode, (const QString &))
    void renameChapter(const QString &chapterId, const QString &title);
    W_SLOT(renameChapter, (const QString &, const QString &))
    void renameEpisode(const QString &chapterId, const QString &episodeId, const QString &title);
    W_SLOT(renameEpisode, (const QString &, const QString &, const QString &))
    void addCharacter();
    W_SLOT(addCharacter)
    void addLocation();
    W_SLOT(addLocation)
    void polishCurrentEpisode();
    W_SLOT(polishCurrentEpisode)
    void rollbackLastAiEdit();
    W_SLOT(rollbackLastAiEdit)
    void projectSearch();
    W_SLOT(projectSearch)
    void quickOpen();
    W_SLOT(quickOpen)
    void commandPalette();
    W_SLOT(commandPalette)
    void showAiSettings();
    W_SLOT(showAiSettings)
    void applyPendingAiReview();
    W_SLOT(applyPendingAiReview)
    void discardPendingAiReview();
    W_SLOT(discardPendingAiReview)
    void restoreRevisionFromHistory(const QString &revisionId);
    W_SLOT(restoreRevisionFromHistory, (const QString &))
    void addProtectedSnippetFromSelection();
    W_SLOT(addProtectedSnippetFromSelection)
    void removeProtectedSnippet(int index);
    W_SLOT(removeProtectedSnippet, (int))
    void clearProtectedSnippets();
    W_SLOT(clearProtectedSnippets)

private:
    void closeEvent(QCloseEvent *event) override;
    void setupMenuBar();
    void setupToolBar();
    void setupDockManager();
    void setupDockWidgets();
    void setupDockViewMenu(QMenu *viewMenu);
    void setupStatusBar();
    void setupEditorTabs();
    void connectSignals();
    void setCurrentProject(const NovelProject &project);
    void applyStyleSheet();
    void loadWindowState();
    void saveWindowState();
    void loadSessionState();
    void saveSessionState();
    void loadRecentProjects();
    void updateRecentProjectsMenu();
    void addRecentProject(const QString &path);
    void openProjectFile(const QString &path);
    bool openDocumentByKind(const QString &kind, const QString &id);
    bool promptCloseDocument(QWidget *tab);
    bool confirmSaveChanges();
    bool maybeSave();
    void updateTabDecorations();
    void refreshExplorerOpenDocuments();
    void refreshProjectViews();
    void refreshWorkspaceView();
    void showWelcomePage();
    void showEditorWorkspace();
    void updateWelcomeState();
    void openFirstEpisodeIfAny();
    void closeOpenDocument(const QString &kind, const QString &id);
    void closeOtherOpenDocuments(const QString &kind, const QString &id);
    void markDocumentDirty(QWidget *widget);
    void markAllTabsDirty();
    void markAllTabsClean();
    void refreshTabTitle(QWidget *widget);
    QString baseTabTitle(QWidget *widget) const;
    QString currentBreadcrumbText() const;
    void updateBreadcrumbStatus();
    void syncOpenDocumentTabs();
    void openQuickOpenResult(const QString &kind, const QString &id);
    void openSearchResult(const QString &kind, const QString &id, const QString &query);
    void showDockPane(ads::CDockWidget *dock);
    void floatDockPane(ads::CDockWidget *dock);
    void showAllDockPanes();
    void resetDockLayout();
    void setFocusMode(bool enabled);
    void toggleFocusMode();
    void storeAiEditSnapshot(const QString &label);
    void clearAiEditSnapshot();
    void refreshRevisionHistoryPanel();
    void recordEpisodeRevision(const QString &episodeId, const QString &content,
                               int type,
                               const QString &notes = QString(),
                               const QString &aiModel = QString());
    void recordAllEpisodeRevisions(int type, const QString &notes = QString());
    void recordPlanAffectedEpisodeRevisions(const PlotEngine::AI::EditPlan &plan,
                                            int type,
                                            const QString &notes,
                                            const QString &aiModel = QString());
    void refreshProtectedSnippetPanel();
    void refreshSceneBoardPanel();
    void syncProtectedSnippetsFromEditor(NovelEditor *editor);
    QString currentDocumentProtectionKey() const;
    QString protectionKey(const QString &kind, const QString &id) const;

    NovelProject *m_projectData = nullptr;
    bool m_dirty = false;
    bool m_loadingSession = false;
    bool m_hasAiEditSnapshot = false;
    bool m_aiEditSnapshotDirty = false;
    NovelProject *m_aiEditSnapshotData = nullptr;
    QString m_aiEditSnapshotLabel;
    QByteArray m_defaultDockState;

    ads::CDockManager *m_dockManager = nullptr;
    QTabWidget *m_editorTabs = nullptr;
    StructurePanel *m_structurePanel = nullptr;
    NotePanel *m_notePanel = nullptr;
    SearchPanel *m_searchPanel = nullptr;
    ReviewPanel *m_reviewPanel = nullptr;
    RevisionHistoryPanel *m_revisionHistoryPanel = nullptr;
    ProtectedSnippetPanel *m_protectedSnippetPanel = nullptr;
    SceneBoardPanel *m_sceneBoardPanel = nullptr;
    ads::CDockWidget *m_structureDock = nullptr;
    ads::CDockWidget *m_noteDock = nullptr;
    ads::CDockWidget *m_searchDock = nullptr;
    ads::CDockWidget *m_reviewDock = nullptr;
    ads::CDockWidget *m_revisionHistoryDock = nullptr;
    ads::CDockWidget *m_protectedSnippetDock = nullptr;
    ads::CDockWidget *m_sceneBoardDock = nullptr;
    ads::CDockWidget *m_editorDock = nullptr;
    QAction *m_focusModeAction = nullptr;
    QStackedWidget *m_centerStack = nullptr;
    QWidget *m_welcomePage = nullptr;
    QPushButton *m_welcomeNewProjectButton = nullptr;
    QPushButton *m_welcomeOpenProjectButton = nullptr;
    QPushButton *m_welcomeChapterButton = nullptr;
    QPushButton *m_welcomeEpisodeButton = nullptr;
    QMenu *m_recentProjectsMenu = nullptr;
    QStringList m_recentProjects;
    QLabel *m_statusFile = nullptr;
    QLabel *m_statusWords = nullptr;
    QLabel *m_statusCursor = nullptr;
    QLabel *m_statusBreadcrumb = nullptr;
    QString m_pendingReviewChapterId;
    QString m_pendingReviewEpisodeId;
    QString m_pendingReviewInstruction;
    QString m_pendingReviewProtectedText;
    QString m_pendingReviewCurrentText;
    QString m_pendingReviewAiModel;
    QHash<QString, QVector<ProtectedSnippetRecord>> m_protectedSnippetsByDocument;
    bool m_focusModeEnabled = false;
    QHash<ads::CDockWidget*, bool> m_focusModeVisibilitySnapshot;
};
