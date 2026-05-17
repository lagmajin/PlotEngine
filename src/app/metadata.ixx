module;

#include <QSettings>
#include <QString>

export module PlotEngine.App.Metadata;

export namespace PlotEngine::App {

QString applicationName()
{
    return QStringLiteral("PlotEngine");
}

QString applicationVersion()
{
    return QStringLiteral("1.0.0");
}

QString organizationName()
{
    return QStringLiteral("PlotEngine");
}

QString mainWindowTitle()
{
    return QStringLiteral("PlotEngine - 小説制作IDE");
}

QSettings makeSettings()
{
    return QSettings(organizationName(), applicationName());
}

}
