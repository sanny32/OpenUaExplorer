// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file themedpushbutton.cpp
/// \brief Implements a theme-aware push button.
///

#include <QEvent>

#include "appicons.h"
#include "themedpushbutton.h"

///
/// \brief Constructs the themed push button.
/// \param parent Parent widget.
///
ThemedPushButton::ThemedPushButton(QWidget *parent)
    : QPushButton(parent)
{
}

///
/// \brief Sets the themed icon by resource name.
/// \param name Icon resource name.
///
void ThemedPushButton::setIcon(const QString &name)
{
    _iconName = name;
    refreshIcon();
}

///
/// \brief Re-renders the icon when the palette changes.
/// \param event Change event being handled.
///
void ThemedPushButton::changeEvent(QEvent *event)
{
    QPushButton::changeEvent(event);

    if (!_iconName.isEmpty()
        && (event->type() == QEvent::PaletteChange
            || event->type() == QEvent::ApplicationPaletteChange)) {
        refreshIcon();
    }
}

///
/// \brief Reloads the current icon for the active theme.
///
void ThemedPushButton::refreshIcon()
{
    QPushButton::setIcon(AppIcons::themed(_iconName));
}
