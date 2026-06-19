// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file appcolors.h
/// \brief Provides the application's named, theme-aware colour palette.
///

#pragma once

#include <QColor>
#include <QString>

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

///
/// \brief Strong colour for prominent titles and headings.
/// \return Theme-matching title colour.
///
inline QColor titleText()
{
    return AppIcons::isDarkTheme() ? QColor(0xffffff) : QColor(0x1a1a1a);
}

///
/// \brief Muted colour for secondary descriptions and subtitles.
/// \return Theme-matching secondary text colour.
///
inline QColor subtitleText()
{
    return AppIcons::isDarkTheme() ? QColor(0xb8b8b8) : QColor(0x6b7280);
}

///
/// \brief Background fill for a warning notice banner.
/// \return Theme-matching warning banner background.
///
inline QColor noticeWarningBackground()
{
    return AppIcons::isDarkTheme() ? QColor(0x24211a) : QColor(0xfdf4e7);
}

///
/// \brief Border colour for a warning notice banner.
/// \return Theme-matching warning banner border.
///
inline QColor noticeWarningBorder()
{
    return AppIcons::isDarkTheme() ? QColor(0x4a3920) : QColor(0xf3e2c2);
}

///
/// \brief Translucent tint behind a warning notice icon.
/// \return Amber tint with low alpha.
///
inline QColor noticeWarningIconTint()
{
    return QColor(0xf5, 0xb8, 0x2e, 31);
}

///
/// \brief Background fill for a neutral notice banner.
/// \return Theme-matching neutral banner background.
///
inline QColor noticeNeutralBackground()
{
    return AppIcons::isDarkTheme() ? QColor(0x222a33) : QColor(0xf1f5f9);
}

///
/// \brief Border colour for a neutral notice banner.
/// \return Theme-matching neutral banner border.
///
inline QColor noticeNeutralBorder()
{
    return AppIcons::isDarkTheme() ? QColor(0x36404c) : QColor(0xe2e8f0);
}

///
/// \brief Translucent tint behind a neutral notice icon.
/// \return Green tint with low alpha.
///
inline QColor noticeNeutralIconTint()
{
    return QColor(0x2e, 0x9e, 0x44, 31);
}

///
/// \brief Background fill for an inline error badge.
/// \return Theme-matching error badge background.
///
inline QColor errorBadgeBackground()
{
    return AppIcons::isDarkTheme() ? QColor(0x3a1f23) : QColor(0xfce9e9);
}

///
/// \brief Border colour for an inline error badge.
/// \return Theme-matching error badge border.
///
inline QColor errorBadgeBorder()
{
    return AppIcons::isDarkTheme() ? QColor(0x7a3038) : QColor(0xf4cdce);
}

///
/// \brief Text colour for an inline error badge.
/// \return Theme-matching error badge text colour.
///
inline QColor errorBadgeText()
{
    return AppIcons::isDarkTheme() ? QColor(0xff6b6b) : statusError();
}

///
/// \brief Formats a colour as a Qt Style Sheet rgba() string, preserving alpha.
/// \param color Colour to format.
/// \return CSS rgba() literal.
///
inline QString toCss(const QColor &color)
{
    return QStringLiteral("rgba(%1, %2, %3, %4)")
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue())
        .arg(QString::number(color.alphaF(), 'f', 3));
}

}
