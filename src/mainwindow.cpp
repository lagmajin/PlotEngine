#include "mainwindow.h"

#include "DockManager.h"
#include "editor/noveleditor.h"
#include "ai/veniceprovider.h"
#include "panels/notepanel.h"
#include "panels/reviewpanel.h"
#include "panels/revisionhistorypanel.h"
#include "panels/searchpanel.h"
#include "panels/structurepanel.h"
#include "panels/protectedsnippetpanel.h"
#include "panels/sceneboardpanel.h"
#include "wobjectimpl.h"
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QAbstractItemView>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QEventLoop>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QKeySequence>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLineEdit>
#include <QPushButton>
#include <QTabBar>
#include <QStackedWidget>
#include <QTextStream>
#include <QTextCursor>
#include <QTextBlock>
#include <QScreen>
#include <QProcessEnvironment>
#include <QInputDialog>
#include <QMenu>
#include <QSettings>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <QVBoxLayout>
#include <QLabel>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QVector>
#include <QList>

import PlotEngine.Core.ProjectManager;
import PlotEngine.Core.DocumentCommands;
import PlotEngine.Core.RevisionManager;
import PlotEngine.Core.RevisionStore;
import PlotEngine.Core.TextUtils;
import PlotEngine.AI.DataAction;
import PlotEngine.AI.RevisionPrompt;
import PlotEngine.AI.SecretStore;
import PlotEngine.App.Metadata;
import PlotEngine.UI.DockPane;
import PlotEngine.UI.IconFactory;

#define m_project (*m_projectData)
#define m_aiEditSnapshot (*m_aiEditSnapshotData)

W_OBJECT_IMPL(MainWindow)

namespace {
constexpr int kRecentProjectLimit = 10;

QIcon appIcon(const QString &name)
{
    return PlotEngine::UI::icon(name);
}

QString stripCodeFences(const QString &text)
{
    QString trimmed = text.trimmed();
    if (!trimmed.startsWith("```"))
        return trimmed;

    const int firstNewline = trimmed.indexOf('\n');
    if (firstNewline < 0)
        return trimmed;

    int start = firstNewline + 1;
    if (trimmed.mid(start).startsWith("```")) {
        start = trimmed.indexOf('\n', start);
        if (start < 0)
            return trimmed;
        ++start;
    }

    int end = trimmed.lastIndexOf("```");
    if (end <= start)
        return trimmed;

    return trimmed.mid(start, end - start).trimmed();
}

struct QuickOpenItem {
    QString kind;
    QString id;
    QString label;
    QString detail;
};

enum class PaletteCommandId {
    QuickOpen,
    ProjectSearch,
    Find,
    Replace,
    FindNext,
    FindPrevious,
    NewProject,
    OpenProject,
    SaveProject,
    SaveProjectAs,
    Polish,
    RollbackAiEdit,
    AddChapter,
    AddEpisode,
    OpenDocument
};

struct PaletteCommand {
    QString title;
    QString category;
    QString shortcut;
    PaletteCommandId id;
    QString documentKind;
    QString documentId;
};

ads::CDockWidget *createDockPane(ads::CDockManager *manager, const DockPaneSpec &spec, QWidget *widget)
{
    if (!manager || !widget)
        return nullptr;

    auto *dock = manager->createDockWidget(spec.title);
    dock->setFeatures(ads::CDockWidget::DefaultDockWidgetFeatures);
    dock->setMinimumSizeHintMode(ads::CDockWidget::MinimumSizeHintFromDockWidget);
    dock->setWidget(widget);
    switch (spec.placement) {
    case DockPlacement::Left:
        manager->addDockWidget(ads::LeftDockWidgetArea, dock);
        break;
    case DockPlacement::Center:
        manager->addDockWidget(ads::CenterDockWidgetArea, dock);
        break;
    case DockPlacement::Right:
        manager->addDockWidget(ads::RightDockWidgetArea, dock);
        break;
    case DockPlacement::Bottom:
        manager->addDockWidget(ads::BottomDockWidgetArea, dock);
        break;
    }
    dock->toggleViewAction()->setChecked(spec.visibleByDefault);
    return dock;
}

Chapter *findChapterById(NovelProject &project, const QString &chapterId)
{
    for (auto &chapter : project.chapters) {
        if (chapter.id == chapterId)
            return &chapter;
    }
    return nullptr;
}

const Chapter *findChapterById(const NovelProject &project, const QString &chapterId)
{
    for (const auto &chapter : project.chapters) {
        if (chapter.id == chapterId)
            return &chapter;
    }
    return nullptr;
}

Episode *findEpisodeById(NovelProject &project, const QString &chapterId, const QString &episodeId)
{
    Chapter *chapter = findChapterById(project, chapterId);
    if (!chapter)
        return nullptr;
    for (auto &episode : chapter->episodes) {
        if (episode.id == episodeId)
            return &episode;
    }
    return nullptr;
}

const Episode *findEpisodeById(const NovelProject &project, const QString &chapterId, const QString &episodeId)
{
    const Chapter *chapter = findChapterById(project, chapterId);
    if (!chapter)
        return nullptr;
    for (const auto &episode : chapter->episodes) {
        if (episode.id == episodeId)
            return &episode;
    }
    return nullptr;
}

CharacterEntry *findCharacterById(NovelProject &project, const QString &characterId)
{
    for (auto &character : project.characters) {
        if (character.id == characterId)
            return &character;
    }
    return nullptr;
}

LocationEntry *findLocationById(NovelProject &project, const QString &locationId)
{
    for (auto &location : project.locations) {
        if (location.id == locationId)
            return &location;
    }
    return nullptr;
}

QString documentTitleForProject(const NovelProject &project, const QString &kind, const QString &id)
{
    if (kind == "episode") {
        for (const auto &chapter : project.chapters) {
            for (const auto &episode : chapter.episodes) {
                if (episode.id == id)
                    return chapter.title + " / " + episode.title;
            }
        }
    } else if (kind == "character") {
        for (const auto &character : project.characters) {
            if (character.id == id)
                return "キャラ: " + character.name;
        }
    } else if (kind == "location") {
        for (const auto &location : project.locations) {
            if (location.id == id)
                return "場所: " + location.name;
        }
    }
    return QString();
}

QString protectedSnippetStorePath(const QString &projectPath)
{
    return projectPath.isEmpty() ? QString() : projectPath + QStringLiteral(".protected.json");
}

QHash<QString, QVector<MainWindow::ProtectedSnippetRecord>> loadProtectedSnippetStore(const QString &projectPath)
{
    QHash<QString, QVector<MainWindow::ProtectedSnippetRecord>> result;
    const QString path = protectedSnippetStorePath(projectPath);
    if (path.isEmpty() || !QFileInfo::exists(path))
        return result;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return result;

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
        return result;

    const QJsonObject root = doc.object();
    for (auto it = root.begin(); it != root.end(); ++it) {
        if (!it.value().isArray())
            continue;
        QVector<MainWindow::ProtectedSnippetRecord> records;
        for (const auto &entry : it.value().toArray()) {
            if (!entry.isObject())
                continue;
            const QJsonObject obj = entry.toObject();
            const QString text = obj.value(QStringLiteral("text")).toString();
            if (text.trimmed().isEmpty())
                continue;
            records.append({
                obj.value(QStringLiteral("label")).toString(),
                text,
                obj.contains(QStringLiteral("start")) ? obj.value(QStringLiteral("start")).toInt(-1) : -1,
                obj.contains(QStringLiteral("length")) ? obj.value(QStringLiteral("length")).toInt(static_cast<int>(text.size())) : static_cast<int>(text.size())
            });
        }
        if (!records.isEmpty())
            result.insert(it.key(), records);
    }

    return result;
}

bool saveProtectedSnippetStore(const QString &projectPath,
                               const QHash<QString, QVector<MainWindow::ProtectedSnippetRecord>> &store)
{
    const QString path = protectedSnippetStorePath(projectPath);
    if (path.isEmpty())
        return false;

    QJsonObject root;
    for (auto it = store.begin(); it != store.end(); ++it) {
        QJsonArray snippets;
        for (const auto &record : it.value()) {
            if (record.text.trimmed().isEmpty())
                continue;
            QJsonObject obj;
            obj[QStringLiteral("label")] = record.label;
            obj[QStringLiteral("text")] = record.text;
            if (record.start >= 0)
                obj[QStringLiteral("start")] = record.start;
            if (record.length > 0)
                obj[QStringLiteral("length")] = record.length;
            snippets.append(obj);
        }
        if (!snippets.isEmpty())
            root[it.key()] = snippets;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_projectData = new NovelProject{};
    m_aiEditSnapshotData = new NovelProject{};
    setMinimumSize(1000, 600);
    applyStyleSheet();
    setupDockManager();
    setupDockWidgets();
    setupEditorTabs();
    m_defaultDockState = m_dockManager->saveState(1);
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    connectSignals();
    loadRecentProjects();
    loadWindowState();
    loadSessionState();
    updateStatusBar();
}

MainWindow::~MainWindow()
{
    delete m_projectData;
    m_projectData = nullptr;
    delete m_aiEditSnapshotData;
    m_aiEditSnapshotData = nullptr;
}

void MainWindow::setupMenuBar()
{
    auto *fileMenu = menuBar()->addMenu("ファイル(&F)");

    auto *newAction = fileMenu->addAction(appIcon("new"), "新規プロジェクト(&N)");
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &MainWindow::newProject);

    auto *openAction = fileMenu->addAction(appIcon("open"), "プロジェクトを開く(&O)");
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::openProject);

    m_recentProjectsMenu = fileMenu->addMenu("最近使ったプロジェクト");
    m_recentProjectsMenu->setIcon(appIcon("recent"));
    updateRecentProjectsMenu();

    fileMenu->addSeparator();

    auto *saveAction = fileMenu->addAction(appIcon("save"), "保存(&S)");
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveProject);

    auto *saveAsAction = fileMenu->addAction(appIcon("save-as"), "名前を付けて保存(&A)");
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::saveProjectAs);

    fileMenu->addSeparator();

    auto *exitAction = fileMenu->addAction(appIcon("exit"), "終了(&X)");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    auto *editMenu = menuBar()->addMenu("編集(&E)");
    editMenu->addAction(appIcon("undo"), "元に戻す(&U)", this, [this]() {
        auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());
        if (editor) editor->undo();
    }, QKeySequence::Undo);
    editMenu->addAction(appIcon("redo"), "やり直し(&R)", this, [this]() {
        auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());
        if (editor) editor->redo();
    }, QKeySequence::Redo);
    editMenu->addSeparator();
    editMenu->addAction(appIcon("find"), "検索(&F)", this, [this]() {
        auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());
        if (!editor) return;
        editor->showSearchBar();
        const QString selected = editor->textCursor().selectedText().trimmed();
        if (!selected.isEmpty())
            editor->setSearchText(selected);
    }, QKeySequence::Find);
    editMenu->addAction(appIcon("replace"), "置換(&H)", this, [this]() {
        auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());
        if (!editor) return;
        editor->showReplaceBar();
        const QString selected = editor->textCursor().selectedText().trimmed();
        if (!selected.isEmpty())
            editor->setSearchText(selected);
    }, QKeySequence::Replace);
    editMenu->addAction(appIcon("find-next"), "次を検索(&G)", this, [this]() {
        auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());
        if (editor) editor->findNext();
    }, QKeySequence("F3"));
    editMenu->addAction(appIcon("find-previous"), "前を検索(&B)", this, [this]() {
        auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());
        if (editor) editor->findPrevious();
    }, QKeySequence("Shift+F3"));
    editMenu->addSeparator();
    editMenu->addAction(appIcon("quick-open"), "クイックオープン(&P)", this, &MainWindow::quickOpen, QKeySequence("Ctrl+P"));
    editMenu->addAction(appIcon("project-search"), "プロジェクト全体検索", this, &MainWindow::projectSearch, QKeySequence("Ctrl+Shift+F"));
    auto *paletteAction = editMenu->addAction(appIcon("command-palette"), "コマンドパレット(&C)", this, &MainWindow::commandPalette);
    paletteAction->setShortcuts({QKeySequence("Ctrl+Shift+P"), QKeySequence("Ctrl+K, Ctrl+P")});

    auto *aiMenu = menuBar()->addMenu("AI(&A)");
    aiMenu->addAction(appIcon("ai-settings"), "Venice 設定...", this, &MainWindow::showAiSettings);
    aiMenu->addAction(appIcon("ai-rollback"), "直前の AI 編集を戻す", this, &MainWindow::rollbackLastAiEdit);

    auto *viewMenu = menuBar()->addMenu("表示(&V)");
    setupDockViewMenu(viewMenu);

    auto *helpMenu = menuBar()->addMenu("ヘルプ(&H)");
    helpMenu->addAction(appIcon("about"), "PlotEngine について", this, [this]() {
        QMessageBox::about(this, "PlotEngine について",
            "PlotEngine v1.0.0\n小説制作統合開発環境");
    });
}

void MainWindow::setupToolBar()
{
    auto *toolbar = addToolBar("メインツールバー");
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(24, 24));

    toolbar->addAction(appIcon("new"), "新規", this, &MainWindow::newProject);
    toolbar->addAction(appIcon("open"), "開く", this, &MainWindow::openProject);
    toolbar->addAction(appIcon("save"), "保存", this, &MainWindow::saveProject);
    toolbar->addSeparator();
    toolbar->addAction(appIcon("quick-open"), "Quick Open", this, &MainWindow::quickOpen);
    toolbar->addAction(appIcon("project-search"), "Search", this, &MainWindow::projectSearch);
    toolbar->addAction(appIcon("command-palette"), "Command Palette", this, &MainWindow::commandPalette);
    if (m_focusModeAction)
        toolbar->addAction(m_focusModeAction);
    toolbar->addSeparator();
    toolbar->addAction(appIcon("add-chapter"), "章追加", this, &MainWindow::addChapter);
    toolbar->addAction(appIcon("add-episode"), "エピソード追加", this, [this]() {
        if (m_project.chapters.isEmpty()) return;
        addEpisode(m_project.chapters.first().id);
    });
    toolbar->addAction(appIcon("polish"), "推敲", this, &MainWindow::polishCurrentEpisode);
    toolbar->addSeparator();
    toolbar->addAction(appIcon("add-character"), "キャラ追加", this, &MainWindow::addCharacter);
    toolbar->addAction(appIcon("add-location"), "場所追加", this, &MainWindow::addLocation);
}

