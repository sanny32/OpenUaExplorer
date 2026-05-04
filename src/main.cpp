#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("OpenUaExplorer");
    QApplication::setOrganizationName("OpenUaExplorer");

    MainWindow window;
    window.show();

    return app.exec();
}
