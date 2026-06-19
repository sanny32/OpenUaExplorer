// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_appsettings.cpp
/// \brief Tests round-trip behaviour and defaults of AppSettings.
///

#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

#include "appsettings.h"

///
/// \brief Unit tests for the central application settings store.
///
class TestAppSettings : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void themeModeDefaultsToSystem();
    void themeModeRoundTrips();
    void windowStateRoundTrips();
    void restoreLayoutDefaultsToTrue();
    void dataAccessPageRoundTrips();
    void viewStateRoundTrips();
    void clearLayoutKeepsPreferences();

private:
    QTemporaryDir _settingsDirectory;
};

///
/// \brief Routes QSettings to a throwaway directory so tests never touch the
///        real user configuration.
///
void TestAppSettings::initTestCase()
{
    QVERIFY(_settingsDirectory.isValid());
    QCoreApplication::setOrganizationName(QStringLiteral("OpenUaExplorerTests"));
    QCoreApplication::setApplicationName(QStringLiteral("OpenUaExplorerTests"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       _settingsDirectory.path());
}

///
/// \brief Clears all stored settings between tests to keep them independent.
///
void TestAppSettings::cleanup()
{
    QSettings settings;
    settings.clear();
}

///
/// \brief A fresh store reports the system theme mode.
///
void TestAppSettings::themeModeDefaultsToSystem()
{
    AppSettings settings;
    QCOMPARE(settings.themeMode(), AppSettings::ThemeMode::System);
}

///
/// \brief The theme mode survives a save/load cycle.
///
void TestAppSettings::themeModeRoundTrips()
{
    AppSettings settings;
    settings.setThemeMode(AppSettings::ThemeMode::Dark);
    QCOMPARE(AppSettings().themeMode(), AppSettings::ThemeMode::Dark);
    settings.setThemeMode(AppSettings::ThemeMode::Light);
    QCOMPARE(AppSettings().themeMode(), AppSettings::ThemeMode::Light);
}

///
/// \brief Window geometry and state blobs round-trip byte-for-byte.
///
void TestAppSettings::windowStateRoundTrips()
{
    const QByteArray geometry = QByteArrayLiteral("\x01\x02geometry");
    const QByteArray state = QByteArrayLiteral("\x03\x04state");
    const QByteArray splitter = QByteArrayLiteral("splitter-blob");

    AppSettings settings;
    settings.setWindowGeometry(geometry);
    settings.setWindowState(state);
    settings.setCentralSplitterState(splitter);

    QCOMPARE(AppSettings().windowGeometry(), geometry);
    QCOMPARE(AppSettings().windowState(), state);
    QCOMPARE(AppSettings().centralSplitterState(), splitter);
}

///
/// \brief Layout restoration is opt-out, defaulting to enabled.
///
void TestAppSettings::restoreLayoutDefaultsToTrue()
{
    QVERIFY(AppSettings().restoreLayoutOnStartup());
    AppSettings().setRestoreLayoutOnStartup(false);
    QVERIFY(!AppSettings().restoreLayoutOnStartup());
}

///
/// \brief The data-access page index round-trips and defaults to zero.
///
void TestAppSettings::dataAccessPageRoundTrips()
{
    QCOMPARE(AppSettings().dataAccessPage(), 0);
    AppSettings().setDataAccessPage(3);
    QCOMPARE(AppSettings().dataAccessPage(), 3);
}

///
/// \brief Per-view element state is keyed independently and round-trips.
///
void TestAppSettings::viewStateRoundTrips()
{
    const QByteArray dataState = QByteArrayLiteral("data-view-state");
    const QByteArray logState = QByteArrayLiteral("log-view-state");

    AppSettings settings;
    settings.setViewState(QStringLiteral("dataView"), dataState);
    settings.setViewState(QStringLiteral("logTable"), logState);

    QCOMPARE(AppSettings().viewState(QStringLiteral("dataView")), dataState);
    QCOMPARE(AppSettings().viewState(QStringLiteral("logTable")), logState);
    QVERIFY(AppSettings().viewState(QStringLiteral("missing")).isEmpty());
}

///
/// \brief clearLayout() removes layout data but keeps user preferences.
///
void TestAppSettings::clearLayoutKeepsPreferences()
{
    AppSettings settings;
    settings.setThemeMode(AppSettings::ThemeMode::Dark);
    settings.setRestoreLayoutOnStartup(false);
    settings.setWindowGeometry(QByteArrayLiteral("geometry"));
    settings.setWindowState(QByteArrayLiteral("state"));
    settings.setCentralSplitterState(QByteArrayLiteral("splitter"));
    settings.setDataAccessPage(2);
    settings.setViewState(QStringLiteral("dataView"), QByteArrayLiteral("view"));

    settings.clearLayout();

    QVERIFY(AppSettings().windowGeometry().isEmpty());
    QVERIFY(AppSettings().windowState().isEmpty());
    QVERIFY(AppSettings().centralSplitterState().isEmpty());
    QCOMPARE(AppSettings().dataAccessPage(), 0);
    QVERIFY(AppSettings().viewState(QStringLiteral("dataView")).isEmpty());

    QCOMPARE(AppSettings().themeMode(), AppSettings::ThemeMode::Dark);
    QVERIFY(!AppSettings().restoreLayoutOnStartup());
}

QTEST_MAIN(TestAppSettings)
#include "test_appsettings.moc"
