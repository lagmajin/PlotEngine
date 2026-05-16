module;

#include <QApplication>
#include <QIcon>
#include <QSize>
#include <QString>
#include "mainwindow.h"

export module PlotEngine.App.Application;

import std;
import PlotEngine.App.Metadata;

namespace {

QIcon plotEngineIcon()
{
    QIcon icon;
    for (int size : {16, 24, 32, 48, 64, 128, 256})
        icon.addFile(QString(":/icons/plotengine-%1.png").arg(size), QSize(size, size));
    return icon;
}

}

export int runPlotEngineApplication(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(PlotEngine::App::applicationName());
    app.setApplicationVersion(PlotEngine::App::applicationVersion());
    app.setOrganizationName(PlotEngine::App::organizationName());
    app.setWindowIcon(plotEngineIcon());

    MainWindow window;
    window.setWindowTitle(PlotEngine::App::mainWindowTitle());
    window.setWindowIcon(plotEngineIcon());
    window.resize(1400, 900);
    window.show();

    return app.exec();
}
