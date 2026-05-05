#include <QApplication>
#include "appstyle.h"
#include "apptheme.h"
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
    a.setStyle(new AppStyle);
    a.setPalette(AppTheme::systemPalette());

    MainWindow window;
    window.show();

    return a.exec();
}
