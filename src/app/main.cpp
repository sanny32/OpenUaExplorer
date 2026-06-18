// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include <QLoggingCategory>

#include "application.h"
#include "mainwindow.h"

///
/// \brief Application entry point.
/// \param argc Argument count passed by the runtime.
/// \param argv Argument vector passed by the runtime.
/// \return Application exit code.
///
int main(int argc, char *argv[])
{
    QLoggingCategory::setFilterRules(QStringLiteral(
        "ouaexp.*=true\n"
        "qt.opcua.*=true\n"
        "qt.opcua.plugins.open62541.sdk.eventloop=false\n"
        "qt.opcua.plugins.open62541.sdk.client.debug=false"));

    Application a(argc, argv);
    a.setOrganizationName(APP_ORGANIZATION_NAME);
    a.setApplicationName(APP_NAME);
    a.setApplicationVersion(APP_VERSION);

    MainWindow window;
    window.show();

    return a.exec();
}
