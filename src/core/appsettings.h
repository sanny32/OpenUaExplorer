// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file appsettings.h
/// \brief Declares the central application settings store.
///

#pragma once

#include <QByteArray>
#include <QHash>
#include <QString>
#include <QVector>

#include "models/subscriptionitem.h"

///
/// \brief Typed facade over QSettings for application preferences and UI state.
///
/// Groups every persisted preference (color scheme, window geometry, dock and
/// toolbar layout, and per-view element state) behind named accessors so the
/// rest of the code never deals with raw settings keys.
///
class AppSettings
{
public:
    ///
    /// \brief User color-scheme preference.
    ///
    enum class ThemeMode {
        System,
        Light,
        Dark
    };

    ///
    /// \brief Display mode for OPC UA timestamps.
    ///
    enum class TimestampMode {
        LocalTime,
        Utc
    };

    ///
    /// \brief User interface language preference.
    ///
    enum class Language {
        System,
        English,
        Russian,
        German,
        ChineseSimplified
    };

    ///
    /// \brief Default OPC UA session and channel settings for new connections.
    ///
    /// Mirrors the Advanced Settings group of the connection dialog so the last
    /// values a user entered are offered again the next time they open it.
    ///
    struct SessionDefaults {
        /// \brief Session timeout in milliseconds.
        int sessionTimeoutMs = 600000;

        /// \brief Endpoint discovery timeout in milliseconds.
        int endpointTimeoutMs = 10000;

        /// \brief Connection timeout in milliseconds.
        int connectTimeoutMs = 10000;

        /// \brief Read/write request timeout in milliseconds.
        int requestTimeoutMs = 5000;

        /// \brief Secure-channel lifetime in milliseconds.
        int secureChannelLifetimeMs = 600000;

        /// \brief Maximum message size in bytes.
        int maxMessageSizeBytes = 4194304;
    };

    ///
    /// \brief Describes one configurable logging category.
    ///
    struct LogCategory {
        /// \brief Stable settings key and identifier (e.g. "network").
        QString key;

        /// \brief Full Qt logging category name used in filter rules.
        QString categoryName;

        /// \brief Human-readable label shown in the settings dialog.
        QString displayName;
        
        /// \brief Whether the category is enabled when the user has no stored preference.
        bool defaultEnabled = true;
    };

    ///
    /// \brief Returns the catalogue of configurable application logging categories.
    /// \return Ordered list of every application category the user can toggle.
    ///
    static QVector<LogCategory> availableApplicationLogCategories();

    ///
    /// \brief Returns the catalogue of configurable Qt OPC UA plugin logging categories.
    /// \return Ordered list of every Qt OPC UA plugin category the user can toggle.
    ///
    static QVector<LogCategory> availableQtOpcUaLogCategories();

    ///
    /// \brief Returns the catalogue of configurable open62541 SDK logging categories.
    /// \return Ordered list of every open62541 SDK category the user can toggle.
    ///
    static QVector<LogCategory> availableOpen62541LogCategories();

    ///
    /// \brief Returns the catalogue of every configurable logging category.
    /// \return Ordered list of every category the user can toggle.
    ///
    static QVector<LogCategory> availableLogCategories();

    ///
    /// \brief Returns the enabled state of every configurable logging category.
    /// \return Map from category key to enabled state, falling back to per-category defaults.
    ///
    QHash<QString, bool> logCategoryStates() const;

    ///
    /// \brief Stores the enabled state of the configurable logging categories.
    /// \param states Map from category key to enabled state.
    ///
    void setLogCategoryStates(const QHash<QString, bool> &states);

    ///
    /// \brief Builds the QLoggingCategory filter rules from the stored preferences.
    /// \return Newline-separated rule string suitable for QLoggingCategory::setFilterRules().
    ///
    QString logFilterRules() const;

    ///
    /// \brief Returns the stored default session settings for new connections.
    /// \return Saved session defaults, or built-in defaults when none are stored.
    ///
    SessionDefaults sessionDefaults() const;

