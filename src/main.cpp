#include <QApplication>
#include "mainwindow.h"

///
/// \brief main
/// \param argc
/// \param argv
/// \return
///
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName(APP_NAME);
    a.setApplicationVersion(APP_VERSION);

    MainWindow window;
    window.show();

    return a.exec();
}
