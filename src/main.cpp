// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file main.cpp
/// \brief Defines the application entry point.
///

#include <QApplication>
#include "appstyle.h"
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

    MainWindow window;
    window.show();

    return a.exec();
}