void MainWindow::setupDockManager()
{
    ads::CDockManager::setConfigFlags(
        ads::CDockManager::DefaultOpaqueConfig
        | ads::CDockManager::FloatingContainerHasWidgetIcon
        | ads::CDockManager::EqualSplitOnInsertion);
    ads::CDockManager::setConfigFlag(ads::CDockManager::TabsAtBottom, true);
    ads::CDockManager::setConfigFlag(ads::CDockManager::FocusHighlighting, true);
    ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaHideDisabledButtons, true);
    ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaDynamicTabsMenuButtonVisibility, true);
    ads::CDockManager::setConfigFlag(ads::CDockManager::HideSingleCentralWidgetTitleBar, false);
    ads::CDockManager::setAutoHideConfigFlags(ads::CDockManager::DefaultAutoHideConfig);
    ads::CDockManager::setFloatingContainersTitle("PlotEngine");
    m_dockManager = new ads::CDockManager(this);
}

void MainWindow::setupEditorTabs()
{
    m_centerStack = new QStackedWidget(this);
    m_editorTabs = new QTabWidget(m_centerStack);
    m_editorTabs->setTabsClosable(true);
    m_editorTabs->setMovable(true);
    m_editorTabs->setDocumentMode(true);
    m_editorTabs->setElideMode(Qt::ElideMiddle);
    m_editorTabs->setUsesScrollButtons(true);
    m_editorTabs->tabBar()->setExpanding(false);
    m_welcomePage = new QWidget(m_centerStack);
    auto *welcomeLayout = new QVBoxLayout(m_welcomePage);
    welcomeLayout->setContentsMargins(24, 24, 24, 24);
    welcomeLayout->setSpacing(12);

    auto *welcomeTitle = new QLabel("PlotEngine", m_welcomePage);
    QFont titleFont = welcomeTitle->font();
    titleFont.setPointSize(titleFont.pointSize() + 8);
    titleFont.setBold(true);
    welcomeTitle->setFont(titleFont);

    auto *welcomeText = new QLabel(
        "プロジェクトを作成するか、最近の作業から再開してください。\n"
        "章やエピソードはここからすぐ追加できます。",
        m_welcomePage);
    welcomeText->setWordWrap(true);

    m_welcomeNewProjectButton = new QPushButton("新規プロジェクト", m_welcomePage);
    m_welcomeOpenProjectButton = new QPushButton("プロジェクトを開く", m_welcomePage);
    m_welcomeChapterButton = new QPushButton("章を追加", m_welcomePage);
    m_welcomeEpisodeButton = new QPushButton("最初のエピソードを開く", m_welcomePage);

    auto *primaryRow = new QHBoxLayout();
    primaryRow->setSpacing(8);
    primaryRow->addWidget(m_welcomeNewProjectButton);
    primaryRow->addWidget(m_welcomeOpenProjectButton);

    auto *secondaryRow = new QHBoxLayout();
    secondaryRow->setSpacing(8);
    secondaryRow->addWidget(m_welcomeChapterButton);
    secondaryRow->addWidget(m_welcomeEpisodeButton);

    welcomeLayout->addWidget(welcomeTitle);
    welcomeLayout->addWidget(welcomeText);
    welcomeLayout->addSpacing(6);
    welcomeLayout->addLayout(primaryRow);
    welcomeLayout->addLayout(secondaryRow);
    welcomeLayout->addStretch(1);

    connect(m_welcomeNewProjectButton, &QPushButton::clicked, this, &MainWindow::newProject);
    connect(m_welcomeOpenProjectButton, &QPushButton::clicked, this, &MainWindow::openProject);
    connect(m_welcomeChapterButton, &QPushButton::clicked, this, &MainWindow::addChapter);
    connect(m_welcomeEpisodeButton, &QPushButton::clicked, this, [this]() {
        openFirstEpisodeIfAny();
    });

    m_centerStack->addWidget(m_welcomePage);
    m_centerStack->addWidget(m_editorTabs);
    m_centerStack->setCurrentWidget(m_welcomePage);

    m_editorDock = createDockPane(m_dockManager, { QStringLiteral("エディタ"), DockPlacement::Center, true }, m_centerStack);
    m_editorDock->setIcon(appIcon("editor"));

    connect(m_editorTabs, &QTabWidget::tabCloseRequested, this, [this](int index) {
        auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->widget(index));
        if (editor && !promptCloseDocument(editor))
            return;
        m_editorTabs->removeTab(index);
        if (editor) editor->deleteLater();
        refreshExplorerOpenDocuments();
        updateCursorStatus();
        updateBreadcrumbStatus();
        refreshRevisionHistoryPanel();
        refreshProtectedSnippetPanel();
        refreshWorkspaceView();
    });
    connect(m_editorTabs, &QTabWidget::currentChanged, this, [this]() {
        updateCursorStatus();
        refreshExplorerOpenDocuments();
        updateBreadcrumbStatus();
        refreshRevisionHistoryPanel();
        refreshProtectedSnippetPanel();
        refreshSceneBoardPanel();
        refreshWorkspaceView();
    });
}

void MainWindow::setupDockWidgets()
{
    m_structurePanel = new StructurePanel(this);
    m_structureDock = createDockPane(m_dockManager, StructurePanel::dockSpec(), m_structurePanel);
    m_structureDock->setIcon(appIcon("explorer"));

    m_notePanel = new NotePanel(this);
    m_noteDock = createDockPane(m_dockManager, NotePanel::dockSpec(), m_notePanel);
    m_noteDock->setIcon(appIcon("notes"));

    m_searchPanel = new SearchPanel(this);
    m_searchDock = createDockPane(m_dockManager, SearchPanel::dockSpec(), m_searchPanel);
    m_searchDock->setIcon(appIcon("search"));

    m_reviewPanel = new ReviewPanel(this);
    m_reviewDock = createDockPane(m_dockManager, ReviewPanel::dockSpec(), m_reviewPanel);
    m_reviewDock->setIcon(appIcon("polish"));

    m_revisionHistoryPanel = new RevisionHistoryPanel(this);
    m_revisionHistoryDock = createDockPane(m_dockManager, RevisionHistoryPanel::dockSpec(), m_revisionHistoryPanel);
    m_revisionHistoryDock->setIcon(appIcon("recent"));

    m_protectedSnippetPanel = new ProtectedSnippetPanel(this);
    m_protectedSnippetDock = createDockPane(m_dockManager, ProtectedSnippetPanel::dockSpec(), m_protectedSnippetPanel);
    m_protectedSnippetDock->setIcon(appIcon("notes"));

    m_sceneBoardPanel = new SceneBoardPanel(this);
    m_sceneBoardDock = createDockPane(m_dockManager, SceneBoardPanel::dockSpec(), m_sceneBoardPanel);
    m_sceneBoardDock->setIcon(appIcon("structure"));
}

void MainWindow::setupDockViewMenu(QMenu *viewMenu)
{
    if (!viewMenu)
        return;

    m_structureDock->toggleViewAction()->setIcon(appIcon("explorer"));
    m_noteDock->toggleViewAction()->setIcon(appIcon("notes"));
    m_searchDock->toggleViewAction()->setIcon(appIcon("search"));
    m_reviewDock->toggleViewAction()->setIcon(appIcon("polish"));
    m_revisionHistoryDock->toggleViewAction()->setIcon(appIcon("recent"));
    m_protectedSnippetDock->toggleViewAction()->setIcon(appIcon("notes"));
    m_sceneBoardDock->toggleViewAction()->setIcon(appIcon("structure"));
    m_editorDock->toggleViewAction()->setIcon(appIcon("editor"));

    viewMenu->addAction(m_structureDock->toggleViewAction());
    viewMenu->addAction(m_noteDock->toggleViewAction());
    viewMenu->addAction(m_searchDock->toggleViewAction());
    viewMenu->addAction(m_reviewDock->toggleViewAction());
    viewMenu->addAction(m_revisionHistoryDock->toggleViewAction());
    viewMenu->addAction(m_protectedSnippetDock->toggleViewAction());
    viewMenu->addAction(m_sceneBoardDock->toggleViewAction());
    viewMenu->addAction(m_editorDock->toggleViewAction());

    viewMenu->addSeparator();
    viewMenu->addAction(appIcon("explorer"), "全ペインを表示", this, &MainWindow::showAllDockPanes);
    viewMenu->addAction(appIcon("editor"), "ドックレイアウトを初期化", this, &MainWindow::resetDockLayout);
    if (!m_focusModeAction) {
        m_focusModeAction = new QAction(appIcon("editor"), "集中執筆モード", this);
        m_focusModeAction->setCheckable(true);
        m_focusModeAction->setShortcut(QKeySequence("F11"));
        connect(m_focusModeAction, &QAction::triggered, this, [this]() {
            toggleFocusMode();
        });
    }
    viewMenu->addAction(m_focusModeAction);
    auto *floatMenu = viewMenu->addMenu(appIcon("quick-open"), "フローティング表示");
    floatMenu->addAction(appIcon("explorer"), "エクスプローラを浮かせる", this, [this]() {
        floatDockPane(m_structureDock);
    });
    floatMenu->addAction(appIcon("notes"), "ノートを浮かせる", this, [this]() {
        floatDockPane(m_noteDock);
    });
    floatMenu->addAction(appIcon("search"), "検索を浮かせる", this, [this]() {
        floatDockPane(m_searchDock);
    });
    floatMenu->addAction(appIcon("polish"), "AI レビューを浮かせる", this, [this]() {
        floatDockPane(m_reviewDock);
    });
    floatMenu->addAction(appIcon("recent"), "リビジョン履歴を浮かせる", this, [this]() {
        floatDockPane(m_revisionHistoryDock);
    });
    floatMenu->addAction(appIcon("notes"), "保護ブロックを浮かせる", this, [this]() {
        floatDockPane(m_protectedSnippetDock);
    });
    floatMenu->addAction(appIcon("structure"), "Scene Cards を浮かせる", this, [this]() {
        floatDockPane(m_sceneBoardDock);
    });
    floatMenu->addAction(appIcon("editor"), "エディタを浮かせる", this, [this]() {
        floatDockPane(m_editorDock);
    });
}

void MainWindow::showDockPane(ads::CDockWidget *dock)
{
    if (!dock)
        return;

    dock->toggleView(true);
    dock->raise();
}

void MainWindow::floatDockPane(ads::CDockWidget *dock)
{
    if (!dock)
        return;

    dock->toggleView(true);
    if (!dock->isFloating())
        dock->setFloating();
    dock->raise();
}

void MainWindow::showAllDockPanes()
{
    showDockPane(m_structureDock);
    showDockPane(m_noteDock);
    showDockPane(m_searchDock);
    showDockPane(m_reviewDock);
    showDockPane(m_revisionHistoryDock);
    showDockPane(m_protectedSnippetDock);
    showDockPane(m_sceneBoardDock);
    showDockPane(m_editorDock);
}

void MainWindow::resetDockLayout()
{
    if (!m_dockManager || m_defaultDockState.isEmpty())
        return;

    setFocusMode(false);
    m_dockManager->restoreState(m_defaultDockState, 1);
    showAllDockPanes();
}

void MainWindow::setFocusMode(bool enabled)
{
    if (m_focusModeEnabled == enabled)
        return;

    m_focusModeEnabled = enabled;
    if (m_focusModeAction)
        m_focusModeAction->setChecked(enabled);

    if (!m_dockManager)
        return;

    if (enabled) {
        m_focusModeVisibilitySnapshot.clear();
        const QList<ads::CDockWidget*> docks = {
            m_structureDock, m_noteDock, m_searchDock, m_reviewDock,
            m_revisionHistoryDock, m_protectedSnippetDock, m_sceneBoardDock
        };
        for (auto *dock : docks) {
            if (!dock)
                continue;
            m_focusModeVisibilitySnapshot.insert(dock, dock->toggleViewAction() && dock->toggleViewAction()->isChecked());
            dock->toggleView(false);
        }
        if (m_editorDock)
            m_editorDock->toggleView(true);
        showEditorWorkspace();
        return;
    }

    for (auto it = m_focusModeVisibilitySnapshot.begin(); it != m_focusModeVisibilitySnapshot.end(); ++it) {
        if (!it.key())
            continue;
        it.key()->toggleView(it.value());
    }
    m_focusModeVisibilitySnapshot.clear();
    refreshWorkspaceView();
}

void MainWindow::toggleFocusMode()
{
    setFocusMode(!m_focusModeEnabled);
}

void MainWindow::setupStatusBar()
{
    m_statusFile = new QLabel("プロジェクト未読み込み");
    m_statusWords = new QLabel("文字数: 0");
    m_statusCursor = new QLabel("行: 1 列: 1");
    m_statusBreadcrumb = new QLabel("エクスプローラ");

    statusBar()->addWidget(m_statusFile, 1);
    statusBar()->addWidget(m_statusBreadcrumb, 2);
    statusBar()->addPermanentWidget(m_statusWords);
    statusBar()->addPermanentWidget(m_statusCursor);
}

void MainWindow::connectSignals()
{
    connect(m_structurePanel, &StructurePanel::episodeSelected,
            this, &MainWindow::onEpisodeSelected);
    connect(m_structurePanel, &StructurePanel::episodeAddRequested,
            this, &MainWindow::addEpisode);
    connect(m_structurePanel, &StructurePanel::chapterRenamed,
            this, &MainWindow::renameChapter);
    connect(m_structurePanel, &StructurePanel::episodeRenamed,
            this, &MainWindow::renameEpisode);
    connect(m_structurePanel, &StructurePanel::openDocumentRequested,
            this, &MainWindow::openQuickOpenResult);
    connect(m_structurePanel, &StructurePanel::openDocumentCloseRequested,
            this, &MainWindow::closeOpenDocument);
    connect(m_structurePanel, &StructurePanel::openOtherDocumentsRequested,
            this, &MainWindow::closeOtherOpenDocuments);
    connect(m_notePanel, &NotePanel::characterSelected,
            this, &MainWindow::onCharacterSelected);
    connect(m_notePanel, &NotePanel::locationSelected,
            this, &MainWindow::onLocationSelected);
    connect(m_searchPanel, &SearchPanel::searchResultActivated,
            this, &MainWindow::openSearchResult);
    connect(m_reviewPanel, &ReviewPanel::applyRequested,
            this, &MainWindow::applyPendingAiReview);
    connect(m_reviewPanel, &ReviewPanel::discardRequested,
            this, &MainWindow::discardPendingAiReview);
    connect(m_revisionHistoryPanel, &RevisionHistoryPanel::restoreRequested,
            this, &MainWindow::restoreRevisionFromHistory);
    connect(m_protectedSnippetPanel, &ProtectedSnippetPanel::addCurrentSelectionRequested,
            this, &MainWindow::addProtectedSnippetFromSelection);
    connect(m_protectedSnippetPanel, &ProtectedSnippetPanel::removeSnippetRequested,
            this, &MainWindow::removeProtectedSnippet);
    connect(m_protectedSnippetPanel, &ProtectedSnippetPanel::clearSnippetsRequested,
            this, &MainWindow::clearProtectedSnippets);
    connect(m_sceneBoardPanel, &SceneBoardPanel::episodeSelected,
            this, &MainWindow::onEpisodeSelected);
    connect(m_sceneBoardPanel, &SceneBoardPanel::episodeOrderChanged,
            this, [this](const QString &chapterId, const QStringList &orderedEpisodeIds) {
                if (!PlotEngine::Docs::reorderEpisodes(m_project, chapterId, orderedEpisodeIds)) {
                    refreshSceneBoardPanel();
                    return;
                }
                markAllTabsDirty();
                refreshProjectViews();
            });
}

