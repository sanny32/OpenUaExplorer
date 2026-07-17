// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file appsettings.cpp
/// \brief Implements the central application settings store.
///

#include <QObject>
#include <QStringList>

#include "appsettings.h"
#include "settingsstore.h"

namespace {
constexpr auto themeModeKey = "appearance/themeMode";
constexpr auto timestampModeKey = "appearance/timestampMode";
constexpr auto languageKey = "appearance/language";
constexpr auto windowGeometryKey = "mainWindow/geometry";
constexpr auto windowStateKey = "mainWindow/state";
constexpr auto centralSplitterKey = "mainWindow/centralSplitter";
constexpr auto dataAccessPageKey = "mainWindow/dataAccessPage";
constexpr auto restoreLayoutKey = "mainWindow/restoreLayout";
constexpr auto viewStateGroup = "viewState";
constexpr auto sessionDefaultsGroup = "connectionDialog/sessionDefaults";
constexpr auto sessionTimeoutKey = "sessionTimeoutMs";
constexpr auto endpointTimeoutKey = "endpointTimeoutMs";
constexpr auto connectTimeoutKey = "connectTimeoutMs";
constexpr auto requestTimeoutKey = "requestTimeoutMs";
constexpr auto secureChannelLifetimeKey = "secureChannelLifetimeMs";
constexpr auto maxMessageSizeKey = "maxMessageSizeBytes";
constexpr auto loggingGroup = "logging";
constexpr auto subscriptionsGroup = "subscriptions/custom";
constexpr auto subscriptionNameKey = "name";
constexpr auto subscriptionIntervalKey = "interval";
}

///
/// \brief Returns the stored color-scheme preference.
/// \return Saved theme mode, or ThemeMode::System when none is stored.
///
AppSettings::ThemeMode AppSettings::themeMode() const
{
    SettingsStore settings;
    const int mode = settings.value(QLatin1String(themeModeKey),
                                    static_cast<int>(ThemeMode::System)).toInt();
    switch (mode) {
    case static_cast<int>(ThemeMode::Light):
        return ThemeMode::Light;
    case static_cast<int>(ThemeMode::Dark):
        return ThemeMode::Dark;
    default:
        return ThemeMode::System;
    }
}

///
/// \brief Stores the color-scheme preference.
/// \param mode Theme mode to persist.
///
void AppSettings::setThemeMode(ThemeMode mode)
{
    SettingsStore settings;
    settings.setValue(QLatin1String(themeModeKey), static_cast<int>(mode));
}

///
/// \brief Returns the stored OPC UA timestamp display preference.
/// \return Saved timestamp mode, or TimestampMode::Utc when none is stored.
///
AppSettings::TimestampMode AppSettings::timestampMode() const
{
    SettingsStore settings;
    const int mode = settings.value(QLatin1String(timestampModeKey),
                                    static_cast<int>(TimestampMode::Utc)).toInt();
    switch (mode) {
    case static_cast<int>(TimestampMode::LocalTime):
        return TimestampMode::LocalTime;
    default:
        return TimestampMode::Utc;
    }
}

///
/// \brief Stores the OPC UA timestamp display preference.
/// \param mode Timestamp mode to persist.
///
void AppSettings::setTimestampMode(TimestampMode mode)
{
    SettingsStore settings;
    settings.setValue(QLatin1String(timestampModeKey), static_cast<int>(mode));
}

///
/// \brief Returns the stored user interface language preference.
/// \return Saved language, or Language::System when none is stored.
///
AppSettings::Language AppSettings::language() const
{
    SettingsStore settings;
    const int language = settings.value(QLatin1String(languageKey),
                                        static_cast<int>(Language::System)).toInt();
    switch (language) {
    case static_cast<int>(Language::English):
        return Language::English;
    case static_cast<int>(Language::Russian):
        return Language::Russian;
    case static_cast<int>(Language::German):
        return Language::German;
    case static_cast<int>(Language::ChineseSimplified):
        return Language::ChineseSimplified;
    default:
        return Language::System;
    }
}

///
/// \brief Stores the user interface language preference.
/// \param language Language to persist.
///
void AppSettings::setLanguage(Language language)
{
    SettingsStore settings;
    settings.setValue(QLatin1String(languageKey), static_cast<int>(language));
}

