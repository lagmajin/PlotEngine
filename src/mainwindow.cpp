#include "mainwindow.h"
#include "core/projectmanager.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QTextStream>
#include <QScreen>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setMinimumSize(1000, 600);
    applyStyleSheet();
    setupMenuBar();
    setupToolBar();
    setupEditorTabs();
    setupDockWidgets();
    setupStatusBar();
    connectSignals();
    updateStatusBar();
}

void MainWindow::setupMenuBar()
{
    auto *fileMenu = menuBar()->addMenu("ファイル(&F)");

    auto *newAction = fileMenu->addAction("新規プロジェクト(&N)");
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &MainWindow::newProject);

    auto *openAction = fileMenu->addAction("プロジェクトを開く(&O)");
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::openProject);

    fileMenu->addSeparator();

    auto *saveAction = fileMenu->addAction("保存(&S)");
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveProject);

    auto *saveAsAction = fileMenu->addAction("名前を付けて保存(&A)");
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::saveProjectAs);

    fileMenu->addSeparator();

    auto *exitAction = fileMenu->addAction("終了(&X)");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    auto *editMenu = menuBar()->addMenu("編集(&E)");
    editMenu->addAction("元に戻す(&U)", this, [this]() {
        auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());
        if (editor) editor->undo();
    }, QKeySequence::Undo);
    editMenu->addAction("やり直し(&R)", this, [this]() {
        auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());
        if (editor) editor->redo();
    }, QKeySequence::Redo);

    auto *viewMenu = menuBar()->addMenu("表示(&V)");
    viewMenu->addAction(m_structureDock->toggleViewAction());
    viewMenu->addAction(m_noteDock->toggleViewAction());

    auto *helpMenu = menuBar()->addMenu("ヘルプ(&H)");
    helpMenu->addAction("PlotEngine について", this, [this]() {
        QMessageBox::about(this, "PlotEngine について",
            "PlotEngine v1.0.0\n小説制作統合開発環境");
    });
}

void MainWindow::setupToolBar()
{
    auto *toolbar = addToolBar("メインツールバー");
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(24, 24));

    toolbar->addAction("新規", this, &MainWindow::newProject);
    toolbar->addAction("開く", this, &MainWindow::openProject);
    toolbar->addAction("保存", this, &MainWindow::saveProject);
    toolbar->addSeparator();
    toolbar->addAction("章追加", this, &MainWindow::addChapter);
    toolbar->addAction("シーン追加", this, [this]() {
        if (m_project.chapters.isEmpty()) return;
        addScene(m_project.chapters.first().id);
    });
    toolbar->addSeparator();
    toolbar->addAction("キャラ追加", this, &MainWindow::addCharacter);
    toolbar->addAction("場所追加", this, &MainWindow::addLocation);
}

void MainWindow::setupEditorTabs()
{
    m_editorTabs = new QTabWidget(this);
    m_editorTabs->setTabsClosable(true);
    m_editorTabs->setMovable(true);
    m_editorTabs->setDocumentMode(true);
    setCentralWidget(m_editorTabs);

    connect(m_editorTabs, &QTabWidget::tabCloseRequested, this, [this](int index) {
        auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->widget(index));
        m_editorTabs->removeTab(index);
        if (editor) editor->deleteLater();
    });
}

void MainWindow::setupDockWidgets()
{
    m_structurePanel = new StructurePanel(this);
    m_structureDock = new QDockWidget("構造", this);
    m_structureDock->setWidget(m_structurePanel);
    m_structureDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, m_structureDock);

    m_notePanel = new NotePanel(this);
    m_noteDock = new QDockWidget("ノート", this);
    m_noteDock->setWidget(m_notePanel);
    m_noteDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    tabifyDockWidget(m_structureDock, m_noteDock);
}

void MainWindow::setupStatusBar()
{
    m_statusFile = new QLabel("プロジェクト未読み込み");
    m_statusWords = new QLabel("文字数: 0");
    m_statusCursor = new QLabel("行: 1 列: 1");

    statusBar()->addWidget(m_statusFile, 1);
    statusBar()->addPermanentWidget(m_statusWords);
    statusBar()->addPermanentWidget(m_statusCursor);
}

