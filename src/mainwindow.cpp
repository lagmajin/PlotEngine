#include "mainwindow.h"
import PlotEngine.Core.NovelProject;
import PlotEngine.Core.ProjectManager;
import PlotEngine.Core.TextUtils;
import PlotEngine.AI.RevisionPrompt;
#include "DockManager.h"
#include <functional>

namespace {
struct QuickOpenItem {
    QString kind;
    QString id;
    QString label;
    QString detail;
};

struct PaletteCommand {
    QString title;
    QString category;
    QString shortcut;
    std::function<void()> action;
};
}

#include <QFileDialog>
#include <QMessageBox>
#include <QAbstractItemView>
#include <QApplication>
#include <QClipboard>
#include <QDialog>
#include <QDialogButtonBox>
#include <QKeySequence>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLineEdit>
#include <QTabBar>
#include <QTextStream>
#include <QTextCursor>
#include <QTextBlock>
#include <QScreen>
#include <QInputDialog>
#include <QStringList>
#include <QVBoxLayout>
#include <QLabel>
#include <QHBoxLayout>
#include <QVector>

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
    editMenu->addSeparator();
    editMenu->addAction("検索(&F)", this, [this]() {
        auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());
        if (!editor) return;
        editor->showSearchBar();
        const QString selected = editor->textCursor().selectedText().trimmed();
        if (!selected.isEmpty())
            editor->setSearchText(selected);
    }, QKeySequence::Find);
    editMenu->addAction("置換(&H)", this, [this]() {
        auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());
        if (!editor) return;
        editor->showReplaceBar();
        const QString selected = editor->textCursor().selectedText().trimmed();
        if (!selected.isEmpty())
            editor->setSearchText(selected);
    }, QKeySequence::Replace);
    editMenu->addAction("次を検索(&G)", this, [this]() {
        auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());
        if (editor) editor->findNext();
    }, QKeySequence("F3"));
    editMenu->addAction("前を検索(&B)", this, [this]() {
        auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());
        if (editor) editor->findPrevious();
    }, QKeySequence("Shift+F3"));
    editMenu->addSeparator();
    editMenu->addAction("クイックオープン(&P)", this, &MainWindow::quickOpen, QKeySequence("Ctrl+P"));
    editMenu->addAction("コマンドパレット(&C)", this, &MainWindow::commandPalette, QKeySequence("Ctrl+Shift+P"));

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
    toolbar->addAction("Quick Open", this, &MainWindow::quickOpen);
    toolbar->addAction("Command Palette", this, &MainWindow::commandPalette);
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
    m_editorTabs->setElideMode(Qt::ElideMiddle);
    m_editorTabs->setUsesScrollButtons(true);
    m_editorTabs->tabBar()->setExpanding(false);
    m_editorDock = m_dockManager->createDockWidget("エディタ");
    m_editorDock->setWidget(m_editorTabs);
    m_dockManager->addDockWidget(ads::CenterDockWidgetArea, m_editorDock);

    connect(m_editorTabs, &QTabWidget::tabCloseRequested, this, [this](int index) {
        auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->widget(index));
        m_editorTabs->removeTab(index);
        if (editor) editor->deleteLater();
        refreshExplorerOpenDocuments();
        updateCursorStatus();
        updateBreadcrumbStatus();
    });
    connect(m_editorTabs, &QTabWidget::currentChanged, this, [this]() {
        updateCursorStatus();
        refreshExplorerOpenDocuments();
        updateBreadcrumbStatus();
    });
}

void MainWindow::setupDockWidgets()
{
    m_structurePanel = new StructurePanel(this);
    m_structureDock = m_dockManager->createDockWidget("エクスプローラ");
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
    connect(m_notePanel, &NotePanel::characterSelected,
            this, &MainWindow::onCharacterSelected);
    connect(m_notePanel, &NotePanel::locationSelected,
            this, &MainWindow::onLocationSelected);
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
    commands.append({"クイックオープン", "ナビゲーション", "Ctrl+P", [this]() { quickOpen(); }});
    commands.append({"検索", "エディタ", "Ctrl+F", [this]() {
        auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());
        if (!editor) return;
        editor->showSearchBar();
    }});
    commands.append({"置換", "エディタ", "Ctrl+H", [this]() {
        auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());
        if (!editor) return;
        editor->showReplaceBar();
    }});
    commands.append({"次を検索", "エディタ", "F3", [this]() {
        auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());
        if (editor) editor->findNext();
    }});
    commands.append({"前を検索", "エディタ", "Shift+F3", [this]() {
        auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());
        if (editor) editor->findPrevious();
    }});
    commands.append({"新規プロジェクト", "ファイル", "Ctrl+N", [this]() { newProject(); }});
    commands.append({"プロジェクトを開く", "ファイル", "Ctrl+O", [this]() { openProject(); }});
    commands.append({"保存", "ファイル", "Ctrl+S", [this]() { saveProject(); }});
    commands.append({"名前を付けて保存", "ファイル", "Ctrl+Shift+S", [this]() { saveProjectAs(); }});
    commands.append({"推敲", "AI", "", [this]() { polishCurrentEpisode(); }});
    commands.append({"章を追加", "構造", "", [this]() { addChapter(); }});
    commands.append({"エピソードを追加", "構造", "", [this]() {
        if (!m_project.chapters.isEmpty())
            addEpisode(m_project.chapters.first().id);
    }});

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
        commands[index].action();
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
    markAllTabsClean();
    markAllTabsDirty();
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
    markAllTabsClean();
    m_structurePanel->loadProject(m_project);
    m_notePanel->loadProject(m_project);
    updateStatusBar();
}

