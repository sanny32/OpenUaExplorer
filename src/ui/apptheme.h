// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file apptheme.h
/// \brief Declares the application theme manager.
///

#pragma once

#include <QObject>
#include <QtGlobal>

#include "appsettings.h"

#ifdef HAS_QTDBUS
#include <QDBusVariant>
#endif

class AppTheme : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief Available application color-scheme states.
    ///
    enum class ColorScheme {
        Unknown,
        Light,
        Dark
    };

    ///
    /// \brief Constructs the theme manager and wires up system color-scheme detection.
    /// \param parent Owning QObject.
    ///
    explicit AppTheme(QObject *parent = nullptr);

    ///
    /// \brief Reports whether the dark scheme is active.
    /// \return True if the current color scheme is dark.
    ///
    bool isDark() const;

    ///
    /// \brief Reports whether the user may switch schemes manually.
    /// \return True if the user can manually toggle the color scheme.
    ///
    bool isManualToggleSupported() const;

    ///
    /// \brief Switches between light and dark, marking the scheme as manually overridden.
    ///
    void toggle();

    ///
    /// \brief Applies and persists a color-scheme preference chosen by the user.
    /// \param mode Light or Dark to override, or System to follow the platform.
    ///
    void setColorSchemePreference(AppSettings::ThemeMode mode);

    ///
    /// \brief Applies the startup color scheme from the saved preference, portal, Qt API, or light fallback.
    ///
    void applyInitialScheme();

signals:
    ///
    /// \brief Emitted when the active color scheme changes.
    ///
    void colorSchemeChanged();

private:
    void setupSystemColorScheme();
    bool setupPortalColorScheme();
    bool applyPortalColorScheme();
    void readStyleHintsColorScheme();
    void applyColorScheme(ColorScheme scheme);
    void applyNativeColorScheme(ColorScheme scheme);

#ifdef HAS_QTDBUS
    Q_SLOT void onPortalSettingChanged(const QString &group, const QString &key,
                                       const QDBusVariant &value);
#endif

private:
    bool _manualToggleSupported = false;
    bool _manualSchemeOverriden = false;
    ColorScheme _scheme = ColorScheme::Unknown;
};