void MainWindow::quickOpen()
{
    QDialog dialog(this);
    dialog.setWindowTitle("クイックオープン");
    dialog.resize(720, 480);

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    auto *hint = new QLabel("タイトルを入力して候補を絞り込みます。Enter で開きます。", &dialog);
    layout->addWidget(hint);

    auto *filterEdit = new QLineEdit(&dialog);
    filterEdit->setPlaceholderText("検索...");
    layout->addWidget(filterEdit);

    auto *list = new QListWidget(&dialog);
    list->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(list, 1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    layout->addWidget(buttons);

    QVector<QuickOpenItem> items;
    for (const auto &ch : m_project.chapters) {
        items.append({ "chapter", ch.id, ch.title, "章" });
        for (const auto &ep : ch.episodes)
            items.append({ "episode", ep.id, ch.title + " / " + ep.title, "エピソード" });
    }
    for (const auto &c : m_project.characters)
        items.append({ "character", c.id, "キャラ: " + c.name, "キャラクター" });
    for (const auto &l : m_project.locations)
        items.append({ "location", l.id, "場所: " + l.name, "ロケーション" });

    auto refresh = [&]() {
        const QString filter = filterEdit->text().trimmed();
        list->clear();
        for (const auto &item : items) {
            if (!filter.isEmpty() && !item.label.contains(filter, Qt::CaseInsensitive) &&
                !item.detail.contains(filter, Qt::CaseInsensitive))
                continue;

            auto *entry = new QListWidgetItem(item.label + "    " + item.detail, list);
            entry->setData(Qt::UserRole, item.kind);
            entry->setData(Qt::UserRole + 1, item.id);
        }
        if (list->count() > 0)
            list->setCurrentRow(0);
    };

    connect(filterEdit, &QLineEdit::textChanged, &dialog, [&](const QString &) {
        refresh();
    });
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(list, &QListWidget::itemActivated, &dialog, [&](QListWidgetItem *item) {
        if (!item) return;
        openQuickOpenResult(item->data(Qt::UserRole).toString(), item->data(Qt::UserRole + 1).toString());
        dialog.accept();
    });
    connect(filterEdit, &QLineEdit::returnPressed, &dialog, [&]() {
        auto *item = list->currentItem();
        if (!item) return;
        openQuickOpenResult(item->data(Qt::UserRole).toString(), item->data(Qt::UserRole + 1).toString());
        dialog.accept();
    });

    refresh();
    filterEdit->setFocus();
    dialog.exec();
}

void MainWindow::projectSearch()
{
    if (m_searchDock)
        m_searchDock->toggleView(true);
    if (m_searchPanel)
        m_searchPanel->setQuery(QString());
}

void MainWindow::commandPalette()
{
    QDialog dialog(this);
    dialog.setWindowTitle("コマンドパレット");
    dialog.resize(760, 520);

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    auto *filterEdit = new QLineEdit(&dialog);
    filterEdit->setPlaceholderText("コマンドを入力...");
    layout->addWidget(filterEdit);

    auto *list = new QListWidget(&dialog);
    list->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(list, 1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    layout->addWidget(buttons);

    QVector<PaletteCommand> commands;
    commands.append({"クイックオープン", "ナビゲーション", "Ctrl+P", PaletteCommandId::QuickOpen, {}, {}});
    commands.append({"プロジェクト全体検索", "ナビゲーション", "Ctrl+Shift+F", PaletteCommandId::ProjectSearch, {}, {}});
    commands.append({"検索", "エディタ", "Ctrl+F", PaletteCommandId::Find, {}, {}});
    commands.append({"置換", "エディタ", "Ctrl+H", PaletteCommandId::Replace, {}, {}});
    commands.append({"次を検索", "エディタ", "F3", PaletteCommandId::FindNext, {}, {}});
    commands.append({"前を検索", "エディタ", "Shift+F3", PaletteCommandId::FindPrevious, {}, {}});
    commands.append({"新規プロジェクト", "ファイル", "Ctrl+N", PaletteCommandId::NewProject, {}, {}});
    commands.append({"プロジェクトを開く", "ファイル", "Ctrl+O", PaletteCommandId::OpenProject, {}, {}});
    commands.append({"保存", "ファイル", "Ctrl+S", PaletteCommandId::SaveProject, {}, {}});
    commands.append({"名前を付けて保存", "ファイル", "Ctrl+Shift+S", PaletteCommandId::SaveProjectAs, {}, {}});
    commands.append({"推敲", "AI", "", PaletteCommandId::Polish, {}, {}});
    commands.append({"直前の AI 編集を戻す", "AI", "", PaletteCommandId::RollbackAiEdit, {}, {}});
    commands.append({"章を追加", "構造", "", PaletteCommandId::AddChapter, {}, {}});
    commands.append({"エピソードを追加", "構造", "", PaletteCommandId::AddEpisode, {}, {}});

    for (int i = 0; i < m_editorTabs->count(); ++i) {
        auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->widget(i));
        if (!editor)
            continue;

        const QString kind = editor->property("documentKind").toString();
        const QString id = editor->property("documentId").toString();
        if (kind != "episode" && kind != "character" && kind != "location")
            continue;

        const QString title = editor->property("baseTabTitle").toString();
        const bool dirty = editor->property("isDirty").toBool();
        const QString category =
            kind == "episode" ? QStringLiteral("開いているエディタ / エピソード") :
            kind == "character" ? QStringLiteral("開いているエディタ / キャラクター") :
            QStringLiteral("開いているエディタ / ロケーション");
        const QString shortcut = dirty ? QStringLiteral("未保存") : QString();

        commands.append({dirty ? title + QStringLiteral(" *") : title, category, shortcut,
                         PaletteCommandId::OpenDocument, kind, id});
    }

    auto refresh = [&]() {
        const QString filter = filterEdit->text().trimmed();
        list->clear();
        for (int i = 0; i < commands.size(); ++i) {
            const auto &cmd = commands[i];
            if (!filter.isEmpty()
                && !cmd.title.contains(filter, Qt::CaseInsensitive)
                && !cmd.category.contains(filter, Qt::CaseInsensitive)
                && !cmd.shortcut.contains(filter, Qt::CaseInsensitive)) {
                continue;
            }

            auto *item = new QListWidgetItem(cmd.title, list);
            item->setData(Qt::UserRole, cmd.category);
            item->setData(Qt::UserRole + 1, cmd.shortcut);
            item->setData(Qt::UserRole + 2, i);
            item->setToolTip(cmd.shortcut.isEmpty() ? cmd.category : cmd.category + " - " + cmd.shortcut);
        }
        if (list->count() > 0)
            list->setCurrentRow(0);
    };

    auto runCurrent = [&]() {
        auto *item = list->currentItem();
        if (!item) return;
        const int index = item->data(Qt::UserRole + 2).toInt();
        if (index < 0 || index >= commands.size())
            return;
        dialog.accept();
        const PaletteCommand &command = commands[index];
        switch (command.id) {
        case PaletteCommandId::QuickOpen:
            quickOpen();
            break;
        case PaletteCommandId::ProjectSearch:
            projectSearch();
            break;
        case PaletteCommandId::Find: {
            auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());
            if (editor) editor->showSearchBar();
            break;
        }
        case PaletteCommandId::Replace: {
            auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());
            if (editor) editor->showReplaceBar();
            break;
        }
        case PaletteCommandId::FindNext: {
            auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());
            if (editor) editor->findNext();
            break;
        }
        case PaletteCommandId::FindPrevious: {
            auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());
            if (editor) editor->findPrevious();
            break;
        }
        case PaletteCommandId::NewProject:
            newProject();
            break;
        case PaletteCommandId::OpenProject:
            openProject();
            break;
        case PaletteCommandId::SaveProject:
            saveProject();
            break;
        case PaletteCommandId::SaveProjectAs:
            saveProjectAs();
            break;
        case PaletteCommandId::Polish:
            polishCurrentEpisode();
            break;
        case PaletteCommandId::RollbackAiEdit:
            rollbackLastAiEdit();
            break;
        case PaletteCommandId::AddChapter:
            addChapter();
            break;
        case PaletteCommandId::AddEpisode:
            if (!m_project.chapters.isEmpty())
                addEpisode(m_project.chapters.first().id);
            break;
        case PaletteCommandId::OpenDocument:
            openQuickOpenResult(command.documentKind, command.documentId);
            break;
        }
    };

    connect(filterEdit, &QLineEdit::textChanged, &dialog, [&](const QString &) {
        refresh();
    });
    connect(filterEdit, &QLineEdit::returnPressed, &dialog, [&]() {
        runCurrent();
    });
    connect(list, &QListWidget::itemActivated, &dialog, [&](QListWidgetItem *) {
        runCurrent();
    });
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    refresh();
    filterEdit->setFocus();
    dialog.exec();
}

void MainWindow::showAiSettings()
{
    QDialog dialog(this);
    dialog.setWindowTitle("AI設定");
    dialog.resize(840, 560);
    dialog.setObjectName("aiSettingsDialog");
    dialog.setStyleSheet(R"(
        QDialog#aiSettingsDialog {
            background: #1e1e1e;
            color: #d4d4d4;
        }
        QGroupBox {
            border: 1px solid #3c3c3c;
            border-radius: 6px;
            margin-top: 12px;
            padding-top: 10px;
            background: #252526;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 4px;
            color: #c8c8c8;
        }
        QLineEdit, QComboBox {
            background: #111111;
            color: #ffffff;
            border: 1px solid #3c3c3c;
            padding: 6px 8px;
            border-radius: 4px;
        }
        QTabWidget::pane {
            border: 1px solid #3c3c3c;
            background: #1e1e1e;
        }
        QTabBar::tab {
            background: #2d2d2d;
            color: #c8c8c8;
            padding: 7px 12px;
            border: 1px solid #3c3c3c;
            border-bottom: none;
        }
        QTabBar::tab:selected {
            background: #1e1e1e;
            color: #ffffff;
            border-bottom: 2px solid #007acc;
        }
        QLabel#dialogTitle {
            color: #ffffff;
            font-size: 18px;
            font-weight: 700;
        }
        QLabel#dialogHint {
            color: #a6adc8;
        }
        QLabel#storageHint {
            color: #a6adc8;
        }
    )");

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    auto *title = new QLabel("AI 接続設定", &dialog);
    title->setObjectName("dialogTitle");
    auto *hint = new QLabel(
        "Venice を既定にして、他のプロバイダは予備に置いています。\n"
        "秘密情報は Windows では DPAPI で暗号化して保存されます。",
        &dialog);
    hint->setObjectName("dialogHint");
    hint->setWordWrap(true);
    layout->addWidget(title);
    layout->addWidget(hint);

    QComboBox *providerCombo = new QComboBox(&dialog);
    providerCombo->addItems({"Venice", "OpenAI", "Anthropic"});

    QLineEdit *veniceKeyEdit = new QLineEdit(&dialog);
    veniceKeyEdit->setEchoMode(QLineEdit::Password);
    QComboBox *veniceModelCombo = new QComboBox(&dialog);
    veniceModelCombo->setEditable(true);
    VeniceProvider modelProbe(QString(), &dialog);
    veniceModelCombo->addItems(modelProbe.availableModels());
    if (veniceModelCombo->count() > 0)
        veniceModelCombo->setCurrentIndex(0);
    QLineEdit *veniceBaseUrlEdit = new QLineEdit("https://api.venice.ai/api/v1", &dialog);

    QLineEdit *openAiKeyEdit = new QLineEdit(&dialog);
    openAiKeyEdit->setEchoMode(QLineEdit::Password);
    QLineEdit *openAiBaseUrlEdit = new QLineEdit("https://api.openai.com", &dialog);
    QLineEdit *anthropicKeyEdit = new QLineEdit(&dialog);
    anthropicKeyEdit->setEchoMode(QLineEdit::Password);

    QCheckBox *saveSecurelyCheck = new QCheckBox("Windows の安全な保存先に暗号化して保存する", &dialog);
    saveSecurelyCheck->setChecked(true);

    QSettings settings = PlotEngine::App::makeSettings();
    providerCombo->setCurrentText(settings.value("ai/defaultProvider", "Venice").toString());
    veniceModelCombo->setCurrentText(settings.value("ai/venice/model", "venice-uncensored").toString());
    veniceBaseUrlEdit->setText(settings.value("ai/venice/baseUrl", "https://api.venice.ai/api/v1").toString());

    veniceKeyEdit->setText(PlotEngine::AI::loadSecret("venice"));
    openAiKeyEdit->setText(PlotEngine::AI::loadSecret("openai"));
    anthropicKeyEdit->setText(PlotEngine::AI::loadSecret("anthropic"));

    auto *profileGroup = new QGroupBox("プロファイル", &dialog);
    auto *profileLayout = new QFormLayout(profileGroup);
    profileLayout->setLabelAlignment(Qt::AlignRight);
    profileLayout->addRow("既定プロバイダ", providerCombo);
    profileLayout->addRow("Venice Model", veniceModelCombo);
    layout->addWidget(profileGroup);

    auto *tabs = new QTabWidget(&dialog);
    tabs->setDocumentMode(true);

    auto *veniceTab = new QWidget(tabs);
    auto *veniceForm = new QFormLayout(veniceTab);
    veniceForm->setLabelAlignment(Qt::AlignRight);
    veniceForm->addRow("Venice API Key", veniceKeyEdit);
    veniceForm->addRow("Venice Base URL", veniceBaseUrlEdit);

    auto *veniceNote = new QLabel("推敲はまず Venice を使います。OpenAI compatible のエンドポイントを前提にしています。", veniceTab);
    veniceNote->setWordWrap(true);
    veniceForm->addRow("説明", veniceNote);
    tabs->addTab(veniceTab, "Venice");

    auto *openAiTab = new QWidget(tabs);
    auto *openAiForm = new QFormLayout(openAiTab);
    openAiForm->setLabelAlignment(Qt::AlignRight);
    openAiForm->addRow("OpenAI API Key", openAiKeyEdit);
    openAiForm->addRow("OpenAI Base URL", openAiBaseUrlEdit);
    auto *openAiNote = new QLabel("互換 API を叩く場合の予備枠です。今は Venice を主運用にする前提です。", openAiTab);
    openAiNote->setWordWrap(true);
    openAiForm->addRow("説明", openAiNote);
    tabs->addTab(openAiTab, "OpenAI");

    auto *anthropicTab = new QWidget(tabs);
    auto *anthropicForm = new QFormLayout(anthropicTab);
    anthropicForm->setLabelAlignment(Qt::AlignRight);
    anthropicForm->addRow("Anthropic API Key", anthropicKeyEdit);
    auto *anthropicNote = new QLabel("Anthropic は互換用の予備です。", anthropicTab);
    anthropicNote->setWordWrap(true);
    anthropicForm->addRow("説明", anthropicNote);
    tabs->addTab(anthropicTab, "Anthropic");

    layout->addWidget(tabs, 1);
    layout->addWidget(saveSecurelyCheck);

    auto *storageHint = new QLabel(
        "保存時の挙動: Windows では DPAPI により暗号化して保存。"
        " 空欄にすると保存済みキーを削除します。",
        &dialog);
    storageHint->setObjectName("storageHint");
    storageHint->setWordWrap(true);
    layout->addWidget(storageHint);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Close, &dialog);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&]() {
        QSettings s = PlotEngine::App::makeSettings();
        s.setValue("ai/defaultProvider", providerCombo->currentText());
        s.setValue("ai/venice/model", veniceModelCombo->currentText().trimmed());
        s.setValue("ai/venice/baseUrl", veniceBaseUrlEdit->text().trimmed());
        s.setValue("ai/openai/baseUrl", openAiBaseUrlEdit->text().trimmed());

        if (saveSecurelyCheck->isChecked()) {
            if (veniceKeyEdit->text().trimmed().isEmpty()) {
                PlotEngine::AI::clearSecret("venice");
            } else {
                PlotEngine::AI::saveSecret("venice", veniceKeyEdit->text().trimmed());
            }

            if (openAiKeyEdit->text().trimmed().isEmpty()) {
                PlotEngine::AI::clearSecret("openai");
            } else {
                PlotEngine::AI::saveSecret("openai", openAiKeyEdit->text().trimmed());
            }

            if (anthropicKeyEdit->text().trimmed().isEmpty()) {
                PlotEngine::AI::clearSecret("anthropic");
            } else {
                PlotEngine::AI::saveSecret("anthropic", anthropicKeyEdit->text().trimmed());
            }
        } else {
            PlotEngine::AI::clearSecret("venice");
            PlotEngine::AI::clearSecret("openai");
            PlotEngine::AI::clearSecret("anthropic");
        }

        s.sync();
        dialog.accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    dialog.exec();
}

