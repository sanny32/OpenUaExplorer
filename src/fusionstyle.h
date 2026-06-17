// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file fusionstyle.h
/// \brief Declares Fusion style palette helpers.
///

#pragma once

#include <QPalette>

///
/// \brief Builds light and dark Fusion palettes for the application.
///
class FusionStyle
{
public:
    ///
    /// \brief Builds a Fusion-style palette tuned for a light or dark appearance.
    /// \param darkAppearance True to create a dark appearance palette.
    /// \return Palette configured for the Fusion style.
    ///
    static QPalette palette(bool darkAppearance);
};
