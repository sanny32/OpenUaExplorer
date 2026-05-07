// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include "application.h"
#include "appstyle.h"
#include "mainwindow.h"

///
/// \brief Application entry point.
/// \param argc Argument count passed by the runtime.
/// \param argv Argument vector passed by the runtime.
/// \return Application exit code.
///
int main(int argc, char *argv[])
{
    Application a(argc, argv);
    a.setApplicationName(APP_NAME);
    a.setApplicationVersion(APP_VERSION);
    a.setStyle(new AppStyle(&a));
    a.theme()->applyInitialScheme();

    MainWindow window;
    window.show();

    return a.exec();
}