void MainWindow::newProject()
{
    if (!maybeSave()) return;

    bool ok;
    QString name = QInputDialog::getText(this, "新規プロジェクト",
        "プロジェクト名:", QLineEdit::Normal, "新しい小説", &ok);
    if (!ok || name.isEmpty()) return;

    QString author = QInputDialog::getText(this, "新規プロジェクト",
        "作者名:", QLineEdit::Normal, QString(), &ok);
    if (!ok) return;

    setCurrentProject(ProjectManager::createNew(name, author));
    markAllTabsDirty();
    refreshProjectViews();
    openFirstEpisodeIfAny();
    updateStatusBar();
}

void MainWindow::openProject()
{
    if (!maybeSave()) return;

    QString path = QFileDialog::getOpenFileName(this,
        "プロジェクトを開く", QString(),
        "PlotEngine プロジェクト (*.plotproj);;全てのファイル (*)");

    if (path.isEmpty()) return;

    openProjectFile(path);
}

void MainWindow::openProjectFile(const QString &path)
{
    if (path.isEmpty())
        return;

    auto result = ProjectManager::load(path);
    if (!result) {
        QMessageBox::warning(this, "エラー", "プロジェクトの読み込みに失敗しました。");
        return;
    }

    setCurrentProject(*result);
    m_protectedSnippetsByDocument = loadProtectedSnippetStore(result->filePath);
    markAllTabsClean();
    refreshProjectViews();
    refreshProtectedSnippetPanel();
    addRecentProject(path);
    if (!m_loadingSession)
        openFirstEpisodeIfAny();
    updateStatusBar();
}

bool MainWindow::saveProject()
{
    if (m_project.filePath.isEmpty())
        return saveProjectAs();
    bool ok = ProjectManager::save(m_project, m_project.filePath);
    if (ok && !saveProtectedSnippetStore(m_project.filePath, m_protectedSnippetsByDocument)) {
        QMessageBox::warning(this, "保存エラー", "保護ブロック設定の保存に失敗しました。");
        ok = false;
    }
    if (ok) {
        markAllTabsClean();
        recordAllEpisodeRevisions(static_cast<int>(PlotEngine::Revisions::RevisionType::ManualEdit), QStringLiteral("プロジェクト保存"));
    }
    if (ok && !m_project.filePath.isEmpty())
        addRecentProject(m_project.filePath);
    updateStatusBar();
    refreshRevisionHistoryPanel();
    return ok;
}

bool MainWindow::saveProjectAs()
{
    QString path = QFileDialog::getSaveFileName(this,
        "名前を付けて保存", QString(),
        "PlotEngine プロジェクト (*.plotproj);;全てのファイル (*)");
    if (path.isEmpty()) return false;

    if (!path.endsWith(".plotproj"))
        path += ".plotproj";

    bool ok = ProjectManager::save(m_project, path);
    if (ok) {
        m_project.filePath = path;
        if (!saveProtectedSnippetStore(m_project.filePath, m_protectedSnippetsByDocument)) {
            QMessageBox::warning(this, "保存エラー", "保護ブロック設定の保存に失敗しました。");
            ok = false;
        }
    }
    if (ok) {
        markAllTabsClean();
        addRecentProject(path);
        recordAllEpisodeRevisions(static_cast<int>(PlotEngine::Revisions::RevisionType::ManualEdit), QStringLiteral("プロジェクト保存"));
    }
    updateStatusBar();
    refreshRevisionHistoryPanel();
    return ok;
}

void MainWindow::onEpisodeSelected(const QString &chapterId, const QString &episodeId)
{
    const Chapter *chapter = PlotEngine::Docs::findChapterById(m_project, chapterId);
    const Episode *episode = PlotEngine::Docs::findEpisodeById(m_project, chapterId, episodeId);
    if (!chapter || !episode)
        return;

    const QString tabTitle = chapter->title + " / " + episode->title;
    for (int i = 0; i < m_editorTabs->count(); ++i) {
        auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->widget(i));
        if (editor && editor->sceneId() == episodeId) {
            m_editorTabs->setCurrentIndex(i);
            refreshRevisionHistoryPanel();
            refreshSceneBoardPanel();
            return;
        }
    }

    auto *editor = new NovelEditor(episodeId);
    editor->setProperty("documentKind", "episode");
    editor->setProperty("documentId", episodeId);
    editor->setProperty("chapterId", chapterId);
    editor->setProperty("baseTabTitle", tabTitle);
    editor->setProperty("isDirty", false);
    editor->setContent(episode->content);
    int idx = m_editorTabs->addTab(editor, tabTitle);
    m_editorTabs->setCurrentIndex(idx);

    connect(editor, &NovelEditor::contentChanged, this,
        [this, editor, chapterId, episodeId](const QString &text) {
            if (Episode *updatedEpisode = PlotEngine::Docs::findEpisodeById(m_project, chapterId, episodeId)) {
                updatedEpisode->content = text;
                updatedEpisode->modifiedAt = QDateTime::currentDateTime();
            }
            markDocumentDirty(editor);
            updateStatusBar();
            refreshRevisionHistoryPanel();
        });
    connect(editor, &NovelEditor::protectedSnippetsEdited, this,
        [this, editor]() {
            syncProtectedSnippetsFromEditor(editor);
            refreshProtectedSnippetPanel();
            markDocumentDirty(editor);
            updateStatusBar();
        });
    connect(editor, &QPlainTextEdit::cursorPositionChanged,
            this, &MainWindow::updateCursorStatus);
    refreshExplorerOpenDocuments();
    updateCursorStatus();
    updateBreadcrumbStatus();
    refreshRevisionHistoryPanel();
    refreshSceneBoardPanel();
}

void MainWindow::onCharacterSelected(const QString &characterId)
{
    const CharacterEntry *character = PlotEngine::Docs::findCharacterById(m_project, characterId);
    if (!character)
        return;

    const QString tabTitle = "キャラ: " + character->name;
    for (int i = 0; i < m_editorTabs->count(); ++i) {
        auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->widget(i));
        if (editor && editor->sceneId() == characterId) {
            m_editorTabs->setCurrentIndex(i);
            refreshRevisionHistoryPanel();
            return;
        }
    }

    auto *editor = new NovelEditor(characterId);
    editor->setProperty("documentKind", "character");
    editor->setProperty("documentId", characterId);
    editor->setProperty("baseTabTitle", tabTitle);
    editor->setProperty("isDirty", false);
    editor->setContent(character->notes);
    int idx = m_editorTabs->addTab(editor, tabTitle);
    m_editorTabs->setCurrentIndex(idx);

    connect(editor, &NovelEditor::contentChanged, this,
        [this, editor, characterId](const QString &text) {
            if (CharacterEntry *characterEntry = PlotEngine::Docs::findCharacterById(m_project, characterId))
                characterEntry->notes = text;
            markDocumentDirty(editor);
            updateStatusBar();
        });
    connect(editor, &NovelEditor::protectedSnippetsEdited, this,
        [this, editor]() {
            syncProtectedSnippetsFromEditor(editor);
            refreshProtectedSnippetPanel();
            markDocumentDirty(editor);
            updateStatusBar();
        });
    connect(editor, &QPlainTextEdit::cursorPositionChanged,
            this, &MainWindow::updateCursorStatus);
    refreshExplorerOpenDocuments();
    updateCursorStatus();
    updateBreadcrumbStatus();
    refreshRevisionHistoryPanel();
    refreshSceneBoardPanel();
}

void MainWindow::onLocationSelected(const QString &locationId)
{
    const LocationEntry *location = PlotEngine::Docs::findLocationById(m_project, locationId);
    if (!location)
        return;

    const QString tabTitle = "場所: " + location->name;
    for (int i = 0; i < m_editorTabs->count(); ++i) {
        auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->widget(i));
        if (editor && editor->sceneId() == locationId) {
            m_editorTabs->setCurrentIndex(i);
            refreshRevisionHistoryPanel();
            return;
        }
    }

    auto *editor = new NovelEditor(locationId);
    editor->setProperty("documentKind", "location");
    editor->setProperty("documentId", locationId);
    editor->setProperty("baseTabTitle", tabTitle);
    editor->setProperty("isDirty", false);
    editor->setContent(location->notes);
    int idx = m_editorTabs->addTab(editor, tabTitle);
    m_editorTabs->setCurrentIndex(idx);

    connect(editor, &NovelEditor::contentChanged, this,
        [this, editor, locationId](const QString &text) {
            if (LocationEntry *locationEntry = PlotEngine::Docs::findLocationById(m_project, locationId))
                locationEntry->notes = text;
            markDocumentDirty(editor);
            updateStatusBar();
        });
    connect(editor, &NovelEditor::protectedSnippetsEdited, this,
        [this, editor]() {
            syncProtectedSnippetsFromEditor(editor);
            refreshProtectedSnippetPanel();
            markDocumentDirty(editor);
            updateStatusBar();
        });
    connect(editor, &QPlainTextEdit::cursorPositionChanged,
            this, &MainWindow::updateCursorStatus);
    refreshExplorerOpenDocuments();
    updateCursorStatus();
    updateBreadcrumbStatus();
    refreshRevisionHistoryPanel();
}

void MainWindow::updateStatusBar()
{
    if (m_project.filePath.isEmpty())
        m_statusFile->setText(m_project.name + " (未保存)");
    else
        m_statusFile->setText(m_project.name + " - " + m_project.filePath);

    const QString windowLabel = m_project.name.isEmpty() ? QStringLiteral("PlotEngine") : m_project.name;
    setWindowTitle(m_dirty ? windowLabel + QStringLiteral(" * - PlotEngine") : windowLabel + QStringLiteral(" - PlotEngine"));

    m_statusWords->setText("文字数: " + QString::number(m_project.totalWordCount()));
    updateCursorStatus();
    updateBreadcrumbStatus();
}

void MainWindow::updateCursorStatus()
{
    auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());
    if (!editor) {
        m_statusCursor->setText("行: 1 列: 1");
        return;
    }

    QTextCursor cursor = editor->textCursor();
    int line = cursor.blockNumber() + 1;
    int column = cursor.positionInBlock() + 1;
    m_statusCursor->setText(QString("行: %1 列: %2").arg(line).arg(column));
}

void MainWindow::refreshTabTitle(QWidget *widget)
{
    if (!widget) return;
    const QString base = widget->property("baseTabTitle").toString();
    if (base.isEmpty()) return;

    const bool dirty = widget->property("isDirty").toBool();
    const int index = m_editorTabs->indexOf(widget);
    if (index >= 0) {
        m_editorTabs->setTabText(index, dirty ? base + " *" : base);
        m_editorTabs->setTabToolTip(index, dirty ? base + " (未保存)" : base);
    }
}

QString MainWindow::baseTabTitle(QWidget *widget) const
{
    if (!widget) return QString();
    return widget->property("baseTabTitle").toString();
}

void MainWindow::updateTabDecorations()
{
    for (int i = 0; i < m_editorTabs->count(); ++i)
        refreshTabTitle(m_editorTabs->widget(i));
    refreshExplorerOpenDocuments();
    updateBreadcrumbStatus();
    refreshRevisionHistoryPanel();
    refreshWorkspaceView();
}