void MainWindow::connectSignals()
{
    connect(m_structurePanel, &StructurePanel::sceneSelected,
            this, &MainWindow::onSceneSelected);
    connect(m_notePanel, &NotePanel::characterSelected,
            this, &MainWindow::onCharacterSelected);
    connect(m_notePanel, &NotePanel::locationSelected,
            this, &MainWindow::onLocationSelected);
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
    m_dirty = true;
    m_structurePanel->loadProject(m_project);
    m_notePanel->loadProject(m_project);
    updateStatusBar();
}

void MainWindow::openProject()
{
    if (!maybeSave()) return;

    QString path = QFileDialog::getOpenFileName(this,
        "プロジェクトを開く", QString(),
        "PlotEngine プロジェクト (*.plotproj);;全てのファイル (*)");

    if (path.isEmpty()) return;

    auto result = ProjectManager::load(path);
    if (!result) {
        QMessageBox::warning(this, "エラー", "プロジェクトの読み込みに失敗しました。");
        return;
    }

    setCurrentProject(*result);
    m_structurePanel->loadProject(m_project);
    m_notePanel->loadProject(m_project);
    updateStatusBar();
}

bool MainWindow::saveProject()
{
    if (m_project.filePath.isEmpty())
        return saveProjectAs();
    return ProjectManager::save(m_project, m_project.filePath);
}

bool MainWindow::saveProjectAs()
{
    QString path = QFileDialog::getSaveFileName(this,
        "名前を付けて保存", QString(),
        "PlotEngine プロジェクト (*.plotproj);;全てのファイル (*)");
    if (path.isEmpty()) return false;

    if (!path.endsWith(".plotproj"))
        path += ".plotproj";

    m_project.filePath = path;
    bool ok = ProjectManager::save(m_project, path);
    if (ok) m_dirty = false;
    updateStatusBar();
    return ok;
}

void MainWindow::onSceneSelected(const QString &chapterId, const QString &sceneId)
{
    for (auto &ch : m_project.chapters) {
        if (ch.id != chapterId) continue;
        for (auto &sc : ch.scenes) {
            if (sc.id != sceneId) continue;

            QString tabTitle = ch.title + " / " + sc.title;
            for (int i = 0; i < m_editorTabs->count(); ++i) {
                auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->widget(i));
                if (editor && editor->sceneId() == sceneId) {
                    m_editorTabs->setCurrentIndex(i);
                    return;
                }
            }

            auto *editor = new NovelEditor(sceneId);
            editor->setContent(sc.content);
            int idx = m_editorTabs->addTab(editor, tabTitle);
            m_editorTabs->setCurrentIndex(idx);

            connect(editor, &NovelEditor::contentChanged, this,
                [this, chapterId, sceneId](const QString &text) {
                    for (auto &ch2 : m_project.chapters) {
                        if (ch2.id != chapterId) continue;
                        for (auto &sc2 : ch2.scenes) {
                            if (sc2.id != sceneId) {
                                sc2.content = text;
                                sc2.modifiedAt = QDateTime::currentDateTime();
                            }
                        }
                    }
                    m_dirty = true;
                    updateStatusBar();
                });
            return;
        }
    }
}

void MainWindow::onCharacterSelected(const QString &characterId)
{
    for (auto &c : m_project.characters) {
        if (c.id != characterId) continue;

        QString tabTitle = "キャラ: " + c.name;
        for (int i = 0; i < m_editorTabs->count(); ++i) {
            auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->widget(i));
            if (editor && editor->sceneId() == characterId) {
                m_editorTabs->setCurrentIndex(i);
                return;
            }
        }

        auto *editor = new NovelEditor(characterId);
        editor->setContent(c.notes);
        int idx = m_editorTabs->addTab(editor, tabTitle);
        m_editorTabs->setCurrentIndex(idx);

        connect(editor, &NovelEditor::contentChanged, this,
            [this, characterId](const QString &text) {
                for (auto &c2 : m_project.characters) {
                    if (c2.id == characterId) c2.notes = text;
                }
                m_dirty = true;
            });
        return;
    }
}

