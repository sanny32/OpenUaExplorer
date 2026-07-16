// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file themecoordinator.cpp
/// \brief Implements the main-window theme action coordinator.
///

#include "themecoordinator.h"

#include <QAction>
#include <QActionGroup>

#include "appicons.h"
#include "application.h"

///
/// \brief Builds a coordinator for the toolbar and menu theme actions.
/// \param themeAction Toolbar action cycling through theme modes.
/// \param lightAction Menu action selecting the Light mode.
/// \param darkAction Menu action selecting the Dark mode.
/// \param systemAction Menu action selecting the System mode.
/// \param parent Parent object.
///
ThemeCoordinator::ThemeCoordinator(QAction *themeAction,
                                   QAction *lightAction,
                                   QAction *darkAction,
                                   QAction *systemAction,
                                   QObject *parent)
    : QObject(parent)
    , _themeAction(themeAction)
    , _lightAction(lightAction)
    , _darkAction(darkAction)
    , _systemAction(systemAction)
    , _group(new QActionGroup(this))
{
    _group->setExclusive(true);

    const bool manualThemeSupported = theApp()->theme().isManualToggleSupported();
    const QList<QAction *> modeActions = {_lightAction, _darkAction, _systemAction};
    for (QAction *action : modeActions) {
        action->setCheckable(true);
        action->setEnabled(manualThemeSupported);
        _group->addAction(action);
    }

    connect(_lightAction, &QAction::triggered, this,
            [this] { apply(AppSettings::ThemeMode::Light); });
    connect(_darkAction, &QAction::triggered, this,
            [this] { apply(AppSettings::ThemeMode::Dark); });
    connect(_systemAction, &QAction::triggered, this,
            [this] { apply(AppSettings::ThemeMode::System); });
    connect(&theApp()->theme(), &AppTheme::colorSchemeChanged, this,
            &ThemeCoordinator::updateActions);

    updateActions();
}

///
/// \brief Advances the colour scheme to the next mode.
///
void ThemeCoordinator::cycle()
{
    AppSettings::ThemeMode next = AppSettings::ThemeMode::Light;
    switch (AppSettings().themeMode()) {
    case AppSettings::ThemeMode::Light:
        next = AppSettings::ThemeMode::Dark;
        break;
    case AppSettings::ThemeMode::Dark:
        next = AppSettings::ThemeMode::System;
        break;
    case AppSettings::ThemeMode::System:
        next = AppSettings::ThemeMode::Light;
        break;
    }
    apply(next);
}

///
/// \brief Applies and persists a colour-scheme mode, then refreshes the actions.
/// \param mode Light, Dark or System mode to activate.
///
void ThemeCoordinator::apply(AppSettings::ThemeMode mode)
{
    theApp()->theme().setColorSchemePreference(mode);
    updateActions();
}

///
/// \brief Reflects the saved colour-scheme mode in the toolbar icon and Theme submenu.
///
void ThemeCoordinator::updateActions()
{
    switch (AppSettings().themeMode()) {
    case AppSettings::ThemeMode::Light:
        _themeAction->setIcon(AppIcons::themed(QStringLiteral("theme-light")));
        _themeAction->setToolTip(tr("Theme: Light - click to switch to Dark"));
        _lightAction->setChecked(true);
        break;
    case AppSettings::ThemeMode::Dark:
        _themeAction->setIcon(AppIcons::themed(QStringLiteral("theme-dark")));
        _themeAction->setToolTip(tr("Theme: Dark - click to switch to System"));
        _darkAction->setChecked(true);
        break;
    case AppSettings::ThemeMode::System:
        _themeAction->setIcon(AppIcons::themed(QStringLiteral("theme-system")));
        _themeAction->setToolTip(tr("Theme: System - click to switch to Light"));
        _systemAction->setChecked(true);
        break;
    }
}