bool MainWindow::saveProject()
{
    if (m_project.filePath.isEmpty())
        return saveProjectAs();
    bool ok = ProjectManager::save(m_project, m_project.filePath);
    if (ok) markAllTabsClean();
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
        markAllTabsClean();
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
            editor->setProperty("documentKind", "episode");
            editor->setProperty("documentId", episodeId);
            editor->setProperty("baseTabTitle", tabTitle);
            editor->setProperty("isDirty", false);
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
                    markAllTabsDirty();
                    updateStatusBar();
                });
            connect(editor, &QPlainTextEdit::cursorPositionChanged,
                    this, &MainWindow::updateCursorStatus);
            refreshExplorerOpenDocuments();
            updateCursorStatus();
            updateBreadcrumbStatus();
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
        editor->setProperty("documentKind", "character");
        editor->setProperty("documentId", characterId);
        editor->setProperty("baseTabTitle", tabTitle);
        editor->setProperty("isDirty", false);
        editor->setContent(c.notes);
        int idx = m_editorTabs->addTab(editor, tabTitle);
        m_editorTabs->setCurrentIndex(idx);

        connect(editor, &NovelEditor::contentChanged, this,
            [this, characterId](const QString &text) {
                for (auto &c2 : m_project.characters) {
                    if (c2.id == characterId) c2.notes = text;
                }
                markAllTabsDirty();
                updateStatusBar();
            });
        connect(editor, &QPlainTextEdit::cursorPositionChanged,
                this, &MainWindow::updateCursorStatus);
        refreshExplorerOpenDocuments();
        updateCursorStatus();
        updateBreadcrumbStatus();
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
        editor->setProperty("documentKind", "location");
        editor->setProperty("documentId", locationId);
        editor->setProperty("baseTabTitle", tabTitle);
        editor->setProperty("isDirty", false);
        editor->setContent(l.notes);
        int idx = m_editorTabs->addTab(editor, tabTitle);
        m_editorTabs->setCurrentIndex(idx);

        connect(editor, &NovelEditor::contentChanged, this,
            [this, locationId](const QString &text) {
                for (auto &l2 : m_project.locations) {
                    if (l2.id == locationId) l2.notes = text;
                }
                markAllTabsDirty();
                updateStatusBar();
            });
        connect(editor, &QPlainTextEdit::cursorPositionChanged,
                this, &MainWindow::updateCursorStatus);
        refreshExplorerOpenDocuments();
        updateCursorStatus();
        updateBreadcrumbStatus();
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
            dirty ? title + QStringLiteral(" *") : title,
            detail,
            dirty
        });
    }

    m_structurePanel->setOpenDocuments(documents);

    auto *currentEditor = qobject_cast<NovelEditor*>(m_editorTabs->currentWidget());
    if (!currentEditor)
        return;

    const QString currentKind = currentEditor->property("documentKind").toString();
    const QString currentId = currentEditor->property("documentId").toString();
    if (!currentKind.isEmpty() && !currentId.isEmpty())
        m_structurePanel->selectOpenDocument(currentKind, currentId);
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

void MainWindow::openQuickOpenResult(const QString &kind, const QString &id)
{
    if (kind == "episode") {
        for (const auto &ch : m_project.chapters) {
            for (const auto &ep : ch.episodes) {
                if (ep.id == id) {
                    m_structurePanel->selectEpisode(ch.id, ep.id);
                    onEpisodeSelected(ch.id, ep.id);
                    return;
                }
            }
        }
    } else if (kind == "chapter") {
        m_structurePanel->selectChapter(id);
        return;
    } else if (kind == "character") {
        onCharacterSelected(id);
        return;
    } else if (kind == "location") {
        onLocationSelected(id);
        return;
    }
}

void MainWindow::addChapter()
{
    Chapter ch;
    ch.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    ch.title = "第" + QString::number(m_project.chapters.size() + 1) + "章";
    ch.sortOrder = m_project.chapters.size();
    m_project.chapters.append(ch);
    markAllTabsDirty();
    m_structurePanel->loadProject(m_project);
    updateStatusBar();
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
    markAllTabsDirty();
    m_structurePanel->loadProject(m_project);
    updateStatusBar();
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
            for (int i = 0; i < m_editorTabs->count(); ++i) {
                auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->widget(i));
                if (!editor) continue;
                for (const auto &ep : ch.episodes) {
                    if (ep.id != editor->sceneId()) continue;
                    const QString baseTitle = ch.title + " / " + ep.title;
                    editor->setProperty("baseTabTitle", baseTitle);
                    refreshTabTitle(editor);
                    break;
                }
            }
            markAllTabsDirty();
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
            for (int i = 0; i < m_editorTabs->count(); ++i) {
                auto *editor = qobject_cast<NovelEditor*>(m_editorTabs->widget(i));
                if (editor && editor->sceneId() == episodeId) {
                    editor->setProperty("baseTabTitle", ch.title + " / " + ep.title);
                    refreshTabTitle(editor);
                    break;
                }
            }
            markAllTabsDirty();
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
    markAllTabsDirty();
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
    markAllTabsDirty();
    m_notePanel->loadProject(m_project);
    updateStatusBar();
}

void MainWindow::addLocation()
{
    LocationEntry l;
    l.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    l.name = "新場所";
    m_project.locations.append(l);
    markAllTabsDirty();
    m_notePanel->loadProject(m_project);
    updateStatusBar();
}

void MainWindow::setCurrentProject(const NovelProject &project)
{
    m_project = project;
    markAllTabsClean();

    while (m_editorTabs->count() > 0) {
        auto *w = m_editorTabs->widget(0);
        m_editorTabs->removeTab(0);
        w->deleteLater();
    }
    refreshExplorerOpenDocuments();
    updateBreadcrumbStatus();
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
