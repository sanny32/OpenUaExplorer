// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file toolbuttonlayout.h
/// \brief Declares helpers for responsive tool-button layouts.
///

#pragma once

#include <QList>

class QLayout;
class ThemedToolButton;

namespace ToolButtonLayout {

///
/// \brief Shows button text when the expanded layout fits, otherwise shows only icons.
/// \param layout Layout whose minimum width determines whether expanded buttons fit.
/// \param availableWidth Width currently available to the layout.
/// \param buttons Tool buttons that switch between expanded and compact styles.
///
void adaptToWidth(QLayout *layout, int availableWidth,
                  const QList<ThemedToolButton *> &buttons);

} // namespace ToolButtonLayout
