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
#include "settingsstore.h"

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
    void timestampModeDefaultsToUtc();
    void timestampModeRoundTrips();
    void windowStateRoundTrips();
    void restoreLayoutDefaultsToTrue();
    void dataAccessPageRoundTrips();
    void viewStateRoundTrips();
    void clearLayoutKeepsPreferences();
    void loggingCategoriesAreGrouped();
    void applicationLoggingCategoryCanBeDisabled();

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
    SettingsStore settings;
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
/// \brief A fresh store reports UTC as the timestamp display mode.
///
void TestAppSettings::timestampModeDefaultsToUtc()
{
    AppSettings settings;
    QCOMPARE(settings.timestampMode(), AppSettings::TimestampMode::Utc);
}

///
/// \brief The timestamp mode survives a save/load cycle.
///
void TestAppSettings::timestampModeRoundTrips()
{
    AppSettings settings;
    settings.setTimestampMode(AppSettings::TimestampMode::LocalTime);
    QCOMPARE(AppSettings().timestampMode(), AppSettings::TimestampMode::LocalTime);
    settings.setTimestampMode(AppSettings::TimestampMode::Utc);
    QCOMPARE(AppSettings().timestampMode(), AppSettings::TimestampMode::Utc);
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

///
/// \brief Logging categories expose application, Qt OPC UA plugin and open62541 SDK groups.
///
void TestAppSettings::loggingCategoriesAreGrouped()
{
    const QVector<AppSettings::LogCategory> application =
        AppSettings::availableApplicationLogCategories();
    const QVector<AppSettings::LogCategory> qtOpcUa =
        AppSettings::availableQtOpcUaLogCategories();
    const QVector<AppSettings::LogCategory> open62541 =
        AppSettings::availableOpen62541LogCategories();
    const QVector<AppSettings::LogCategory> all =
        AppSettings::availableLogCategories();

    QCOMPARE(application.size(), 8);
    QCOMPARE(application.first().key, QStringLiteral("application.app"));
    QCOMPARE(application.first().categoryName, QStringLiteral("ouaexp.App"));
    QCOMPARE(qtOpcUa.size(), 1);
    QCOMPARE(qtOpcUa.first().key, QStringLiteral("plugin"));
    QCOMPARE(qtOpcUa.first().displayName, QStringLiteral("plugin"));
    QCOMPARE(qtOpcUa.first().categoryName,
             QStringLiteral("qt.opcua.plugins.open62541"));
    QVERIFY(!open62541.isEmpty());
    for (const AppSettings::LogCategory &category : open62541)
        QVERIFY(category.displayName.at(0).isLower());
    QVERIFY(open62541.first().categoryName.startsWith(
        QStringLiteral("qt.opcua.plugins.open62541.sdk.")));
    QCOMPARE(all.size(), application.size() + qtOpcUa.size() + open62541.size());
    QCOMPARE(all.first().key, application.first().key);
    QCOMPARE(all.at(application.size()).key, qtOpcUa.first().key);
    QCOMPARE(all.at(application.size() + qtOpcUa.size()).key, open62541.first().key);
}

///
/// \brief Application logging categories generate specific filter rules.
///
void TestAppSettings::applicationLoggingCategoryCanBeDisabled()
{
    QHash<QString, bool> states = AppSettings().logCategoryStates();
    states.insert(QStringLiteral("application.app"), false);
    AppSettings().setLogCategoryStates(states);

    const QString rules = AppSettings().logFilterRules();
    QVERIFY(rules.contains(QStringLiteral("ouaexp.*=true")));
    QVERIFY(rules.contains(QStringLiteral("ouaexp.App=false")));
}

QTEST_MAIN(TestAppSettings)
#include "test_appsettings.moc"
