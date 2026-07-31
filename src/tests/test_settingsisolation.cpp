// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_settingsisolation.cpp
/// \brief Tests process-wide settings isolation for test executables.
///

#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QTest>

#include "settingsstore.h"

///
/// \brief Verifies test settings cannot reach the real user configuration.
///
class TestSettingsIsolation : public QObject
{
    Q_OBJECT

private slots:
    void settingsUseTemporaryIniStorage();
};

///
/// \brief SettingsStore uses an INI file below the system temporary directory.
///
void TestSettingsIsolation::settingsUseTemporaryIniStorage()
{
    SettingsStore settings;
    QCOMPARE(settings.format(), QSettings::IniFormat);

    const QString temporaryRoot = QDir::cleanPath(QDir::tempPath()) + QLatin1Char('/');
    const QString settingsPath = QDir::cleanPath(QFileInfo(settings.fileName()).absoluteFilePath());
    QVERIFY2(settingsPath.startsWith(temporaryRoot), qPrintable(settingsPath));
}

QTEST_GUILESS_MAIN(TestSettingsIsolation)

#include "test_settingsisolation.moc"