void MainWindow::refreshProjectViews()
{
    if (m_structurePanel)
        m_structurePanel->loadProject(m_project);
    if (m_notePanel)
        m_notePanel->loadProject(m_project);
    if (m_searchPanel)
        m_searchPanel->loadProject(m_project);
    refreshSceneBoardPanel();
    syncOpenDocumentTabs();
    updateStatusBar();
    refreshRevisionHistoryPanel();
    refreshProtectedSnippetPanel();
    refreshWorkspaceView();
}

void MainWindow::refreshWorkspaceView()
{
    if (!m_centerStack || !m_welcomePage || !m_editorTabs)
        return;

    updateWelcomeState();
    if (m_editorTabs->count() > 0)
        showEditorWorkspace();
    else
        showWelcomePage();
}

void MainWindow::showWelcomePage()
{
    if (!m_centerStack || !m_welcomePage)
        return;
    m_centerStack->setCurrentWidget(m_welcomePage);
}

void MainWindow::showEditorWorkspace()
{
    if (!m_centerStack || !m_editorTabs)
        return;
    m_centerStack->setCurrentWidget(m_editorTabs);
}

void MainWindow::updateWelcomeState()
{
    if (m_welcomeChapterButton)
        m_welcomeChapterButton->setEnabled(!m_project.name.isEmpty() || !m_project.filePath.isEmpty());
    if (m_welcomeEpisodeButton)
        m_welcomeEpisodeButton->setEnabled(!m_project.chapters.isEmpty());
}

void MainWindow::openFirstEpisodeIfAny()
{
    if (m_project.chapters.isEmpty()) {
        refreshWorkspaceView();
        return;
    }

    const auto &chapter = m_project.chapters.first();
    if (chapter.episodes.isEmpty()) {
        refreshWorkspaceView();
        return;
    }

    onEpisodeSelected(chapter.id, chapter.episodes.first().id);
    showEditorWorkspace();
}

void MainWindow::refreshExplorerOpenDocuments()
{
    if (!m_structurePanel || !m_editorTabs)
        return;

    QVector<StructurePanel::OpenDocumentEntry> documents;
    documents.reserve(m_editorTabs->count());

    for (int i = 0; i < m_editorTabs->count(); ++i) {
        auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->widget(i));
        if (!editor)
            continue;

        const QString kind = editor->property("documentKind").toString();
        if (kind != "episode" && kind != "character" && kind != "location")
            continue;

        const QString title = editor->property("baseTabTitle").toString();
        const bool dirty = editor->property("isDirty").toBool();
        const QString detail =
            kind == "episode" ? QStringLiteral("エピソード") :
            kind == "character" ? QStringLiteral("キャラクター") :
            QStringLiteral("ロケーション");

        documents.append({
            kind,
            editor->property("documentId").toString(),
            title,
            detail,
            dirty
        });
    }

    m_structurePanel->setOpenDocuments(documents);

    auto *currentEditor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());
    if (!currentEditor) {
        m_structurePanel->setCurrentDocument(StructurePanel::OpenDocumentEntry{});
        return;
    }

    const QString currentKind = currentEditor->property("documentKind").toString();
    const QString currentId = currentEditor->property("documentId").toString();
    const QString currentTitle = currentEditor->property("baseTabTitle").toString();
    const bool currentDirty = currentEditor->property("isDirty").toBool();
    if (!currentKind.isEmpty() && !currentId.isEmpty()) {
        StructurePanel::OpenDocumentEntry currentEntry;
        currentEntry.kind = currentKind;
        currentEntry.id = currentId;
        currentEntry.title = currentTitle;
        currentEntry.detail =
            currentKind == "episode" ? QStringLiteral("エピソード") :
            currentKind == "character" ? QStringLiteral("キャラクター") :
            QStringLiteral("ロケーション");
        currentEntry.dirty = currentDirty;
        m_structurePanel->setCurrentDocument(currentEntry);
        m_structurePanel->selectOpenDocument(currentKind, currentId);
        return;
    }

    m_structurePanel->setCurrentDocument(StructurePanel::OpenDocumentEntry{});
}

void MainWindow::syncOpenDocumentTabs()
{
    if (!m_editorTabs)
        return;

    for (int i = 0; i < m_editorTabs->count(); ++i) {
        auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->widget(i));
        if (!editor)
            continue;

        const QString kind = editor->property("documentKind").toString();
        const QString id = editor->property("documentId").toString();
        if (kind.isEmpty() || id.isEmpty())
            continue;

        const QString title = PlotEngine::Docs::documentTitleForProject(m_project, kind, id);
        if (!title.isEmpty()) {
            editor->setProperty("baseTabTitle", title);
            refreshTabTitle(editor);
        }

        if (kind == "episode") {
            const QString chapterId = editor->property("chapterId").toString();
            const Episode *episode = !chapterId.isEmpty()
                ? PlotEngine::Docs::findEpisodeById(m_project, chapterId, id)
                : PlotEngine::Docs::findEpisodeById(m_project, id);
            if (episode) {
                if (editor->toPlainText() != episode->content)
                    editor->setContent(episode->content);
            }
        } else if (kind == "character") {
            if (const CharacterEntry *character = findCharacterById(m_project, id)) {
                if (editor->toPlainText() != character->notes)
                    editor->setContent(character->notes);
            }
        } else if (kind == "location") {
            if (const LocationEntry *location = findLocationById(m_project, id)) {
                if (editor->toPlainText() != location->notes)
                    editor->setContent(location->notes);
            }
        }
    }

    if (m_structurePanel)
        m_structurePanel->loadProject(m_project);
    if (m_notePanel)
        m_notePanel->loadProject(m_project);
    if (m_searchPanel)
        m_searchPanel->loadProject(m_project);

    updateTabDecorations();
    updateStatusBar();
}

void MainWindow::closeOpenDocument(const QString &kind, const QString &id)
{
    for (int i = 0; i < m_editorTabs->count(); ++i) {
        auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->widget(i));
        if (!editor)
            continue;
        if (editor->property("documentKind").toString() != kind)
            continue;
        if (editor->property("documentId").toString() != id)
            continue;

        if (!promptCloseDocument(editor))
            return;
        auto *tab = m_editorTabs->widget(i);
        m_editorTabs->removeTab(i);
        if (tab)
            tab->deleteLater();
        refreshExplorerOpenDocuments();
        updateCursorStatus();
        updateBreadcrumbStatus();
        return;
    }
}

void MainWindow::closeOtherOpenDocuments(const QString &kind, const QString &id)
{
    for (int i = m_editorTabs->count() - 1; i >= 0; --i) {
        auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->widget(i));
        if (!editor)
            continue;
        const QString docKind = editor->property("documentKind").toString();
        const QString docId = editor->property("documentId").toString();
        if (docKind == kind && docId == id)
            continue;

        if (!promptCloseDocument(editor))
            return;

        auto *tab = m_editorTabs->widget(i);
        m_editorTabs->removeTab(i);
        if (tab)
            tab->deleteLater();
    }

    refreshExplorerOpenDocuments();
    updateCursorStatus();
    updateBreadcrumbStatus();
}

void MainWindow::markDocumentDirty(QWidget *widget)
{
    if (widget)
        widget->setProperty("isDirty", true);
    m_dirty = true;
    if (widget)
        refreshTabTitle(widget);
    refreshExplorerOpenDocuments();
    updateBreadcrumbStatus();
    refreshWorkspaceView();
}

void MainWindow::markAllTabsDirty()
{
    for (int i = 0; i < m_editorTabs->count(); ++i) {
        QWidget *tab = m_editorTabs->widget(i);
        tab->setProperty("isDirty", true);
    }
    m_dirty = true;
    updateTabDecorations();
}

void MainWindow::markAllTabsClean()
{
    for (int i = 0; i < m_editorTabs->count(); ++i) {
        QWidget *tab = m_editorTabs->widget(i);
        tab->setProperty("isDirty", false);
    }
    m_dirty = false;
    updateTabDecorations();
}

QString MainWindow::currentBreadcrumbText() const
{
    auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());
    if (!editor)
        return "エクスプローラ";

    const QString kind = editor->property("documentKind").toString();
    const QString id = editor->property("documentId").toString();

    if (kind == "episode") {
        for (const auto &ch : m_project.chapters) {
            for (const auto &ep : ch.episodes) {
                if (ep.id == id)
                    return ch.title + " / " + ep.title;
            }
        }
    } else if (kind == "character") {
        for (const auto &c : m_project.characters) {
            if (c.id == id)
                return "キャラ / " + c.name;
        }
    } else if (kind == "location") {
        for (const auto &l : m_project.locations) {
            if (l.id == id)
                return "場所 / " + l.name;
        }
    }

    const QString base = baseTabTitle(editor);
    return base.isEmpty() ? "エディタ" : base;
}

void MainWindow::updateBreadcrumbStatus()
{
    if (m_statusBreadcrumb)
        m_statusBreadcrumb->setText(currentBreadcrumbText());
}

void MainWindow::storeAiEditSnapshot(const QString &label)
{
    m_aiEditSnapshot = m_project;
    m_aiEditSnapshotDirty = m_dirty;
    m_aiEditSnapshotLabel = label;
    m_hasAiEditSnapshot = true;
}

void MainWindow::clearAiEditSnapshot()
{
    m_hasAiEditSnapshot = false;
    m_aiEditSnapshotDirty = false;
    m_aiEditSnapshot = NovelProject{};
    m_aiEditSnapshotLabel.clear();
}

void MainWindow::refreshRevisionHistoryPanel()
{
    if (!m_revisionHistoryPanel || !m_editorTabs)
        return;

    auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());
    if (!editor) {
        m_revisionHistoryPanel->setEmptyState(QStringLiteral("エピソードを開くと履歴を表示します。"));
        return;
    }

    if (editor->property("documentKind").toString() != "episode") {
        m_revisionHistoryPanel->setEmptyState(QStringLiteral("本文エピソードを選ぶと履歴を表示します。"));
        return;
    }

    const QString episodeId = editor->property("documentId").toString();
    if (episodeId.isEmpty()) {
        m_revisionHistoryPanel->setEmptyState(QStringLiteral("エピソード ID を取得できません。"));
        return;
    }

    QVector<RevisionHistoryPanel::Entry> entries;
    entries.append({
        QString(),
        QStringLiteral("現在のドラフト"),
        editor->property("isDirty").toBool() ? QStringLiteral("未保存") : QStringLiteral("現在"),
        editor->toPlainText(),
        QStringLiteral("現在のドラフトです。"),
        QString(),
        QString(),
        QString(),
        QString(),
        true
    });

    const QString currentContent = editor->toPlainText();
    if (!m_project.filePath.isEmpty()) {
        const auto revisions = PlotEngine::RevisionStore::loadEpisodeHistory(m_project.filePath, episodeId);
        for (auto it = revisions.crbegin(); it != revisions.crend(); ++it) {
            const auto &revision = *it;
            const QString detail = QStringLiteral("%1 | %2 字")
                .arg(PlotEngine::Revisions::revisionTypeToString(revision.type))
                .arg(revision.wordCount);
            QString content = revision.content;
            if (!revision.notes.trimmed().isEmpty())
                content = QStringLiteral("[notes] %1\n\n%2").arg(revision.notes.trimmed(), revision.content);
            const QString diffSummary = PlotEngine::AI::buildRevisionDiffSummary(revision.content, currentContent);
            entries.append({
                revision.id,
                revision.timestamp.isValid()
                    ? revision.timestamp.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))
                    : QStringLiteral("timestamp unavailable"),
                detail,
                content,
                diffSummary,
                QStringLiteral("現在のドラフト"),
                currentContent,
                revision.timestamp.isValid()
                    ? revision.timestamp.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))
                    : QStringLiteral("保存済み履歴"),
                revision.content,
                false
            });
        }
    }

    m_revisionHistoryPanel->setEntries(entries);
}

QString MainWindow::protectionKey(const QString &kind, const QString &id) const
{
    return kind + QLatin1Char(':') + id;
}

QString MainWindow::currentDocumentProtectionKey() const
{
    if (!m_editorTabs)
        return QString();

    auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());
    if (!editor)
        return QString();

    const QString kind = editor->property("documentKind").toString();
    const QString id = editor->property("documentId").toString();
    if (kind.isEmpty() || id.isEmpty())
        return QString();

    return protectionKey(kind, id);
}

void MainWindow::refreshProtectedSnippetPanel()
{
    if (!m_protectedSnippetPanel || !m_editorTabs)
        return;

    auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());
    if (!editor) {
        m_protectedSnippetPanel->clearPanel();
        return;
    }

    const QString kind = editor->property("documentKind").toString();
    const QString id = editor->property("documentId").toString();
    const QString title = editor->property("baseTabTitle").toString();
    if (kind.isEmpty() || id.isEmpty()) {
        m_protectedSnippetPanel->clearPanel();
        return;
    }

    m_protectedSnippetPanel->setDocumentTitle(title);

    const auto records = m_protectedSnippetsByDocument.value(protectionKey(kind, id));
    QVector<ProtectedSnippetPanel::Snippet> snippets;
    QVector<NovelEditor::ProtectedSnippet> protectedSnippets;
    snippets.reserve(records.size());
    protectedSnippets.reserve(records.size());
    for (const auto &record : records) {
        snippets.append({record.label, record.text});
        protectedSnippets.append({record.label, record.text, record.start, record.length});
    }
    m_protectedSnippetPanel->setSnippets(snippets);
    editor->setProtectedSnippets(protectedSnippets);
}

void MainWindow::refreshSceneBoardPanel()
{
    if (!m_sceneBoardPanel)
        return;

    m_sceneBoardPanel->loadProject(m_project);

    if (!m_editorTabs)
        return;

    auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());
    if (!editor)
        return;

    const QString kind = editor->property("documentKind").toString();
    if (kind != "episode")
        return;

    const QString episodeId = editor->property("documentId").toString();
    const QString chapterId = editor->property("chapterId").toString();
    if (!chapterId.isEmpty() && !episodeId.isEmpty())
        m_sceneBoardPanel->setCurrentEpisode(chapterId, episodeId);
}

