// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file macwindowbuttons.h
/// \brief Declares native control over the macOS title-bar buttons.
///

#pragma once

class QWidget;

namespace MacWindowButtons {

///
/// \brief Hides the minimize and zoom title-bar buttons, keeping only close.
/// \param window Widget whose native window is adjusted.
///
void hideMinimizeAndZoomButtons(QWidget *window);

} // namespace MacWindowButtons
