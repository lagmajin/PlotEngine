#pragma once

#include <QSettings>
#include <QString>

namespace PlotEngine::App {

QString applicationName();
QString applicationVersion();
QString organizationName();
QString mainWindowTitle();
QSettings makeSettings();

}