void MainWindow::addProtectedSnippetFromSelection()
{
    auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());
    if (!editor) {
        QMessageBox::information(this, "保護ブロック", "本文エディタを開いてください。");
        return;
    }

    const QTextCursor selection = editor->textCursor();
    QString selectedText = selection.selectedText();
    selectedText.replace(QChar(0x2029), '\n');
    selectedText.replace(QChar(0x2028), '\n');
    if (selectedText.trimmed().isEmpty()) {
        QMessageBox::information(this, "保護ブロック", "追加するには本文を選択してください。");
        return;
    }

    bool ok = false;
    const QString defaultLabel = QStringLiteral("選択範囲 %1")
        .arg(m_protectedSnippetsByDocument.value(currentDocumentProtectionKey()).size() + 1);
    QString label = QInputDialog::getText(
        this,
        "保護ブロック名",
        "ラベル:",
        QLineEdit::Normal,
        defaultLabel,
        &ok).trimmed();
    if (!ok)
        return;
    if (label.isEmpty())
        label = defaultLabel;

    const QString key = currentDocumentProtectionKey();
    if (key.isEmpty())
        return;

    m_protectedSnippetsByDocument[key].append({
        label,
        selectedText,
        selection.selectionStart(),
        selection.selectionEnd() - selection.selectionStart()
    });
    refreshProtectedSnippetPanel();
    syncProtectedSnippetsFromEditor(editor);
    markDocumentDirty(editor);
    updateStatusBar();
    showDockPane(m_protectedSnippetDock);
}

void MainWindow::removeProtectedSnippet(int index)
{
    const QString key = currentDocumentProtectionKey();
    if (key.isEmpty() || !m_protectedSnippetsByDocument.contains(key))
        return;
    auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());

    auto &records = m_protectedSnippetsByDocument[key];
    if (index < 0 || index >= records.size())
        return;

    records.removeAt(index);
    if (records.isEmpty())
        m_protectedSnippetsByDocument.remove(key);
    refreshProtectedSnippetPanel();
    markDocumentDirty(editor);
    updateStatusBar();
}

void MainWindow::clearProtectedSnippets()
{
    const QString key = currentDocumentProtectionKey();
    if (key.isEmpty())
        return;
    auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());

    m_protectedSnippetsByDocument.remove(key);
    refreshProtectedSnippetPanel();
    markDocumentDirty(editor);
    updateStatusBar();
}

void MainWindow::syncProtectedSnippetsFromEditor(NovelEditor *editor)
{
    if (!editor)
        return;

    const QString kind = editor->property("documentKind").toString();
    const QString id = editor->property("documentId").toString();
    if (kind.isEmpty() || id.isEmpty())
        return;

    QVector<ProtectedSnippetRecord> records;
    const auto snippets = editor->protectedSnippets();
    records.reserve(snippets.size());
    for (const auto &snippet : snippets)
        records.append({snippet.label, snippet.text, snippet.start, snippet.length});

    const QString key = protectionKey(kind, id);
    if (records.isEmpty())
        m_protectedSnippetsByDocument.remove(key);
    else
        m_protectedSnippetsByDocument.insert(key, records);
}

void MainWindow::recordEpisodeRevision(const QString &episodeId, const QString &content,
                                       int type,
                                       const QString &notes, const QString &aiModel)
{
    if (m_project.filePath.isEmpty() || episodeId.isEmpty())
        return;

    PlotEngine::RevisionStore::appendRevision(
        m_project.filePath,
        episodeId,
        content,
        static_cast<PlotEngine::Revisions::RevisionType>(type),
        notes,
        aiModel);
}

void MainWindow::recordAllEpisodeRevisions(int type, const QString &notes)
{
    if (m_project.filePath.isEmpty())
        return;

    for (const auto &chapter : m_project.chapters) {
        for (const auto &episode : chapter.episodes)
            recordEpisodeRevision(episode.id, episode.content, type, notes);
    }
}

void MainWindow::recordPlanAffectedEpisodeRevisions(const PlotEngine::AI::EditPlan &plan,
                                                    int type,
                                                    const QString &notes,
                                                    const QString &aiModel)
{
    if (m_project.filePath.isEmpty())
        return;

    std::set<QString> touchedEpisodes;
    for (const auto &action : plan.actions) {
        if (action.episodeId.isEmpty())
            continue;
        touchedEpisodes.insert(action.episodeId);
    }

    for (const auto &episodeId : touchedEpisodes) {
        if (const Episode *episode = PlotEngine::Docs::findEpisodeById(m_project, episodeId))
            recordEpisodeRevision(episodeId, episode->content, type, notes, aiModel);
    }
}

void MainWindow::rollbackLastAiEdit()
{
    if (!m_hasAiEditSnapshot) {
        QMessageBox::information(this, "AI 編集の復元", "戻せる直前の AI 編集がありません。");
        return;
    }

    const auto result = QMessageBox::question(
        this,
        "AI 編集の復元",
        m_aiEditSnapshotLabel.isEmpty()
            ? "直前の AI 編集を元に戻しますか?"
            : QString("「%1」を元に戻しますか?").arg(m_aiEditSnapshotLabel),
        QMessageBox::Yes | QMessageBox::No);
    if (result != QMessageBox::Yes)
        return;

    const NovelProject beforeRollback = m_project;
    const bool snapshotDirty = m_aiEditSnapshotDirty;
    m_project = m_aiEditSnapshot;
    clearAiEditSnapshot();
    refreshProjectViews();
    syncOpenDocumentTabs();
    if (snapshotDirty) {
        m_dirty = true;
        markAllTabsDirty();
    } else {
        m_dirty = false;
        markAllTabsClean();
    }
    updateStatusBar();
    discardPendingAiReview();

    for (const auto &chapter : m_project.chapters) {
        for (const auto &episode : chapter.episodes) {
            const Episode *beforeEpisode = PlotEngine::Docs::findEpisodeById(beforeRollback, episode.id);
            if (!beforeEpisode || beforeEpisode->content == episode.content)
                continue;
            recordEpisodeRevision(episode.id, episode.content, static_cast<int>(PlotEngine::Revisions::RevisionType::Rollback),
                                  QStringLiteral("直前の AI 編集を復元"));
        }
    }
    refreshRevisionHistoryPanel();

    QMessageBox::information(this, "AI 編集の復元", "直前の AI 編集を戻しました。");
}

void MainWindow::openQuickOpenResult(const QString &kind, const QString &id)
{
    openDocumentByKind(kind, id);
}

void MainWindow::openSearchResult(const QString &kind, const QString &id, const QString &query)
{
    if (!openDocumentByKind(kind, id))
        return;

    auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());
    if (!editor || query.trimmed().isEmpty())
        return;

    editor->setSearchText(query.trimmed());
    editor->showSearchBar();
    QTextCursor cursor = editor->textCursor();
    cursor.movePosition(QTextCursor::Start);
    editor->setTextCursor(cursor);
    editor->findNext();
}

void MainWindow::applyPendingAiReview()
{
    if (!m_reviewPanel) {
        return;
    }

    QString parseError;
    const auto plan = PlotEngine::AI::parseEditPlan(m_reviewPanel->rawResponse(), &parseError);
    if (!plan) {
        QMessageBox::warning(this, "AI レビュー", parseError.isEmpty()
            ? QStringLiteral("レビュー対象の編集計画を再構築できませんでした。")
            : parseError);
        return;
    }

    const QVector<int> selectedIndices = m_reviewPanel->selectedActionIndices();
    if (selectedIndices.isEmpty()) {
        QMessageBox::information(this, "AI レビュー", "適用するアクションを 1 件以上選択してください。");
        return;
    }

    PlotEngine::AI::EditPlan selectedPlan;
    selectedPlan.rationale = plan->rationale;
    for (const int index : selectedIndices) {
        if (index >= 0 && index < plan->actions.size())
            selectedPlan.actions.append(plan->actions.at(index));
    }

    if (selectedPlan.actions.isEmpty()) {
        QMessageBox::warning(this, "AI レビュー", "選択されたアクションを適用できませんでした。");
        return;
    }

    const QString chapterId = m_pendingReviewChapterId;
    const QString episodeId = m_pendingReviewEpisodeId;
    const QString currentText = m_pendingReviewCurrentText;
    const QString instruction = m_pendingReviewInstruction;
    const QString protectedText = m_pendingReviewProtectedText;
    const QString aiModel = m_pendingReviewAiModel;

    bool touchedCurrentEpisode = false;
    for (const auto &action : selectedPlan.actions) {
        if (action.chapterId != chapterId || action.episodeId != episodeId)
            continue;
        if (action.type == PlotEngine::AI::EditActionType::UpdateEpisodeContent
            || action.type == PlotEngine::AI::EditActionType::UpdateEpisodeMetadata
            || action.type == PlotEngine::AI::EditActionType::RenameEpisode) {
            touchedCurrentEpisode = true;
            break;
        }
    }

    storeAiEditSnapshot(QStringLiteral("Venice の直接編集"));
    const QStringList appliedSummary = PlotEngine::AI::applyEditPlan(m_project, selectedPlan);
    if (appliedSummary.isEmpty()) {
        clearAiEditSnapshot();
        QMessageBox::warning(this, "AI レビュー", "編集計画の適用に失敗しました。");
        return;
    }

    if (Episode *updatedEpisode = PlotEngine::Docs::findEpisodeById(m_project, chapterId, episodeId)) {
        if (touchedCurrentEpisode) {
            updatedEpisode->revisionNotes = instruction.trimmed();
            if (!selectedPlan.rationale.trimmed().isEmpty())
                updatedEpisode->revisionNotes += "\n[AI] " + selectedPlan.rationale.trimmed();
            if (!protectedText.isEmpty())
                updatedEpisode->revisionNotes += "\n[保護] " + protectedText.left(120);
            updatedEpisode->modifiedAt = QDateTime::currentDateTime();
        }
    }

    markAllTabsDirty();
    syncOpenDocumentTabs();
    recordPlanAffectedEpisodeRevisions(selectedPlan, static_cast<int>(PlotEngine::Revisions::RevisionType::AiPolished),
                                       instruction.trimmed(), aiModel);
    refreshProjectViews();
    refreshRevisionHistoryPanel();

    QString details = appliedSummary.join("\n");
    if (const Episode *updatedEpisode = PlotEngine::Docs::findEpisodeById(m_project, chapterId, episodeId)) {
        const QString diff = PlotEngine::AI::buildRevisionDiffSummary(currentText, updatedEpisode->content);
        if (!diff.trimmed().isEmpty()) {
            if (!details.isEmpty())
                details += "\n\n";
            details += "差分の概要:\n" + diff;
        }
    }

    discardPendingAiReview();

    QMessageBox box(this);
    box.setWindowTitle("AI レビュー");
    box.setText("選択した編集を適用しました。");
    if (!selectedPlan.rationale.trimmed().isEmpty())
        box.setInformativeText("意図: " + selectedPlan.rationale.trimmed());
    if (!details.trimmed().isEmpty())
        box.setDetailedText(details.trimmed());
    box.exec();
}

void MainWindow::discardPendingAiReview()
{
    m_pendingReviewChapterId.clear();
    m_pendingReviewEpisodeId.clear();
    m_pendingReviewInstruction.clear();
    m_pendingReviewProtectedText.clear();
    m_pendingReviewCurrentText.clear();
    m_pendingReviewAiModel.clear();
    if (m_reviewPanel)
        m_reviewPanel->clearReview();
}

void MainWindow::restoreRevisionFromHistory(const QString &revisionId)
{
    if (revisionId.isEmpty() || m_project.filePath.isEmpty())
        return;

    auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());
    if (!editor || editor->property("documentKind").toString() != "episode")
        return;

    const QString episodeId = editor->property("documentId").toString();
    const QString chapterId = editor->property("chapterId").toString();
    const auto revisions = PlotEngine::RevisionStore::loadEpisodeHistory(m_project.filePath, episodeId);

    for (const auto &revision : revisions) {
        if (revision.id != revisionId)
            continue;

        if (QMessageBox::question(this, "リビジョン復元",
                QStringLiteral("選択した履歴 (%1) に戻しますか?").arg(revision.timestamp.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))),
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
            return;
        }

        if (Episode *episode = PlotEngine::Docs::findEpisodeById(m_project, chapterId, episodeId)) {
            episode->content = revision.content;
            episode->modifiedAt = QDateTime::currentDateTime();
            episode->revisionNotes = QStringLiteral("履歴から復元: %1").arg(revision.timestamp.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
        }

        editor->setContent(revision.content);
        markDocumentDirty(editor);
        updateStatusBar();
        recordEpisodeRevision(episodeId, revision.content, static_cast<int>(PlotEngine::Revisions::RevisionType::Rollback),
                              QStringLiteral("履歴パネルから復元"));
        refreshRevisionHistoryPanel();
        return;
    }
}

bool MainWindow::openDocumentByKind(const QString &kind, const QString &id)
{
    if (kind == "episode") {
        for (const auto &ch : m_project.chapters) {
            for (const auto &ep : ch.episodes) {
                if (ep.id == id) {
                    m_structurePanel->selectEpisode(ch.id, ep.id);
                    onEpisodeSelected(ch.id, ep.id);
                    return true;
                }
            }
        }
    } else if (kind == "chapter") {
        if (PlotEngine::Docs::findChapterById(m_project, id)) {
            m_structurePanel->selectChapter(id);
            return true;
        }
    } else if (kind == "character") {
        if (PlotEngine::Docs::findCharacterById(m_project, id)) {
            onCharacterSelected(id);
            return true;
        }
    } else if (kind == "location") {
        if (PlotEngine::Docs::findLocationById(m_project, id)) {
            onLocationSelected(id);
            return true;
        }
    }

    return false;
}

void MainWindow::addChapter()
{
    QString episodeId;
    const QString chapterId = PlotEngine::Docs::addChapter(m_project, &episodeId);
    if (chapterId.isEmpty() || episodeId.isEmpty())
        return;

    markAllTabsDirty();
    refreshProjectViews();
    m_structurePanel->selectEpisode(chapterId, episodeId);
    onEpisodeSelected(chapterId, episodeId);
}

void MainWindow::addEpisode(const QString &chapterId)
{
    const QString createdEpisodeId = PlotEngine::Docs::addEpisode(m_project, chapterId);
    if (createdEpisodeId.isEmpty())
        return;

    markAllTabsDirty();
    refreshProjectViews();
    m_structurePanel->selectEpisode(chapterId, createdEpisodeId);
    onEpisodeSelected(chapterId, createdEpisodeId);
}

