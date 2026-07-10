// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file maintoolbutton.cpp
/// \brief Implements the main toolbar action button.
///

#include <QAction>

#include "maintoolbutton.h"

///
/// \brief Builds a text-under-icon button bound to an action.
/// \param action Default action driving the button; supplies its text, icon and tooltip.
/// \param parent Parent widget.
///
MainToolButton::MainToolButton(QAction *action, QWidget *parent)
    : ThemedToolButton(parent)
{
    // setDefaultAction already mirrors the action's tooltip onto the button, and
    // QAction::toolTip() falls back to the action text when none is set, so buttons
    // without an explicit tooltip still show their label.
    setDefaultAction(action);
    setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    // Floor only; MainToolBar equalises the final width across all buttons.
    setMinimumWidth(minWidth);
}
