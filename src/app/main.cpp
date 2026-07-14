// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include <QLoggingCategory>

#include "application.h"
#include "appsettings.h"
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

    QLoggingCategory::setFilterRules(AppSettings().logFilterRules());

    MainWindow window;
    window.show();

    // Connected here rather than inside MainWindow: the window is also built by tests,
    // which run under a plain QApplication that has no such signal to connect to.
    QObject::connect(&a, &Application::sessionFileRequested,
                     &window, &MainWindow::openSessionFile);
    a.deliverPendingSessionFile();

    return a.exec();
}
