// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file maintoolbutton.h
/// \brief Declares the main toolbar action button.
///

#pragma once

#include "themedtoolbutton.h"

class QAction;

///
/// \brief Themed toolbar button for main actions, width-equalised by MainToolBar.
///
class MainToolButton : public ThemedToolButton
{
    Q_OBJECT

public:
    /// \brief Lower bound for the shared width of main toolbar buttons, in pixels.
    ///
    /// The toolbar equalises every button to the widest button's content width,
    /// never going below this floor so short labels still look balanced.
    static constexpr int minWidth = 78;

    ///
    /// \brief Builds a text-under-icon button bound to an action.
    /// \param action Default action driving the button; its text becomes the tooltip.
    /// \param parent Parent widget.
    ///
    explicit MainToolButton(QAction *action, QWidget *parent = nullptr);
};
