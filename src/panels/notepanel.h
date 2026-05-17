#pragma once

#include <QWidget>
#include <QTabWidget>
#include <QTreeWidget>
#include "wobjectdefs.h"

#include "ui/dockpane.h"
#include "core/novelproject.h"

class NotePanel : public QWidget {
    W_OBJECT(NotePanel)
public:
    explicit NotePanel(QWidget *parent = nullptr);

    static DockPaneSpec dockSpec();

    void loadProject(const NovelProject &project);

public:
    void characterSelected(const QString &characterId)
    W_SIGNAL(characterSelected, (const QString &), characterId)
    void locationSelected(const QString &locationId)
    W_SIGNAL(locationSelected, (const QString &), locationId)

private:
    void onCharacterDoubleClick(QTreeWidgetItem *item, int column);
    W_SLOT(onCharacterDoubleClick, (QTreeWidgetItem *, int))
    void onLocationDoubleClick(QTreeWidgetItem *item, int column);
    W_SLOT(onLocationDoubleClick, (QTreeWidgetItem *, int))

private:
    QTabWidget *m_tabs = nullptr;
    QTreeWidget *m_characterTree = nullptr;
    QTreeWidget *m_locationTree = nullptr;
    NovelProject m_project;
};
