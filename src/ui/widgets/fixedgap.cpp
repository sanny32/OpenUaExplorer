// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file fixedgap.cpp
/// \brief Implements a fixed-width spacer widget.
///

#include <QSizePolicy>
#include "fixedgap.h"

///
/// \brief Constructs a spacer of fixed width.
/// \param width Fixed width in pixels.
/// \param parent Parent widget.
///
FixedGap::FixedGap(int width, QWidget *parent)
    : QWidget(parent)
{
    setFixedWidth(width);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
}
