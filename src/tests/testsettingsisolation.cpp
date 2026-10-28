// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file testsettingsisolation.cpp
/// \brief Redirects settings and standard paths for every test executable.
///

#include <QCoreApplication>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>

namespace {

///
/// \brief Returns the settings directory kept alive for the test process.
/// \return Temporary settings directory.
///
QTemporaryDir &settingsDirectory()
{
    static QTemporaryDir directory;
    return directory;
}

///
/// \brief Redirects user settings before test application construction completes.
///
/// Test mode roots the standard paths below the per-test home ctest sets; macOS ignores it otherwise.
///
void isolateTestSettings()
{
    QTemporaryDir &directory = settingsDirectory();
    if (!directory.isValid())
        qFatal("Could not create a temporary settings directory");

    QCoreApplication::setOrganizationName(QStringLiteral("OpenUaExplorerTests"));
    QCoreApplication::setApplicationName(QStringLiteral("OpenUaExplorerTests"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, directory.path());
    QStandardPaths::setTestModeEnabled(true);
}

Q_COREAPP_STARTUP_FUNCTION(isolateTestSettings)

}