void MainWindow::onLocationSelected(const QString &locationId)
{
    for (auto &l : m_project.locations) {
        if (l.id != locationId) continue;

        QString tabTitle = "場所: " + l.name;
        for (int i = 0; i < m_editorTabs->count(); ++i) {
            auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->widget(i));
            if (editor && editor->sceneId() == locationId) {
                m_editorTabs->setCurrentIndex(i);
                return;
            }
        }

        auto *editor = new NovelEditor(locationId);
        editor->setContent(l.notes);
        int idx = m_editorTabs->addTab(editor, tabTitle);
        m_editorTabs->setCurrentIndex(idx);

        connect(editor, &NovelEditor::contentChanged, this,
            [this, locationId](const QString &text) {
                for (auto &l2 : m_project.locations) {
                    if (l2.id == locationId) l2.notes = text;
                }
                m_dirty = true;
            });
        return;
    }
}

void MainWindow::updateStatusBar()
{
    if (m_project.filePath.isEmpty())
        m_statusFile->setText(m_project.name + " (未保存)");
    else
        m_statusFile->setText(m_project.name + " - " + m_project.filePath);

    m_statusWords->setText("文字数: " + QString::number(m_project.totalWordCount()));
}

void MainWindow::addChapter()
{
    Chapter ch;
    ch.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    ch.title = "第" + QString::number(m_project.chapters.size() + 1) + "章";
    ch.sortOrder = m_project.chapters.size();
    m_project.chapters.append(ch);
    m_dirty = true;
    m_structurePanel->loadProject(m_project);
}

void MainWindow::addScene(const QString &chapterId)
{
    for (auto &ch : m_project.chapters) {
        if (ch.id != chapterId) continue;
        Scene sc;
        sc.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        sc.title = "Scene " + QString::number(ch.scenes.size() + 1);
        sc.createdAt = QDateTime::currentDateTime();
        sc.modifiedAt = QDateTime::currentDateTime();
        ch.scenes.append(sc);
        break;
    }
    m_dirty = true;
    m_structurePanel->loadProject(m_project);
}

void MainWindow::addCharacter()
{
    CharacterEntry c;
    c.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    c.name = "新キャラクター";
    m_project.characters.append(c);
    m_dirty = true;
    m_notePanel->loadProject(m_project);
}

void MainWindow::addLocation()
{
    LocationEntry l;
    l.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    l.name = "新場所";
    m_project.locations.append(l);
    m_dirty = true;
    m_notePanel->loadProject(m_project);
}

void MainWindow::setCurrentProject(const NovelProject &project)
{
    m_project = project;
    m_dirty = false;

    while (m_editorTabs->count() > 0) {
        auto *w = m_editorTabs->widget(0);
        m_editorTabs->removeTab(0);
        w->deleteLater();
    }
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

void MainWindow::applyStyleSheet()
{
    qApp->setStyleSheet(R"(
        QMainWindow { background: #1e1e2e; }
        QMenuBar { background: #181825; color: #cdd6f4; }
        QMenuBar::item:selected { background: #45475a; }
        QMenu { background: #1e1e2e; color: #cdd6f4; border: 1px solid #313244; }
        QMenu::item:selected { background: #45475a; }
        QToolBar { background: #181825; border: none; spacing: 4px; padding: 2px; }
        QToolBar QToolButton { color: #cdd6f4; padding: 4px 8px; }
        QToolBar QToolButton:hover { background: #45475a; }
        QTabWidget::pane { background: #1e1e2e; border: 1px solid #313244; }
        QTabBar::tab { background: #181825; color: #a6adc8; padding: 6px 12px; border: 1px solid #313244; }
        QTabBar::tab:selected { background: #1e1e2e; color: #cdd6f4; border-bottom: 2px solid #89b4fa; }
        QDockWidget { color: #cdd6f4; titlebar-close-icon: none; }
        QDockWidget::title { background: #181825; padding: 4px; }
        QTreeView { background: #1e1e2e; color: #cdd6f4; border: none; }
        QTreeView::item:selected { background: #45475a; }
        QTreeView::item:hover { background: #313244; }
        QStatusBar { background: #181825; color: #a6adc8; }
        QStatusBar::item { border: none; }
        QSplitter::handle { background: #313244; }
    )");
}