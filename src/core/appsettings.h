// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file appsettings.h
/// \brief Declares the central application settings store.
///

#pragma once

#include <QByteArray>
#include <QString>

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
