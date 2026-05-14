#include "mainwindow.h"
import PlotEngine.Core.NovelProject;
import PlotEngine.Core.ProjectManager;
import PlotEngine.Core.TextUtils;
import PlotEngine.AI.RevisionPrompt;
#include "DockManager.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QTextStream>
#include <QTextCursor>
#include <QScreen>
#include <QInputDialog>
#include <QStringList>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setMinimumSize(1000, 600);
    applyStyleSheet();
    setupDockManager();
    setupDockWidgets();
    setupEditorTabs();
    setupMenuBar();
    setupToolBar();
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
    viewMenu->addAction(m_editorDock->toggleViewAction());

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
    toolbar->addAction("エピソード追加", this, [this]() {
        if (m_project.chapters.isEmpty()) return;
        addEpisode(m_project.chapters.first().id);
    });
    toolbar->addAction("推敲", this, &MainWindow::polishCurrentEpisode);
    toolbar->addSeparator();
    toolbar->addAction("キャラ追加", this, &MainWindow::addCharacter);
    toolbar->addAction("場所追加", this, &MainWindow::addLocation);
}

void MainWindow::setupDockManager()
{
    ads::CDockManager::setConfigFlags(ads::CDockManager::DefaultOpaqueConfig);
    ads::CDockManager::setConfigFlag(ads::CDockManager::TabsAtBottom, true);
    ads::CDockManager::setConfigFlag(ads::CDockManager::FocusHighlighting, true);
    ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaHideDisabledButtons, true);
    ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaDynamicTabsMenuButtonVisibility, true);
    ads::CDockManager::setConfigFlag(ads::CDockManager::HideSingleCentralWidgetTitleBar, true);
    m_dockManager = new ads::CDockManager(this);
}

void MainWindow::setupEditorTabs()
{
    m_editorTabs = new QTabWidget(this);
    m_editorTabs->setTabsClosable(true);
    m_editorTabs->setMovable(true);
    m_editorTabs->setDocumentMode(true);
    m_editorDock = m_dockManager->createDockWidget("エディタ");
    m_editorDock->setWidget(m_editorTabs);
    m_dockManager->addDockWidget(ads::CenterDockWidgetArea, m_editorDock);

    connect(m_editorTabs, &QTabWidget::tabCloseRequested, this, [this](int index) {
        auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->widget(index));
        m_editorTabs->removeTab(index);
        if (editor) editor->deleteLater();
    });
}

void MainWindow::setupDockWidgets()
{
    m_structurePanel = new StructurePanel(this);
    m_structureDock = m_dockManager->createDockWidget("構造");
    m_structureDock->setWidget(m_structurePanel);
    m_dockManager->addDockWidget(ads::LeftDockWidgetArea, m_structureDock);

    m_notePanel = new NotePanel(this);
    m_noteDock = m_dockManager->createDockWidget("ノート");
    m_noteDock->setWidget(m_notePanel);
    m_dockManager->addDockWidget(ads::LeftDockWidgetArea, m_noteDock);
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
    connect(m_structurePanel, &StructurePanel::episodeSelected,
            this, &MainWindow::onEpisodeSelected);
    connect(m_structurePanel, &StructurePanel::episodeAddRequested,
            this, &MainWindow::addEpisode);
    connect(m_structurePanel, &StructurePanel::chapterRenamed,
            this, &MainWindow::renameChapter);
    connect(m_structurePanel, &StructurePanel::episodeRenamed,
            this, &MainWindow::renameEpisode);
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
    bool ok = ProjectManager::save(m_project, m_project.filePath);
    if (ok) m_dirty = false;
    updateStatusBar();
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
        m_dirty = false;
    }
    updateStatusBar();
    return ok;
}

