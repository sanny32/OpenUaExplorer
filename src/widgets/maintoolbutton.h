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
/// \brief Fixed-width themed toolbar button for main actions.
///
class MainToolButton : public ThemedToolButton
{
    Q_OBJECT

public:
    explicit MainToolButton(QAction *action, QWidget *parent = nullptr);
};
