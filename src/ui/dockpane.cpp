#include "DockManager.h"
#include "DockWidget.h"

#include <QWidget>

#include "ui/dockpane.h"

namespace PlotEngine::UI {

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
        manager->setCentralWidget(dock);
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

void showDockPane(ads::CDockWidget *dock)
{
    if (!dock)
        return;

    dock->toggleView(true);
    dock->raise();
}

void floatDockPane(ads::CDockWidget *dock)
{
    if (!dock)
        return;

    dock->toggleView(true);
    if (!dock->isFloating())
        dock->setFloating();
    dock->raise();
}

void dockDockPane(ads::CDockManager *manager, ads::CDockWidget *dock, DockPlacement placement)
{
    if (!manager || !dock)
        return;

    switch (placement) {
    case DockPlacement::Left:
        manager->addDockWidget(ads::LeftDockWidgetArea, dock);
        break;
    case DockPlacement::Center:
        manager->setCentralWidget(dock);
        break;
    case DockPlacement::Right:
        manager->addDockWidget(ads::RightDockWidgetArea, dock);
        break;
    case DockPlacement::Bottom:
        manager->addDockWidget(ads::BottomDockWidgetArea, dock);
        break;
    }

    dock->toggleView(true);
    dock->raise();
}

}
