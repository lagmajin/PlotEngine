#pragma once

#include <QString>

class QWidget;

namespace ads {
class CDockManager;
class CDockWidget;
}

enum class DockPlacement {
    Left,
    Center,
    Right,
    Bottom
};

struct DockPaneSpec {
    QString title;
    DockPlacement placement = DockPlacement::Left;
    bool visibleByDefault = true;
};

namespace PlotEngine::UI {

ads::CDockWidget *createDockPane(ads::CDockManager *manager, const DockPaneSpec &spec, QWidget *widget);
void showDockPane(ads::CDockWidget *dock);
void floatDockPane(ads::CDockWidget *dock);
void dockDockPane(ads::CDockManager *manager, ads::CDockWidget *dock, DockPlacement placement);

}