///
/// \brief Returns the catalogue of configurable application logging categories.
/// \return Ordered list of every application category the user can toggle.
///
QVector<AppSettings::LogCategory> AppSettings::availableApplicationLogCategories()
{
    return {
        { QStringLiteral("application.app"),          QStringLiteral("ouaexp.App"),          QStringLiteral("App"),           true },
        { QStringLiteral("application.addressspace"), QStringLiteral("ouaexp.AddressSpace"), QStringLiteral("Address Space"), true },
        { QStringLiteral("application.attribute"),    QStringLiteral("ouaexp.Attribute"),    QStringLiteral("Attribute"),     true },
        { QStringLiteral("application.client"),       QStringLiteral("ouaexp.Client"),       QStringLiteral("Client"),        true },
        { QStringLiteral("application.dataaccess"),   QStringLiteral("ouaexp.DataAccess"),   QStringLiteral("Data Access"),   true },
        { QStringLiteral("application.reference"),    QStringLiteral("ouaexp.Reference"),    QStringLiteral("Reference"),     true },
        { QStringLiteral("application.server"),       QStringLiteral("ouaexp.Server"),       QStringLiteral("Server"),        true },
        { QStringLiteral("application.session"),      QStringLiteral("ouaexp.Session"),      QStringLiteral("Session"),       true }
    };
}

///
/// \brief Returns the catalogue of configurable Qt OPC UA plugin logging categories.
/// \return Ordered list of every Qt OPC UA plugin category the user can toggle.
///
QVector<AppSettings::LogCategory> AppSettings::availableQtOpcUaLogCategories()
{
    return {
        { QStringLiteral("plugin"),
          QStringLiteral("qt.opcua.plugins.open62541"),
          QStringLiteral("plugin"), true }
    };
}

///
/// \brief Returns the catalogue of configurable open62541 SDK logging categories.
/// \return Ordered list of every open62541 SDK category the user can toggle.
///
QVector<AppSettings::LogCategory> AppSettings::availableOpen62541LogCategories()
{
    const auto sdk = [](const char *suffix) {
        return QStringLiteral("qt.opcua.plugins.open62541.sdk.") + QLatin1String(suffix);
    };
    return {
        { QStringLiteral("network"),        sdk("network"),        QStringLiteral("network"),         false },
        { QStringLiteral("securechannel"),  sdk("securechannel"),  QStringLiteral("secure channel"),  false },
        { QStringLiteral("session"),        sdk("session"),        QStringLiteral("session"),         true  },
        { QStringLiteral("server"),         sdk("server"),         QStringLiteral("server"),          true  },
        { QStringLiteral("client"),         sdk("client"),         QStringLiteral("client"),          false },
        { QStringLiteral("userland"),       sdk("userland"),       QStringLiteral("userland"),        true  },
        { QStringLiteral("securitypolicy"), sdk("securitypolicy"), QStringLiteral("security policy"), true  },
        { QStringLiteral("eventloop"),      sdk("eventloop"),      QStringLiteral("event loop"),      false },
        { QStringLiteral("pubsub"),         sdk("pubsub"),         QStringLiteral("pubsub"),          true  },
        { QStringLiteral("discovery"),      sdk("discovery"),      QStringLiteral("discovery"),       true  }
    };
}

///
/// \brief Returns the catalogue of every configurable logging category.
/// \return Ordered list of every category the user can toggle.
///
QVector<AppSettings::LogCategory> AppSettings::availableLogCategories()
{
    QVector<LogCategory> categories = availableApplicationLogCategories();
    const QVector<LogCategory> qtOpcUaCategories = availableQtOpcUaLogCategories();
    for (const LogCategory &category : qtOpcUaCategories)
        categories.append(category);
    const QVector<LogCategory> open62541Categories = availableOpen62541LogCategories();
    for (const LogCategory &category : open62541Categories)
        categories.append(category);
    return categories;
}

///
/// \brief Returns the enabled state of every configurable logging category.
/// \return Map from category key to enabled state, falling back to per-category defaults.
///
QHash<QString, bool> AppSettings::logCategoryStates() const
{
    SettingsStore settings;
    settings.beginGroup(QLatin1String(loggingGroup));
    QHash<QString, bool> states;
    const QVector<LogCategory> categories = availableLogCategories();
    for (const LogCategory &category : categories)
        states.insert(category.key,
                      settings.value(category.key, category.defaultEnabled).toBool());
    return states;
}

///
/// \brief Stores the enabled state of the configurable logging categories.
/// \param states Map from category key to enabled state.
///
void AppSettings::setLogCategoryStates(const QHash<QString, bool> &states)
{
    SettingsStore settings;
    settings.beginGroup(QLatin1String(loggingGroup));
    for (auto it = states.cbegin(); it != states.cend(); ++it)
        settings.setValue(it.key(), it.value());
}

