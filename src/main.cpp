#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("PlotEngine");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("PlotEngine");

    MainWindow window;
    window.setWindowTitle("PlotEngine - 小説制作IDE");
    window.resize(1400, 900);
    window.show();

    return app.exec();
}
