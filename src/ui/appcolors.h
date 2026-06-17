// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file appcolors.h
/// \brief Provides the application's named, theme-aware colour palette.
///

#pragma once

#include <QColor>

#include "appicons.h"

///
/// \brief Central source of truth for the application's semantic colours.
///
/// Mirrors the AppIcons helper: a thin, theme-aware wrapper so widgets reference
/// named roles instead of hard-coding hex literals. Light/dark variants follow the
/// active colour scheme reported by the theme.
///
namespace AppColors {

///
/// \brief Accent colour for primary/affirmative push buttons.
/// \return Light-theme base colour (ColoredPushButton derives the dark variant).
///
inline QColor accent()
{
    return QColor(0x0a74d1);
}

///
/// \brief Hover colour for primary/affirmative push buttons.
/// \return Light-theme hover colour.
///
inline QColor accentHover()
{
    return QColor(0x1682df);
}

///
/// \brief Pressed colour for primary/affirmative push buttons.
/// \return Light-theme pressed colour.
///
inline QColor accentPressed()
{
    return QColor(0x075ca7);
}

///
/// \brief Steel-blue used for field labels in detail grids.
/// \return Theme-matching field-label colour.
///
inline QColor fieldLabel()
{
    return AppIcons::isDarkTheme() ? QColor(0x8aaace) : QColor(0x4a6f96);
}

///
/// \brief Muted grey used for small captions.
/// \return Caption text colour.
///
inline QColor caption()
{
    return QColor(0x6b7280);
}

///
/// \brief Emphasised colour for section headers and link-like text.
/// \return Theme-matching header/link colour.
///
inline QColor header()
{
    return AppIcons::isDarkTheme() ? QColor(0x60a5fa) : QColor(0x2563eb);
}

///
/// \brief Muted colour for hint and disabled text.
/// \return Theme-matching hint colour.
///
inline QColor hint()
{
    return AppIcons::isDarkTheme() ? QColor(0x7c828b) : QColor(0x9aa0a6);
}

///
/// \brief Status colour signalling success or a valid state.
/// \return Success colour.
///
inline QColor statusSuccess()
{
    return QColor(0x2e9e44);
}

///
/// \brief Status colour signalling a warning or pending state.
/// \return Warning colour.
///
inline QColor statusWarning()
{
    return QColor(0xc07d00);
}

///
/// \brief Status colour signalling an error or invalid state.
/// \return Error colour.
///
inline QColor statusError()
{
    return QColor(0xd13438);
}

}