void MainWindow::renameChapter(const QString &chapterId, const QString &title)
{
    if (!PlotEngine::Docs::renameChapter(m_project, chapterId, title)) {
        m_structurePanel->loadProject(m_project);
        return;
    }

    if (const Chapter *chapter = PlotEngine::Docs::findChapterById(m_project, chapterId)) {
        for (int i = 0; i < m_editorTabs->count(); ++i) {
            auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->widget(i));
            if (!editor)
                continue;
            for (const auto &episode : chapter->episodes) {
                if (episode.id != editor->sceneId())
                    continue;
                editor->setProperty("baseTabTitle", chapter->title + " / " + episode.title);
                refreshTabTitle(editor);
                break;
            }
        }
    }

    markAllTabsDirty();
    refreshProjectViews();
    m_structurePanel->selectChapter(chapterId);
}

void MainWindow::renameEpisode(const QString &chapterId, const QString &episodeId, const QString &title)
{
    if (!PlotEngine::Docs::renameEpisode(m_project, chapterId, episodeId, title)) {
        m_structurePanel->loadProject(m_project);
        return;
    }

    if (const Chapter *chapter = PlotEngine::Docs::findChapterById(m_project, chapterId)) {
        if (const Episode *episode = PlotEngine::Docs::findEpisodeById(m_project, chapterId, episodeId)) {
            for (int i = 0; i < m_editorTabs->count(); ++i) {
                auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->widget(i));
                if (editor && editor->sceneId() == episodeId) {
                    editor->setProperty("baseTabTitle", chapter->title + " / " + episode->title);
                    refreshTabTitle(editor);
                    break;
                }
            }
        }
    }

    markAllTabsDirty();
    refreshProjectViews();
    m_structurePanel->selectEpisode(chapterId, episodeId);
}

void MainWindow::polishCurrentEpisode()
{
    auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());
    if (!editor) {
        QMessageBox::information(this, "推敲", "推敲したいエピソードを開いてください。");
        return;
    }

    const QString episodeId = editor->sceneId();
    QString chapterId;
    Episode *episode = PlotEngine::Docs::findEpisodeById(m_project, episodeId, &chapterId);

    if (!episode) {
        QMessageBox::warning(this, "推敲", "開いているエピソードを特定できませんでした。");
        return;
    }

    QString currentText = editor->toPlainText();
    QString selectedText = editor->textCursor().selectedText();
    selectedText.replace(QChar(0x2029), '\n');
    selectedText.replace(QChar(0x2028), '\n');
    selectedText = selectedText.trimmed();
    const QString protectionDocKey = currentDocumentProtectionKey();
    const QVector<ProtectedSnippetRecord> protectedRecords = m_protectedSnippetsByDocument.value(protectionDocKey);
    QVector<PlotEngine::AI::ProtectedSnippet> protectedSnippets;
    protectedSnippets.reserve(protectedRecords.size() + (selectedText.isEmpty() ? 0 : 1));
    QStringList protectedLabels;
    for (const auto &record : protectedRecords) {
        if (record.text.trimmed().isEmpty())
            continue;
        protectedSnippets.append({record.label, record.text});
        protectedLabels.append(record.label.trimmed().isEmpty() ? QStringLiteral("保護ブロック") : record.label.trimmed());
    }
    if (!selectedText.isEmpty())
        protectedLabels.prepend(QStringLiteral("現在の選択範囲"));

    bool ok = false;
    QString instruction = QInputDialog::getMultiLineText(
        this,
        "変更したい内容",
        "AIに直してほしい内容:",
        QStringLiteral("文体を保ちながら、冗長な表現を削って読みやすくする"),
        &ok);
    if (!ok || instruction.trimmed().isEmpty())
        return;

    QString protectedText;
    if (!selectedText.isEmpty()) {
        const auto protectSelection = QMessageBox::question(
            this,
            "選択範囲を保護",
            "現在の選択範囲を今回の AI 編集でも保護しますか?",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes);
        if (protectSelection == QMessageBox::Yes) {
            protectedText = selectedText;
            protectedSnippets.append({QStringLiteral("選択範囲"), selectedText});
        }
    }

    if (!protectedLabels.isEmpty()) {
        QMessageBox::information(
            this,
            "保護ブロック",
            QStringLiteral("今回の推敲では次の保護ブロックを使用します:\n- %1")
                .arg(protectedLabels.join(QStringLiteral("\n- "))));
    }

    QString mode = QInputDialog::getItem(
        this,
        "推敲モード",
        "処理:",
        QStringList({"Veniceで生成して適用", "Veniceで直接編集", "整形して適用", "AI用プロンプトを作成"}),
        0,
        false,
        &ok);
    if (!ok || mode.isEmpty())
        return;

    if (mode == "AI用プロンプトを作成") {
        PlotEngine::AI::RevisionPromptRequest request;
        request.desiredChanges = instruction.trimmed();
        request.protectedSnippets = protectedSnippets;

        QString prompt = PlotEngine::AI::buildRevisionPrompt(
            m_project, chapterId, episodeId, currentText, request);
        qApp->clipboard()->setText(prompt);
        QMessageBox::information(this, "推敲", "AI用プロンプトをクリップボードにコピーしました。");
        return;
    }

    if (mode != "Veniceで生成して適用" && mode != "Veniceで直接編集") {
        QString polished = PlotEngine::Text::normalizeEpisodeText(currentText);
        editor->setContent(polished);
        episode->content = polished;
        episode->modifiedAt = QDateTime::currentDateTime();
        episode->revisionNotes = instruction.trimmed();
        if (!protectedText.isEmpty())
            episode->revisionNotes += "\n[保護] " + protectedText.left(120);
        markAllTabsDirty();
        syncOpenDocumentTabs();
        recordEpisodeRevision(episodeId, polished, static_cast<int>(PlotEngine::Revisions::RevisionType::ManualEdit), instruction.trimmed());
        refreshRevisionHistoryPanel();

        const QString diff = PlotEngine::AI::buildRevisionDiffSummary(currentText, polished);
        QMessageBox box(this);
        box.setWindowTitle("推敲");
        box.setText("軽い整形を適用しました。");
        box.setInformativeText("差分の概要:");
        box.setDetailedText(diff);
        box.exec();
        return;
    }

    QSettings aiSettings = PlotEngine::App::makeSettings();
    const QString storedVeniceKey = PlotEngine::AI::loadSecret("venice");
    const QString apiKey = !storedVeniceKey.isEmpty()
        ? storedVeniceKey
        : QProcessEnvironment::systemEnvironment().value("VENICE_API_KEY");
    const bool directEdit = mode == "Veniceで直接編集";
    if (apiKey.isEmpty() && directEdit) {
        QMessageBox::information(this, "推敲",
            "Venice 直接編集には API キーが必要です。");
        return;
    }

    if (!apiKey.isEmpty()) {
        VeniceProvider provider(apiKey, this);
        const QString model = aiSettings.value("ai/venice/model", "venice-uncensored").toString();
        const QString baseUrl = aiSettings.value("ai/venice/baseUrl", "https://api.venice.ai/api/v1").toString();
        provider.setBaseUrl(baseUrl);

    const QString prompt = directEdit
        ? PlotEngine::AI::buildDirectEditPrompt(
            m_project, chapterId, episodeId, currentText, instruction.trimmed(),
            protectedSnippets)
        : PlotEngine::AI::buildRevisionPrompt(
            m_project, chapterId, episodeId, currentText,
            PlotEngine::AI::RevisionPromptRequest{
                instruction.trimmed(),
                protectedSnippets
            });

    AiRequest aiRequest = AiRequest::forGeneration(
        directEdit
            ? "You are a structured edit engine for Japanese novels. Return JSON only and respect protected text."
            : "You are a professional Japanese novel editor. Preserve protected text and follow the requested changes.",
        prompt,
        model,
        directEdit ? 0.2 : 0.7,
        4096);

        QEventLoop loop;
        AiResponse response;
        bool responded = false;

        connect(&provider, &IAiProvider::responseReceived, &loop, [&](const AiResponse &r) {
            response = r;
            responded = true;
            loop.quit();
        });
        connect(&provider, &IAiProvider::error, &loop, [&](const QString &message) {
            response.success = false;
            response.error = message;
            responded = true;
            loop.quit();
        });

        provider.generate(aiRequest);
        loop.exec();

        if (!responded || !response.success || response.content.trimmed().isEmpty()) {
            QMessageBox::warning(this, "推敲", responded && !response.error.isEmpty()
                ? response.error
                : QStringLiteral("Venice から有効な応答を受け取れませんでした。"));
            return;
        }

        if (directEdit) {
            QString parseError;
            const auto plan = PlotEngine::AI::parseEditPlan(response.content, &parseError);
            if (!plan) {
                QMessageBox::warning(this, "推敲", parseError.isEmpty()
                    ? QStringLiteral("Venice の応答を編集計画として解釈できませんでした。")
                    : parseError);
                return;
            }

            NovelProject previewProject = m_project;
            const QStringList summary = PlotEngine::AI::applyEditPlan(previewProject, *plan);
            if (summary.isEmpty()) {
                QMessageBox::warning(this, "推敲", "AI から適用できる編集が返りませんでした。");
                return;
            }

            QString previewDiff;
            if (const Episode *previewEpisode = PlotEngine::Docs::findEpisodeById(previewProject, chapterId, episodeId))
                previewDiff = PlotEngine::AI::buildRevisionDiffSummary(currentText, previewEpisode->content);

            QVector<ReviewPanel::ActionItem> actionItems;
            actionItems.reserve(plan->actions.size());
            for (int i = 0; i < plan->actions.size(); ++i) {
                const auto &action = plan->actions.at(i);
                const QString summaryLine = i < summary.size() ? summary.at(i) : QStringLiteral("編集 %1").arg(i + 1);
                QString detail = summaryLine;
                QString actionType;
                switch (action.type) {
                case PlotEngine::AI::EditActionType::UpdateEpisodeContent:
                    actionType = QStringLiteral("update_episode_content");
                    break;
                case PlotEngine::AI::EditActionType::UpdateEpisodeMetadata:
                    actionType = QStringLiteral("update_episode_metadata");
                    break;
                case PlotEngine::AI::EditActionType::RenameChapter:
                    actionType = QStringLiteral("rename_chapter");
                    break;
                case PlotEngine::AI::EditActionType::RenameEpisode:
                    actionType = QStringLiteral("rename_episode");
                    break;
                case PlotEngine::AI::EditActionType::UpdateCharacterNotes:
                    actionType = QStringLiteral("update_character_notes");
                    break;
                case PlotEngine::AI::EditActionType::UpdateLocationNotes:
                    actionType = QStringLiteral("update_location_notes");
                    break;
                }
                detail += "\n種別: " + actionType;
                if (!action.title.trimmed().isEmpty())
                    detail += "\nタイトル: " + action.title.trimmed();
                if (!action.summary.trimmed().isEmpty())
                    detail += "\n要約: " + action.summary.trimmed();
                if (!action.notes.trimmed().isEmpty())
                    detail += "\nノート: " + action.notes.trimmed();
                if (!action.content.trimmed().isEmpty())
                    detail += "\n\n本文:\n" + action.content.left(4000);
                QString actionDiff;
                QString compareLeftTitle;
                QString compareLeftContent;
                QString compareRightTitle;
                QString compareRightContent;
                switch (action.type) {
                case PlotEngine::AI::EditActionType::UpdateEpisodeContent: {
                    if (const Episode *beforeEpisode = PlotEngine::Docs::findEpisodeById(m_project, action.chapterId, action.episodeId)) {
                        actionDiff = PlotEngine::AI::buildRevisionDiffSummary(beforeEpisode->content, action.content);
                        compareLeftTitle = QStringLiteral("現在の本文");
                        compareLeftContent = beforeEpisode->content;
                        compareRightTitle = QStringLiteral("AI 提案本文");
                        compareRightContent = action.content;
                    }
                    break;
                }
                case PlotEngine::AI::EditActionType::UpdateEpisodeMetadata: {
                    if (const Episode *beforeEpisode = PlotEngine::Docs::findEpisodeById(m_project, action.chapterId, action.episodeId)) {
                        QStringList lines;
                        if (!action.summary.isEmpty() && beforeEpisode->summary != action.summary)
                            lines.append(QStringLiteral("summary: %1 -> %2").arg(beforeEpisode->summary, action.summary));
                        if (!action.povCharacter.isEmpty() && beforeEpisode->povCharacter != action.povCharacter)
                            lines.append(QStringLiteral("POV: %1 -> %2").arg(beforeEpisode->povCharacter, action.povCharacter));
                        if (!action.timePeriod.isEmpty() && beforeEpisode->timePeriod != action.timePeriod)
                            lines.append(QStringLiteral("時間帯: %1 -> %2").arg(beforeEpisode->timePeriod, action.timePeriod));
                        if (!action.sceneType.isEmpty() && beforeEpisode->sceneType != action.sceneType)
                            lines.append(QStringLiteral("シーン種別: %1 -> %2").arg(beforeEpisode->sceneType, action.sceneType));
                        if (action.emotionalIntensity >= 0 && beforeEpisode->emotionalIntensity != action.emotionalIntensity)
                            lines.append(QStringLiteral("感情強度: %1 -> %2").arg(beforeEpisode->emotionalIntensity).arg(action.emotionalIntensity));
                        if (action.targetWordCount >= 0 && beforeEpisode->targetWordCount != action.targetWordCount)
                            lines.append(QStringLiteral("目標文字数: %1 -> %2").arg(beforeEpisode->targetWordCount).arg(action.targetWordCount));
                        actionDiff = lines.join('\n');
                        compareLeftTitle = QStringLiteral("現在のメタデータ");
                        compareLeftContent = QStringLiteral(
                            "タイトル: %1\n要約: %2\nPOV: %3\n時間帯: %4\nシーン種別: %5\n感情強度: %6\n目標文字数: %7")
                            .arg(beforeEpisode->title, beforeEpisode->summary, beforeEpisode->povCharacter,
                                 beforeEpisode->timePeriod, beforeEpisode->sceneType)
                            .arg(beforeEpisode->emotionalIntensity)
                            .arg(beforeEpisode->targetWordCount);
                        compareRightTitle = QStringLiteral("AI 提案メタデータ");
                        compareRightContent = QStringLiteral(
                            "タイトル: %1\n要約: %2\nPOV: %3\n時間帯: %4\nシーン種別: %5\n感情強度: %6\n目標文字数: %7")
                            .arg(beforeEpisode->title,
                                 action.summary.isEmpty() ? beforeEpisode->summary : action.summary,
                                 action.povCharacter.isEmpty() ? beforeEpisode->povCharacter : action.povCharacter,
                                 action.timePeriod.isEmpty() ? beforeEpisode->timePeriod : action.timePeriod,
                                 action.sceneType.isEmpty() ? beforeEpisode->sceneType : action.sceneType)
                            .arg(action.emotionalIntensity >= 0 ? action.emotionalIntensity : beforeEpisode->emotionalIntensity)
                            .arg(action.targetWordCount >= 0 ? action.targetWordCount : beforeEpisode->targetWordCount);
                    }
                    break;
                }
                case PlotEngine::AI::EditActionType::RenameChapter: {
                    if (const Chapter *beforeChapter = PlotEngine::Docs::findChapterById(m_project, action.chapterId)) {
                        actionDiff = QStringLiteral("章名: %1 -> %2").arg(beforeChapter->title, action.title);
                        compareLeftTitle = QStringLiteral("現在の章名");
                        compareLeftContent = beforeChapter->title;
                        compareRightTitle = QStringLiteral("AI 提案章名");
                        compareRightContent = action.title;
                    }
                    break;
                }
                case PlotEngine::AI::EditActionType::RenameEpisode: {
                    if (const Episode *beforeEpisode = PlotEngine::Docs::findEpisodeById(m_project, action.chapterId, action.episodeId)) {
                        actionDiff = QStringLiteral("エピソード名: %1 -> %2").arg(beforeEpisode->title, action.title);
                        compareLeftTitle = QStringLiteral("現在のエピソード名");
                        compareLeftContent = beforeEpisode->title;
                        compareRightTitle = QStringLiteral("AI 提案エピソード名");
                        compareRightContent = action.title;
                    }
                    break;
                }
                case PlotEngine::AI::EditActionType::UpdateCharacterNotes: {
                    if (const CharacterEntry *beforeCharacter = PlotEngine::Docs::findCharacterById(m_project, action.characterId)) {
                        actionDiff = PlotEngine::AI::buildRevisionDiffSummary(beforeCharacter->notes, action.notes);
                        compareLeftTitle = QStringLiteral("現在のキャラノート");
                        compareLeftContent = beforeCharacter->notes;
                        compareRightTitle = QStringLiteral("AI 提案キャラノート");
                        compareRightContent = action.notes;
                    }
                    break;
                }
                case PlotEngine::AI::EditActionType::UpdateLocationNotes: {
                    if (const LocationEntry *beforeLocation = PlotEngine::Docs::findLocationById(m_project, action.locationId)) {
                        actionDiff = PlotEngine::AI::buildRevisionDiffSummary(beforeLocation->notes, action.notes);
                        compareLeftTitle = QStringLiteral("現在の場所ノート");
                        compareLeftContent = beforeLocation->notes;
                        compareRightTitle = QStringLiteral("AI 提案場所ノート");
                        compareRightContent = action.notes;
                    }
                    break;
                }
                }

                actionItems.append({
                    summaryLine,
                    detail,
                    actionDiff,
                    compareLeftTitle,
                    compareLeftContent,
                    compareRightTitle,
                    compareRightContent,
                    true
                });
            }

            m_pendingReviewChapterId = chapterId;
            m_pendingReviewEpisodeId = episodeId;
            m_pendingReviewInstruction = instruction.trimmed();
            m_pendingReviewProtectedText = protectedText;
            m_pendingReviewCurrentText = currentText;
            m_pendingReviewAiModel = model;

            m_reviewPanel->setReviewTitle(QStringLiteral("Venice 編集レビュー"));
            m_reviewPanel->setRationale(plan->rationale);
            m_reviewPanel->setActions(actionItems);
            m_reviewPanel->setDiffSummary(previewDiff);
            m_reviewPanel->setRawResponse(response.content.left(16000));
            showDockPane(m_reviewDock);
            return;
        }

        QString polished = stripCodeFences(response.content);
        editor->setContent(polished);
        episode->content = polished;
        episode->modifiedAt = QDateTime::currentDateTime();
        episode->revisionNotes = instruction.trimmed();
        if (!protectedText.isEmpty())
            episode->revisionNotes += "\n[保護] " + protectedText.left(120);
        markAllTabsDirty();
        syncOpenDocumentTabs();
        recordEpisodeRevision(episodeId, polished, static_cast<int>(PlotEngine::Revisions::RevisionType::AiPolished),
                              instruction.trimmed(), model);
        refreshRevisionHistoryPanel();

        const QString diff = PlotEngine::AI::buildRevisionDiffSummary(currentText, polished);
        QMessageBox box(this);
        box.setWindowTitle("推敲");
        box.setText("Venice による推敲を適用しました。");
        box.setInformativeText("差分の概要:");
        box.setDetailedText(diff);
        box.exec();
        return;
    }

    QString polished = PlotEngine::Text::normalizeEpisodeText(currentText);
    editor->setContent(polished);
    episode->content = polished;
    episode->modifiedAt = QDateTime::currentDateTime();
    episode->revisionNotes = instruction.trimmed();
    if (!protectedText.isEmpty())
        episode->revisionNotes += "\n[保護] " + protectedText.left(120);
    markAllTabsDirty();
    syncOpenDocumentTabs();
    recordEpisodeRevision(episodeId, polished, static_cast<int>(PlotEngine::Revisions::RevisionType::ManualEdit), instruction.trimmed());
    refreshRevisionHistoryPanel();

    const QString diff = PlotEngine::AI::buildRevisionDiffSummary(currentText, polished);
    QMessageBox box(this);
    box.setWindowTitle("推敲");
    box.setText("軽い整形を適用しました。");
    box.setInformativeText("差分の概要:");
    box.setDetailedText(diff);
    box.exec();
}