    ///
    /// \brief Stores the default session settings for new connections.
    /// \param defaults Session defaults to persist.
    ///
    void setSessionDefaults(const SessionDefaults &defaults);

    ///
    /// \brief Returns the stored color-scheme preference.
    /// \return Saved theme mode, or ThemeMode::System when none is stored.
    ///
    ThemeMode themeMode() const;

    ///
    /// \brief Stores the color-scheme preference.
    /// \param mode Theme mode to persist.
    ///
    void setThemeMode(ThemeMode mode);

    ///
    /// \brief Returns the stored OPC UA timestamp display preference.
    /// \return Saved timestamp mode, or TimestampMode::Utc when none is stored.
    ///
    TimestampMode timestampMode() const;

    ///
    /// \brief Stores the OPC UA timestamp display preference.
    /// \param mode Timestamp mode to persist.
    ///
    void setTimestampMode(TimestampMode mode);

    ///
    /// \brief Returns the stored user interface language preference.
    /// \return Saved language, or Language::System when none is stored.
    ///
    Language language() const;

    ///
    /// \brief Stores the user interface language preference.
    /// \param language Language to persist.
    ///
    void setLanguage(Language language);

    ///
    /// \brief Returns the saved top-level window geometry.
    /// \return Window geometry blob, or an empty array when none is stored.
    ///
    QByteArray windowGeometry() const;

    ///
    /// \brief Stores the top-level window geometry.
    /// \param geometry Geometry blob from QWidget::saveGeometry().
    ///
    void setWindowGeometry(const QByteArray &geometry);

    ///
    /// \brief Returns the saved dock and toolbar layout state.
    /// \return Window state blob, or an empty array when none is stored.
    ///
    QByteArray windowState() const;

    ///
    /// \brief Stores the dock and toolbar layout state.
    /// \param state State blob from QMainWindow::saveState().
    ///
    void setWindowState(const QByteArray &state);

    ///
    /// \brief Returns the saved central splitter state.
    /// \return Splitter state blob, or an empty array when none is stored.
    ///
    QByteArray centralSplitterState() const;

    ///
    /// \brief Stores the central splitter state.
    /// \param state State blob from QSplitter::saveState().
    ///
    void setCentralSplitterState(const QByteArray &state);

    ///
    /// \brief Returns the saved data-access tab page index.
    /// \return Stored page index, or 0 when none is stored.
    ///
    int dataAccessPage() const;

    ///
    /// \brief Stores the data-access tab page index.
    /// \param page Page index to persist.
    ///
    void setDataAccessPage(int page);

    ///
    /// \brief Reports whether the saved window layout should be restored at startup.
    /// \return True when the layout should be restored, defaulting to true.
    ///
    bool restoreLayoutOnStartup() const;

    ///
    /// \brief Stores whether the saved window layout should be restored at startup.
    /// \param enabled True to restore the layout on the next launch.
    ///
    void setRestoreLayoutOnStartup(bool enabled);

    ///
    /// \brief Returns the user-created subscriptions persisted from the last session.
    /// \return Custom subscriptions in stored order, or an empty vector when none are stored.
    ///
    QVector<SubscriptionItem> customSubscriptions() const;

    ///
    /// \brief Stores the user-created subscriptions to restore on the next launch.
    /// \param subscriptions Custom subscriptions to persist; built-in ones are ignored.
    ///
    void setCustomSubscriptions(const QVector<SubscriptionItem> &subscriptions);

    ///
    /// \brief Returns the saved element state for a named view.
    /// \param viewKey Stable identifier of the view (its object name).
    /// \return State blob, or an empty array when none is stored.
    ///
    QByteArray viewState(const QString &viewKey) const;

    ///
    /// \brief Stores the element state for a named view.
    /// \param viewKey Stable identifier of the view (its object name).
    /// \param state State blob produced by the view.
    ///
    void setViewState(const QString &viewKey, const QByteArray &state);

    ///
    /// \brief Removes the saved window geometry, layout, and per-view element state.
    ///
    /// Leaves user preferences (theme, restore-on-startup) untouched.
    ///
    void clearLayout();
};
