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
    void restoreLastSessionDefaultsToFalse();
    void reconnectDefaultsToEveryFiveSeconds();
    void reconnectIntervalIsClampedToTheSupportedRange();
    void maxLogRowsRoundTripsAndIsClamped();
    void lastSavedSessionPathRoundTrips();
    void builtinSubscriptionOverridesRoundTrip();
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
/// \brief Session restoration is opt-in, defaulting to disabled.
///
void TestAppSettings::restoreLastSessionDefaultsToFalse()
{
    QVERIFY(!AppSettings().restoreLastSessionOnStartup());
    AppSettings().setRestoreLastSessionOnStartup(true);
    QVERIFY(AppSettings().restoreLastSessionOnStartup());
}

///
/// \brief Reconnecting after a connection loss is on by default, every five seconds.
///
void TestAppSettings::reconnectDefaultsToEveryFiveSeconds()
{
    QVERIFY(AppSettings().reconnectEnabled());
    QCOMPARE(AppSettings().reconnectIntervalSeconds(), 5);

    AppSettings().setReconnectEnabled(false);
    AppSettings().setReconnectIntervalSeconds(30);
    QVERIFY(!AppSettings().reconnectEnabled());
    QCOMPARE(AppSettings().reconnectIntervalSeconds(), 30);
}

///
/// \brief A stored reconnect interval never leaves the range the retry timer accepts.
///
void TestAppSettings::reconnectIntervalIsClampedToTheSupportedRange()
{
    AppSettings().setReconnectIntervalSeconds(0);
    QCOMPARE(AppSettings().reconnectIntervalSeconds(), 1);

    AppSettings().setReconnectIntervalSeconds(100000);
    QCOMPARE(AppSettings().reconnectIntervalSeconds(), 3600);
}

///
/// \brief The log depth defaults, round-trips, and never leaves the supported range.
///
void TestAppSettings::maxLogRowsRoundTripsAndIsClamped()
{
    QCOMPARE(AppSettings().maxLogRows(), AppSettings::defaultMaxLogRows);

    AppSettings().setMaxLogRows(250);
    QCOMPARE(AppSettings().maxLogRows(), 250);

    AppSettings().setMaxLogRows(0);
    QCOMPARE(AppSettings().maxLogRows(), AppSettings::minMaxLogRows);

    AppSettings().setMaxLogRows(AppSettings::maxMaxLogRows * 10);
    QCOMPARE(AppSettings().maxLogRows(), AppSettings::maxMaxLogRows);
}

///
/// \brief The named startup session path can be stored and cleared.
///
void TestAppSettings::lastSavedSessionPathRoundTrips()
{
    const QString path = QStringLiteral("/tmp/saved-session.ouas");
    QVERIFY(AppSettings().lastSavedSessionPath().isEmpty());
    AppSettings().setLastSavedSessionPath(path);
    QCOMPARE(AppSettings().lastSavedSessionPath(), path);
    AppSettings().setLastSavedSessionPath(QString());
    QVERIFY(AppSettings().lastSavedSessionPath().isEmpty());
}

///
/// \brief Built-in subscription overrides round-trip and stay separate from custom ones.
///
void TestAppSettings::builtinSubscriptionOverridesRoundTrip()
{
    QVERIFY(AppSettings().builtinSubscriptionOverrides().isEmpty());

    SubscriptionItem renamed;
    renamed.id = DefaultSubscriptionId;
    renamed.name = QStringLiteral("Telemetry");
    renamed.publishingInterval = 50.0;
    renamed.builtin = true;

    SubscriptionItem intervalOnly;
    intervalOnly.id = SlowSubscriptionId;
    intervalOnly.publishingInterval = 9000.0;
    intervalOnly.builtin = true;

    AppSettings().setBuiltinSubscriptionOverrides({renamed, intervalOnly});

    const QVector<SubscriptionItem> stored = AppSettings().builtinSubscriptionOverrides();
    QCOMPARE(stored.size(), 2);
    QCOMPARE(stored.at(0).id, int(DefaultSubscriptionId));
    QCOMPARE(stored.at(0).name, QStringLiteral("Telemetry"));
    QCOMPARE(stored.at(0).publishingInterval, 50.0);
    QVERIFY(stored.at(0).isBuiltin());

    // An empty name means only the interval was overridden; the factory name stays in effect.
    QCOMPARE(stored.at(1).id, int(SlowSubscriptionId));
    QVERIFY(stored.at(1).name.isEmpty());
    QCOMPARE(stored.at(1).publishingInterval, 9000.0);

    // Built-in overrides live in their own group and never leak into the custom list.
    QVERIFY(AppSettings().customSubscriptions().isEmpty());

    AppSettings().setBuiltinSubscriptionOverrides({});
    QVERIFY(AppSettings().builtinSubscriptionOverrides().isEmpty());
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
    settings.setTrendPanelVisible(false);
    settings.setViewState(QStringLiteral("dataView"), QByteArrayLiteral("view"));

    settings.clearLayout();

    QVERIFY(AppSettings().windowGeometry().isEmpty());
    QVERIFY(AppSettings().windowState().isEmpty());
    QVERIFY(AppSettings().centralSplitterState().isEmpty());
    QCOMPARE(AppSettings().dataAccessPage(), 0);
    QVERIFY(AppSettings().trendPanelVisible());
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
    QCOMPARE(qtOpcUa.size(), 2);
    QCOMPARE(qtOpcUa.first().key, QStringLiteral("plugin"));
    QCOMPARE(qtOpcUa.first().displayName, QStringLiteral("plugin"));
    QCOMPARE(qtOpcUa.first().categoryName,
             QStringLiteral("qt.opcua.plugins.open62541"));
    QCOMPARE(qtOpcUa.at(1).categoryName, QStringLiteral("qt.opcuagenericstructhandler"));
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