///
/// \brief Builds the QLoggingCategory filter rules from the stored preferences.
/// \return Newline-separated rule string suitable for QLoggingCategory::setFilterRules().
///
QString AppSettings::logFilterRules() const
{
    const QHash<QString, bool> states = logCategoryStates();
    QStringList rules;
    rules << QStringLiteral("ouaexp.*=true");
    rules << QStringLiteral("qt.opcua.*=true");
    const QVector<LogCategory> categories = availableLogCategories();
    for (const LogCategory &category : categories) {
        const bool enabled = states.value(category.key, category.defaultEnabled);
        rules << category.categoryName + QLatin1Char('=')
                     + (enabled ? QStringLiteral("true") : QStringLiteral("false"));
    }
    return rules.join(QLatin1Char('\n'));
}

///
/// \brief Returns the stored default session settings for new connections.
/// \return Saved session defaults, or built-in defaults when none are stored.
///
AppSettings::SessionDefaults AppSettings::sessionDefaults() const
{
    SettingsStore settings;
    settings.beginGroup(QLatin1String(sessionDefaultsGroup));
    SessionDefaults defaults;
    defaults.sessionTimeoutMs =
        settings.value(QLatin1String(sessionTimeoutKey), defaults.sessionTimeoutMs).toInt();
    defaults.endpointTimeoutMs =
        settings.value(QLatin1String(endpointTimeoutKey), defaults.endpointTimeoutMs).toInt();
    defaults.connectTimeoutMs =
        settings.value(QLatin1String(connectTimeoutKey), defaults.connectTimeoutMs).toInt();
    defaults.requestTimeoutMs =
        settings.value(QLatin1String(requestTimeoutKey), defaults.requestTimeoutMs).toInt();
    defaults.secureChannelLifetimeMs =
        settings.value(QLatin1String(secureChannelLifetimeKey), defaults.secureChannelLifetimeMs).toInt();
    defaults.maxMessageSizeBytes =
        settings.value(QLatin1String(maxMessageSizeKey), defaults.maxMessageSizeBytes).toInt();
    return defaults;
}

///
/// \brief Stores the default session settings for new connections.
/// \param defaults Session defaults to persist.
///
void AppSettings::setSessionDefaults(const SessionDefaults &defaults)
{
    SettingsStore settings;
    settings.beginGroup(QLatin1String(sessionDefaultsGroup));
    settings.setValue(QLatin1String(sessionTimeoutKey), defaults.sessionTimeoutMs);
    settings.setValue(QLatin1String(endpointTimeoutKey), defaults.endpointTimeoutMs);
    settings.setValue(QLatin1String(connectTimeoutKey), defaults.connectTimeoutMs);
    settings.setValue(QLatin1String(requestTimeoutKey), defaults.requestTimeoutMs);
    settings.setValue(QLatin1String(secureChannelLifetimeKey), defaults.secureChannelLifetimeMs);
    settings.setValue(QLatin1String(maxMessageSizeKey), defaults.maxMessageSizeBytes);
}

///
/// \brief Returns the saved top-level window geometry.
/// \return Window geometry blob, or an empty array when none is stored.
///
QByteArray AppSettings::windowGeometry() const
{
    SettingsStore settings;
    return settings.value(QLatin1String(windowGeometryKey)).toByteArray();
}

///
/// \brief Stores the top-level window geometry.
/// \param geometry Geometry blob from QWidget::saveGeometry().
///
void AppSettings::setWindowGeometry(const QByteArray &geometry)
{
    SettingsStore settings;
    settings.setValue(QLatin1String(windowGeometryKey), geometry);
}

///
/// \brief Returns the saved dock and toolbar layout state.
/// \return Window state blob, or an empty array when none is stored.
///
QByteArray AppSettings::windowState() const
{
    SettingsStore settings;
    return settings.value(QLatin1String(windowStateKey)).toByteArray();
}

///
/// \brief Stores the dock and toolbar layout state.
/// \param state State blob from QMainWindow::saveState().
///
void AppSettings::setWindowState(const QByteArray &state)
{
    SettingsStore settings;
    settings.setValue(QLatin1String(windowStateKey), state);
}

