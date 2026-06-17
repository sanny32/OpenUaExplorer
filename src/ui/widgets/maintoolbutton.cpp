// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file maintoolbutton.cpp
/// \brief Implements the main toolbar action button.
///

#include <QAction>

#include "maintoolbutton.h"

namespace {
constexpr int mainToolButtonWidth = 78;
}

///
/// \brief Builds a fixed-width text-under-icon button bound to an action.
/// \param action Default action driving the button; its text becomes the tooltip.
/// \param parent Parent widget.
///
MainToolButton::MainToolButton(QAction *action, QWidget *parent)
    : ThemedToolButton(parent)
{
    setDefaultAction(action);
    setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    setMinimumWidth(mainToolButtonWidth);
    setMaximumWidth(mainToolButtonWidth);

    if (action) {
        setToolTip(action->text());
    }
}
