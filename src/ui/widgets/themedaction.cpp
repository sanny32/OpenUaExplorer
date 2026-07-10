// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file themedaction.cpp
/// \brief Implements a theme-aware action.
///

#include <QApplication>
#include <QEvent>

#include "appicons.h"
#include "themedaction.h"

///
/// \brief Constructs the themed action.
/// \param parent Parent object.
///
ThemedAction::ThemedAction(QObject *parent)
    : QAction(parent)
{
    qApp->installEventFilter(this);
}

///
/// \brief Constructs the themed action with an icon and text.
/// \param iconName Icon resource name.
/// \param text Action text.
/// \param parent Parent object.
///
ThemedAction::ThemedAction(const QString &iconName, const QString &text, QObject *parent)
    : QAction(text, parent)
{
    qApp->installEventFilter(this);
    setIconName(iconName);
}

///
/// \brief Returns the themed icon resource name.
/// \return Icon resource name.
///
QString ThemedAction::iconName() const
{
    return _iconName;
}

///
/// \brief Sets the themed icon resource name.
/// \param name Icon resource name.
///
void ThemedAction::setIconName(const QString &name)
{
    if (_iconName == name) {
        return;
    }

    _iconName = name;
    if (_iconName.isEmpty()) {
        setIcon({});
        return;
    }
    refreshIcon();
}

///
/// \brief Reloads the icon when the application palette changes.
/// \param watched Object delivering the event.
/// \param event Event being filtered.
/// \return Result of the base class event filter.
///
bool ThemedAction::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == qApp && !_iconName.isEmpty()
        && (event->type() == QEvent::ApplicationPaletteChange
            || event->type() == QEvent::PaletteChange)) {
        refreshIcon();
    }
    return QAction::eventFilter(watched, event);
}

///
/// \brief Reloads the current icon for the active theme.
///
void ThemedAction::refreshIcon()
{
    setIcon(AppIcons::themed(_iconName));
}