///
/// \brief Returns the saved central splitter state.
/// \return Splitter state blob, or an empty array when none is stored.
///
QByteArray AppSettings::centralSplitterState() const
{
    SettingsStore settings;
    return settings.value(QLatin1String(centralSplitterKey)).toByteArray();
}

///
/// \brief Stores the central splitter state.
/// \param state State blob from QSplitter::saveState().
///
void AppSettings::setCentralSplitterState(const QByteArray &state)
{
    SettingsStore settings;
    settings.setValue(QLatin1String(centralSplitterKey), state);
}

///
/// \brief Returns the saved data-access tab page index.
/// \return Stored page index, or 0 when none is stored.
///
int AppSettings::dataAccessPage() const
{
    SettingsStore settings;
    return settings.value(QLatin1String(dataAccessPageKey), 0).toInt();
}

///
/// \brief Stores the data-access tab page index.
/// \param page Page index to persist.
///
void AppSettings::setDataAccessPage(int page)
{
    SettingsStore settings;
    settings.setValue(QLatin1String(dataAccessPageKey), page);
}

///
/// \brief Reports whether the saved window layout should be restored at startup.
/// \return True when the layout should be restored, defaulting to true.
///
bool AppSettings::restoreLayoutOnStartup() const
{
    SettingsStore settings;
    return settings.value(QLatin1String(restoreLayoutKey), true).toBool();
}

///
/// \brief Stores whether the saved window layout should be restored at startup.
/// \param enabled True to restore the layout on the next launch.
///
void AppSettings::setRestoreLayoutOnStartup(bool enabled)
{
    SettingsStore settings;
    settings.setValue(QLatin1String(restoreLayoutKey), enabled);
}

///
/// \brief Returns the user-created subscriptions persisted from the last session.
/// \return Custom subscriptions in stored order, or an empty vector when none are stored.
///
QVector<SubscriptionItem> AppSettings::customSubscriptions() const
{
    SettingsStore settings;
    QVector<SubscriptionItem> subscriptions;
    const int count = settings.beginReadArray(QLatin1String(subscriptionsGroup));
    subscriptions.reserve(count);
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        SubscriptionItem item;
        item.name = settings.value(QLatin1String(subscriptionNameKey)).toString();
        item.publishingInterval =
            settings.value(QLatin1String(subscriptionIntervalKey), 1000.0).toDouble();
        if (!item.name.isEmpty())
            subscriptions.append(item);
    }
    settings.endArray();
    return subscriptions;
}

///
/// \brief Stores the user-created subscriptions to restore on the next launch.
/// \param subscriptions Custom subscriptions to persist; built-in ones are ignored.
///
void AppSettings::setCustomSubscriptions(const QVector<SubscriptionItem> &subscriptions)
{
    SettingsStore settings;
    settings.remove(QLatin1String(subscriptionsGroup));
    settings.beginWriteArray(QLatin1String(subscriptionsGroup));
    int index = 0;
    for (const SubscriptionItem &item : subscriptions) {
        if (item.isBuiltin())
            continue;
        settings.setArrayIndex(index++);
        settings.setValue(QLatin1String(subscriptionNameKey), item.name);
        settings.setValue(QLatin1String(subscriptionIntervalKey), item.publishingInterval);
    }
    settings.endArray();
}

///
/// \brief Returns the saved element state for a named view.
/// \param viewKey Stable identifier of the view (its object name).
/// \return State blob, or an empty array when none is stored.
///
QByteArray AppSettings::viewState(const QString &viewKey) const
{
    SettingsStore settings;
    settings.beginGroup(QLatin1String(viewStateGroup));
    return settings.value(viewKey).toByteArray();
}

///
/// \brief Stores the element state for a named view.
/// \param viewKey Stable identifier of the view (its object name).
/// \param state State blob produced by the view.
///
void AppSettings::setViewState(const QString &viewKey, const QByteArray &state)
{
    SettingsStore settings;
    settings.beginGroup(QLatin1String(viewStateGroup));
    settings.setValue(viewKey, state);
}

///
/// \brief Removes the saved window geometry, layout, and per-view element state.
///
/// Leaves user preferences (theme, restore-on-startup) untouched.
///
void AppSettings::clearLayout()
{
    SettingsStore settings;
    settings.remove(QLatin1String(windowGeometryKey));
    settings.remove(QLatin1String(windowStateKey));
    settings.remove(QLatin1String(centralSplitterKey));
    settings.remove(QLatin1String(dataAccessPageKey));
    settings.remove(QLatin1String(viewStateGroup));
}