void MainWindow::onEpisodeSelected(const QString &chapterId, const QString &episodeId)
{
    for (auto &ch : m_project.chapters) {
        if (ch.id != chapterId) continue;
        for (auto &ep : ch.episodes) {
            if (ep.id != episodeId) continue;

            QString tabTitle = ch.title + " / " + ep.title;
            for (int i = 0; i < m_editorTabs->count(); ++i) {
                auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->widget(i));
                if (editor && editor->sceneId() == episodeId) {
                    m_editorTabs->setCurrentIndex(i);
                    return;
                }
            }

            auto *editor = new NovelEditor(episodeId);
            editor->setContent(ep.content);
            int idx = m_editorTabs->addTab(editor, tabTitle);
            m_editorTabs->setCurrentIndex(idx);

            connect(editor, &NovelEditor::contentChanged, this,
                [this, chapterId, episodeId](const QString &text) {
                    for (auto &ch2 : m_project.chapters) {
                        if (ch2.id != chapterId) continue;
                        for (auto &ep2 : ch2.episodes) {
                            if (ep2.id != episodeId) continue;
                            ep2.content = text;
                            ep2.modifiedAt = QDateTime::currentDateTime();
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

void MainWindow::addEpisode(const QString &chapterId)
{
    for (auto &ch : m_project.chapters) {
        if (ch.id != chapterId) continue;
        Episode ep;
        ep.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        ep.title = "エピソード" + QString::number(ch.episodes.size() + 1);
        ep.createdAt = QDateTime::currentDateTime();
        ep.modifiedAt = QDateTime::currentDateTime();
        ch.episodes.append(ep);
        break;
    }
    m_dirty = true;
    m_structurePanel->loadProject(m_project);
}

void MainWindow::renameChapter(const QString &chapterId, const QString &title)
{
    if (title.trimmed().isEmpty()) {
        m_structurePanel->loadProject(m_project);
        return;
    }

    for (auto &ch : m_project.chapters) {
        if (ch.id == chapterId) {
            ch.title = title;
            m_dirty = true;
            for (int i = 0; i < m_editorTabs->count(); ++i) {
                auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->widget(i));
                if (!editor) continue;
                for (const auto &ep : ch.episodes) {
                    if (ep.id != editor->sceneId()) continue;
                    m_editorTabs->setTabText(i, ch.title + " / " + ep.title);
                    break;
                }
            }
            updateStatusBar();
            break;
        }
    }
}

void MainWindow::renameEpisode(const QString &chapterId, const QString &episodeId, const QString &title)
{
    if (title.trimmed().isEmpty()) {
        m_structurePanel->loadProject(m_project);
        return;
    }

    for (auto &ch : m_project.chapters) {
        if (ch.id != chapterId) continue;
        for (auto &ep : ch.episodes) {
            if (ep.id != episodeId) continue;
            ep.title = title;
            m_dirty = true;
            for (int i = 0; i < m_editorTabs->count(); ++i) {
                auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->widget(i));
                if (editor && editor->sceneId() == episodeId) {
                    m_editorTabs->setTabText(i, ch.title + " / " + ep.title);
                    break;
                }
            }
            updateStatusBar();
            break;
        }
    }
}

void MainWindow::polishCurrentEpisode()
{
    auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());
    if (!editor) {
        QMessageBox::information(this, "推敲", "推敲したいエピソードを開いてください。");
        return;
    }

    QString episodeId = editor->sceneId();
    Episode *episode = nullptr;
    for (auto &ch : m_project.chapters) {
        for (auto &ep : ch.episodes) {
            if (ep.id != episodeId) continue;
            episode = &ep;
            break;
        }
        if (episode) break;
    }

    if (!episode) {
        QMessageBox::warning(this, "推敲", "開いているエピソードを特定できませんでした。");
        return;
    }

    QString currentText = editor->toPlainText();
    QString selectedText = editor->textCursor().selectedText();
    selectedText.replace(QChar(0x2029), '\n');
    selectedText.replace(QChar(0x2028), '\n');
    selectedText = selectedText.trimmed();

    bool ok = false;
    QString instruction = QInputDialog::getMultiLineText(
        this,
        "変更したい内容",
        "AIに直してほしい内容:",
        QStringLiteral("文体を保ちながら、冗長な表現を削って読みやすくする"),
        &ok);
    if (!ok || instruction.trimmed().isEmpty())
        return;

    QString protectedText = QInputDialog::getMultiLineText(
        this,
        "保護したい本文",
        selectedText.isEmpty()
            ? "変更したくない本文や固有表現を貼ってください（空欄可）:"
            : "選択範囲を保護するなら、そのまま残してください（空欄可）:",
        selectedText,
        &ok);
    if (!ok)
        return;
    protectedText = protectedText.trimmed();

    QString mode = QInputDialog::getItem(
        this,
        "推敲モード",
        "処理:",
        QStringList({"整形して適用", "AI用プロンプトを作成"}),
        0,
        false,
        &ok);
    if (!ok || mode.isEmpty())
        return;

    PlotEngine::AI::RevisionPromptRequest request;
    request.desiredChanges = instruction.trimmed();
    if (!protectedText.isEmpty()) {
        request.protectedSnippets.append({
            selectedText.isEmpty() ? QStringLiteral("保護対象") : QStringLiteral("選択範囲"),
            protectedText
        });
    }

    QString prompt = PlotEngine::AI::buildRevisionPrompt(
        m_project, chapterId, episodeId, currentText, request);
    if (mode == "AI用プロンプトを作成") {
        qApp->clipboard()->setText(prompt);
        QMessageBox::information(this, "推敲", "AI用プロンプトをクリップボードにコピーしました。");
        return;
    }

    QString polished = PlotEngine::Text::normalizeEpisodeText(currentText);
    editor->setContent(polished);
    episode->content = polished;
    episode->modifiedAt = QDateTime::currentDateTime();
    episode->revisionNotes = instruction.trimmed();
    if (!protectedText.isEmpty())
        episode->revisionNotes += "\n[保護] " + protectedText.left(120);
    m_dirty = true;
    updateStatusBar();

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
        QTreeView { background: #1e1e2e; color: #cdd6f4; border: none; }
        QTreeView::item:selected { background: #45475a; }
        QTreeView::item:hover { background: #313244; }
        QStatusBar { background: #181825; color: #a6adc8; }
        QStatusBar::item { border: none; }
        QSplitter::handle { background: #313244; }
    )");
}
