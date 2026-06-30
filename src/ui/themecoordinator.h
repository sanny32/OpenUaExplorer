// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file themecoordinator.h
/// \brief Declares the main-window theme action coordinator.
///

#pragma once

#include <QObject>

#include "appsettings.h"

class QAction;
class QActionGroup;

///
/// \brief Coordinates the main-window theme actions with the application theme state.
///
class ThemeCoordinator : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief Builds a coordinator for the toolbar and menu theme actions.
    /// \param themeAction Toolbar action cycling through theme modes.
    /// \param lightAction Menu action selecting the Light mode.
    /// \param darkAction Menu action selecting the Dark mode.
    /// \param systemAction Menu action selecting the System mode.
    /// \param parent Parent object.
    ///
    ThemeCoordinator(QAction *themeAction,
                     QAction *lightAction,
                     QAction *darkAction,
                     QAction *systemAction,
                     QObject *parent = nullptr);

    ///
    /// \brief Advances the colour scheme to the next mode.
    ///
    void cycle();

private:
    void apply(AppSettings::ThemeMode mode);
    void updateActions();

    QAction *_themeAction;
    QAction *_lightAction;
    QAction *_darkAction;
    QAction *_systemAction;
    QActionGroup *_group;
};
