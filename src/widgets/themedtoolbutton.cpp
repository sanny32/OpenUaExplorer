// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file themedtoolbutton.cpp
/// \brief Implements a theme-aware tool button.
///

#include <QEvent>

#include "appicons.h"
#include "themedtoolbutton.h"

///
/// \brief ThemedToolButton::ThemedToolButton
/// \param parent
///
ThemedToolButton::ThemedToolButton(QWidget *parent)
    : QToolButton(parent)
{
}

///
/// \brief ThemedToolButton::setIcon
/// \param name
///
void ThemedToolButton::setIcon(const QString &name)
{
    _iconName = name;
    refreshIcon();
}

///
/// \brief ThemedToolButton::changeEvent
/// \param event
///
void ThemedToolButton::changeEvent(QEvent *event)
{
    QToolButton::changeEvent(event);

    if (!_iconName.isEmpty()
        && (event->type() == QEvent::PaletteChange
            || event->type() == QEvent::ApplicationPaletteChange)) {
        refreshIcon();
    }
}

///
/// \brief ThemedToolButton::refreshIcon
///
void ThemedToolButton::refreshIcon()
{
    QToolButton::setIcon(AppIcons::themed(_iconName));
}
