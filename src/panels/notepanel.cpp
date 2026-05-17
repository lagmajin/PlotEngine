#include "notepanel.h"
#include "wobjectimpl.h"
#include <QVBoxLayout>

W_OBJECT_IMPL(NotePanel)

DockPaneSpec NotePanel::dockSpec()
{
    return { QStringLiteral("ノート"), DockPlacement::Right, true };
}

NotePanel::NotePanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_tabs = new QTabWidget(this);
    m_tabs->setStyleSheet(
        "QTabWidget::pane { background: #1e1e2e; border: none; }"
        "QTabBar::tab { background: #181825; color: #a6adc8; padding: 4px 8px; }"
        "QTabBar::tab:selected { color: #cdd6f4; }"
    );

    m_characterTree = new QTreeWidget(this);
    m_characterTree->setHeaderLabels({"名前", "役割"});
    m_characterTree->setRootIsDecorated(false);
    m_characterTree->setStyleSheet(
        "QTreeWidget { background: #1e1e2e; color: #cdd6f4; border: none; }"
        "QTreeWidget::item:selected { background: #45475a; }"
    );
    connect(m_characterTree, &QTreeWidget::itemDoubleClicked,
            this, &NotePanel::onCharacterDoubleClick);

    m_locationTree = new QTreeWidget(this);
    m_locationTree->setHeaderLabels({"場所", "説明"});
    m_locationTree->setRootIsDecorated(false);
    m_locationTree->setStyleSheet(
        "QTreeWidget { background: #1e1e2e; color: #cdd6f4; border: none; }"
        "QTreeWidget::item:selected { background: #45475a; }"
    );
    connect(m_locationTree, &QTreeWidget::itemDoubleClicked,
            this, &NotePanel::onLocationDoubleClick);

    m_tabs->addTab(m_characterTree, "キャラクター");
    m_tabs->addTab(m_locationTree, "場所");
    layout->addWidget(m_tabs);
}

void NotePanel::loadProject(const NovelProject &project)
{
    m_project = project;

    m_characterTree->clear();
    for (const auto &c : project.characters) {
        auto *item = new QTreeWidgetItem({c.name, c.role});
        item->setData(0, Qt::UserRole, c.id);
        m_characterTree->addTopLevelItem(item);
    }

    m_locationTree->clear();
    for (const auto &l : project.locations) {
        auto *item = new QTreeWidgetItem({l.name, l.description.left(30)});
        item->setData(0, Qt::UserRole, l.id);
        m_locationTree->addTopLevelItem(item);
    }
}

void NotePanel::onCharacterDoubleClick(QTreeWidgetItem *item, int)
{
    if (item) emit characterSelected(item->data(0, Qt::UserRole).toString());
}

void NotePanel::onLocationDoubleClick(QTreeWidgetItem *item, int)
{
    if (item) emit locationSelected(item->data(0, Qt::UserRole).toString());
}
