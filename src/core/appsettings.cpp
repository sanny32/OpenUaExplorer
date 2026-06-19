// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file appsettings.cpp
/// \brief Implements the central application settings store.
///

#include <QSettings>

#include "appsettings.h"

namespace {
constexpr auto themeModeKey = "appearance/themeMode";
constexpr auto windowGeometryKey = "mainWindow/geometry";
constexpr auto windowStateKey = "mainWindow/state";
constexpr auto centralSplitterKey = "mainWindow/centralSplitter";
constexpr auto dataAccessPageKey = "mainWindow/dataAccessPage";
constexpr auto restoreLayoutKey = "mainWindow/restoreLayout";
constexpr auto viewStateGroup = "viewState";
}

///
/// \brief Returns the stored color-scheme preference.
/// \return Saved theme mode, or ThemeMode::System when none is stored.
///
AppSettings::ThemeMode AppSettings::themeMode() const
{
    QSettings settings;
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
    QSettings settings;
    settings.setValue(QLatin1String(themeModeKey), static_cast<int>(mode));
}

///
/// \brief Returns the saved top-level window geometry.
/// \return Window geometry blob, or an empty array when none is stored.
///
QByteArray AppSettings::windowGeometry() const
{
    QSettings settings;
    return settings.value(QLatin1String(windowGeometryKey)).toByteArray();
}

///
/// \brief Stores the top-level window geometry.
/// \param geometry Geometry blob from QWidget::saveGeometry().
///
void AppSettings::setWindowGeometry(const QByteArray &geometry)
{
    QSettings settings;
    settings.setValue(QLatin1String(windowGeometryKey), geometry);
}

///
/// \brief Returns the saved dock and toolbar layout state.
/// \return Window state blob, or an empty array when none is stored.
///
QByteArray AppSettings::windowState() const
{
    QSettings settings;
    return settings.value(QLatin1String(windowStateKey)).toByteArray();
}

///
/// \brief Stores the dock and toolbar layout state.
/// \param state State blob from QMainWindow::saveState().
///
void AppSettings::setWindowState(const QByteArray &state)
{
    QSettings settings;
    settings.setValue(QLatin1String(windowStateKey), state);
}

///
/// \brief Returns the saved central splitter state.
/// \return Splitter state blob, or an empty array when none is stored.
///
QByteArray AppSettings::centralSplitterState() const
{
    QSettings settings;
    return settings.value(QLatin1String(centralSplitterKey)).toByteArray();
}

///
/// \brief Stores the central splitter state.
/// \param state State blob from QSplitter::saveState().
///
void AppSettings::setCentralSplitterState(const QByteArray &state)
{
    QSettings settings;
    settings.setValue(QLatin1String(centralSplitterKey), state);
}

///
/// \brief Returns the saved data-access tab page index.
/// \return Stored page index, or 0 when none is stored.
///
int AppSettings::dataAccessPage() const
{
    QSettings settings;
    return settings.value(QLatin1String(dataAccessPageKey), 0).toInt();
}

///
/// \brief Stores the data-access tab page index.
/// \param page Page index to persist.
///
void AppSettings::setDataAccessPage(int page)
{
    QSettings settings;
    settings.setValue(QLatin1String(dataAccessPageKey), page);
}

///
/// \brief Reports whether the saved window layout should be restored at startup.
/// \return True when the layout should be restored, defaulting to true.
///
bool AppSettings::restoreLayoutOnStartup() const
{
    QSettings settings;
    return settings.value(QLatin1String(restoreLayoutKey), true).toBool();
}

///
/// \brief Stores whether the saved window layout should be restored at startup.
/// \param enabled True to restore the layout on the next launch.
///
void AppSettings::setRestoreLayoutOnStartup(bool enabled)
{
    QSettings settings;
    settings.setValue(QLatin1String(restoreLayoutKey), enabled);
}

///
/// \brief Returns the saved element state for a named view.
/// \param viewKey Stable identifier of the view (its object name).
/// \return State blob, or an empty array when none is stored.
///
QByteArray AppSettings::viewState(const QString &viewKey) const
{
    QSettings settings;
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
    QSettings settings;
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
    QSettings settings;
    settings.remove(QLatin1String(windowGeometryKey));
    settings.remove(QLatin1String(windowStateKey));
    settings.remove(QLatin1String(centralSplitterKey));
    settings.remove(QLatin1String(dataAccessPageKey));
    settings.remove(QLatin1String(viewStateGroup));
}