void MainWindow::addCharacter()
{
    CharacterEntry c;
    c.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    c.name = "新キャラクター";
    m_project.characters.append(c);
    markAllTabsDirty();
    refreshProjectViews();
    onCharacterSelected(c.id);
}

void MainWindow::addLocation()
{
    LocationEntry l;
    l.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    l.name = "新場所";
    m_project.locations.append(l);
    markAllTabsDirty();
    refreshProjectViews();
    onLocationSelected(l.id);
}

void MainWindow::setCurrentProject(const NovelProject &project)
{
    setFocusMode(false);
    m_project = project;
    m_protectedSnippetsByDocument.clear();
    clearAiEditSnapshot();
    discardPendingAiReview();
    markAllTabsClean();

    while (m_editorTabs->count() > 0) {
        auto *w = m_editorTabs->widget(0);
        m_editorTabs->removeTab(0);
        w->deleteLater();
    }
    refreshExplorerOpenDocuments();
    updateBreadcrumbStatus();
    refreshRevisionHistoryPanel();
    refreshProtectedSnippetPanel();
    refreshWorkspaceView();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!maybeSave()) {
        event->ignore();
        return;
    }

    saveWindowState();
    event->accept();
}

bool MainWindow::confirmSaveChanges()
{
    if (!m_dirty) return true;
    auto result = QMessageBox::question(this, "変更の保存",
        "プロジェクトに未保存の変更があります。保存しますか?",
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    if (result == QMessageBox::Yes) return saveProject();
    if (result == QMessageBox::No) return true;
    return false;
}

bool MainWindow::maybeSave()
{
    if (!m_dirty) return true;
    return confirmSaveChanges();
}

bool MainWindow::promptCloseDocument(QWidget *tab)
{
    if (!tab)
        return true;

    if (!tab->property("isDirty").toBool())
        return true;

    const QString title = baseTabTitle(tab).isEmpty() ? QStringLiteral("この文書") : baseTabTitle(tab);
    const auto result = QMessageBox::question(this, "未保存の変更",
        QString("「%1」に未保存の変更があります。保存しますか?").arg(title),
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    if (result == QMessageBox::Yes)
        return saveProject();
    if (result == QMessageBox::No)
        return true;
    return false;
}

void MainWindow::loadRecentProjects()
{
    QSettings settings = PlotEngine::App::makeSettings();
    m_recentProjects = settings.value("recentProjects").toStringList();
    m_recentProjects.removeAll(QString());
    m_recentProjects.removeDuplicates();
    updateRecentProjectsMenu();
}

void MainWindow::updateRecentProjectsMenu()
{
    if (!m_recentProjectsMenu)
        return;

    m_recentProjectsMenu->clear();

    if (m_recentProjects.isEmpty()) {
        auto *emptyAction = m_recentProjectsMenu->addAction("履歴なし");
        emptyAction->setEnabled(false);
        return;
    }

    for (int i = 0; i < m_recentProjects.size() && i < kRecentProjectLimit; ++i) {
        const QString path = m_recentProjects.at(i);
        const QString label = QStringLiteral("%1. %2").arg(i + 1).arg(QFileInfo(path).fileName().isEmpty() ? path : QFileInfo(path).fileName());
        auto *action = m_recentProjectsMenu->addAction(appIcon("recent"), label);
        action->setToolTip(path);
        action->setData(path);
        connect(action, &QAction::triggered, this, [this, path]() {
            if (maybeSave())
                openProjectFile(path);
        });
    }

    m_recentProjectsMenu->addSeparator();
    auto *clearAction = m_recentProjectsMenu->addAction(appIcon("exit"), "履歴を消去");
    connect(clearAction, &QAction::triggered, this, [this]() {
        m_recentProjects.clear();
        QSettings settings = PlotEngine::App::makeSettings();
        settings.setValue("recentProjects", m_recentProjects);
        updateRecentProjectsMenu();
    });
}

void MainWindow::addRecentProject(const QString &path)
{
    if (path.isEmpty())
        return;

    QFileInfo info(path);
    const QString normalized = info.absoluteFilePath();
    m_recentProjects.removeAll(normalized);
    m_recentProjects.prepend(normalized);
    while (m_recentProjects.size() > kRecentProjectLimit)
        m_recentProjects.removeLast();

    QSettings settings = PlotEngine::App::makeSettings();
    settings.setValue("recentProjects", m_recentProjects);
    updateRecentProjectsMenu();
}

void MainWindow::loadWindowState()
{
    QSettings settings = PlotEngine::App::makeSettings();
    const QByteArray geometry = settings.value("window/geometry").toByteArray();
    if (!geometry.isEmpty())
        restoreGeometry(geometry);

    const QByteArray dockState = settings.value("window/dockState").toByteArray();
    if (!dockState.isEmpty())
        m_dockManager->restoreState(dockState, 1);
}

void MainWindow::saveWindowState()
{
    QSettings settings = PlotEngine::App::makeSettings();
    settings.setValue("window/geometry", saveGeometry());
    if (m_dockManager)
        settings.setValue("window/dockState", m_dockManager->saveState(1));
    saveSessionState();
}

void MainWindow::loadSessionState()
{
    QSettings settings = PlotEngine::App::makeSettings();
    const QString projectPath = settings.value("session/lastProjectPath").toString();
    if (projectPath.isEmpty() || !QFileInfo::exists(projectPath)) {
        refreshWorkspaceView();
        return;
    }

    m_loadingSession = true;
    openProjectFile(projectPath);

    const QStringList openDocuments = settings.value("session/openDocuments").toStringList();
    bool restoredAny = false;
    for (const QString &entry : openDocuments) {
        const QStringList parts = entry.split('\t');
        if (parts.size() != 2)
            continue;
        restoredAny = openDocumentByKind(parts.at(0), parts.at(1)) || restoredAny;
    }

    const QString currentKind = settings.value("session/currentKind").toString();
    const QString currentId = settings.value("session/currentId").toString();
    if (!currentKind.isEmpty() && !currentId.isEmpty())
        openDocumentByKind(currentKind, currentId);
    else if (!restoredAny)
        openFirstEpisodeIfAny();

    m_loadingSession = false;
    markAllTabsClean();
    refreshExplorerOpenDocuments();
    updateStatusBar();
}

void MainWindow::saveSessionState()
{
    QSettings settings = PlotEngine::App::makeSettings();
    settings.setValue("session/lastProjectPath", m_project.filePath);

    QStringList openDocuments;
    for (int i = 0; i < m_editorTabs->count(); ++i) {
        auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->widget(i));
        if (!editor)
            continue;
        const QString kind = editor->property("documentKind").toString();
        const QString id = editor->property("documentId").toString();
        if (!kind.isEmpty() && !id.isEmpty())
            openDocuments.append(kind + '\t' + id);
    }
    settings.setValue("session/openDocuments", openDocuments);

    auto *currentEditor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());
    if (currentEditor) {
        settings.setValue("session/currentKind", currentEditor->property("documentKind").toString());
        settings.setValue("session/currentId", currentEditor->property("documentId").toString());
    } else {
        settings.remove("session/currentKind");
        settings.remove("session/currentId");
    }
    settings.sync();
}

void MainWindow::applyStyleSheet()
{
    qApp->setStyleSheet(R"(
        QMainWindow { background: #1e1e1e; }
        QMenuBar { background: #252526; color: #cccccc; padding: 2px 6px; }
        QMenuBar::item:selected { background: #094771; }
        QMenu { background: #252526; color: #cccccc; border: 1px solid #1f1f1f; }
        QMenu::item:selected { background: #094771; }
        QToolBar { background: #2d2d2d; border: none; spacing: 4px; padding: 4px; }
        QToolBar QToolButton { color: #cccccc; padding: 4px 10px; border-radius: 3px; }
        QToolBar QToolButton:hover { background: #3e3e42; }
        QTabWidget::pane { background: #1e1e1e; border: 1px solid #3c3c3c; }
        QTabBar::tab { background: #2d2d2d; color: #cccccc; padding: 7px 12px; border: 1px solid #3c3c3c; border-bottom: none; min-width: 120px; }
        QTabBar::tab:selected { background: #1e1e1e; color: #ffffff; border-bottom: 2px solid #007acc; }
        QTabBar::tab:hover { background: #3e3e42; }
        QTreeView { background: #1e1e1e; color: #cccccc; border: none; }
        QTreeView::item:selected { background: #094771; }
        QTreeView::item:hover { background: #2a2d2e; }
        QStatusBar { background: #007acc; color: #ffffff; }
        QStatusBar QLabel { color: #ffffff; }
        QStatusBar::item { border: none; }
        QSplitter::handle { background: #3c3c3c; }
        QDialog { background: #1e1e1e; color: #cccccc; }
        QLineEdit { background: #3c3c3c; color: #ffffff; border: 1px solid #3c3c3c; padding: 6px 8px; }
        QListWidget { background: #252526; color: #cccccc; border: 1px solid #3c3c3c; }
        QListWidget::item:selected { background: #094771; }
    )");
}
