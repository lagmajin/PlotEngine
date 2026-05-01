#pragma once

#include <QMainWindow>
#include <QDockWidget>
#include <QTabWidget>
#include <QLabel>
#include <QStatusBar>
#include <QMenuBar>
#include <QToolBar>
#include <QInputDialog>
#include "core/novelproject.h"
#include "editor/noveleditor.h"
#include "panels/structurepanel.h"
#include "panels/notepanel.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void newProject();
    void openProject();
    bool saveProject();
    bool saveProjectAs();
    void onSceneSelected(const QString &chapterId, const QString &sceneId);
    void onCharacterSelected(const QString &characterId);
    void onLocationSelected(const QString &locationId);
    void updateStatusBar();
    void addChapter();
    void addScene(const QString &chapterId);
    void addCharacter();
    void addLocation();

private:
    void setupMenuBar();
    void setupToolBar();
    void setupDockWidgets();
    void setupStatusBar();
    void setupEditorTabs();
    void connectSignals();
    void setCurrentProject(const NovelProject &project);
    void applyStyleSheet();
    bool confirmSaveChanges();
    bool maybeSave();

    NovelProject m_project;
    bool m_dirty = false;

    QTabWidget *m_editorTabs = nullptr;
    StructurePanel *m_structurePanel = nullptr;
    NotePanel *m_notePanel = nullptr;
    QDockWidget *m_structureDock = nullptr;
    QDockWidget *m_noteDock = nullptr;
    QLabel *m_statusFile = nullptr;
    QLabel *m_statusWords = nullptr;
    QLabel *m_statusCursor = nullptr;
};