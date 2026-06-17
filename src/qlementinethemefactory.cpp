// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#if defined(HAVE_QLEMENTINE_APP_STYLE)

#include "qlementinethemefactory.h"

#include <QColor>
#include <QPalette>

namespace {

using oclero::qlementine::Theme;
namespace Light {
constexpr QRgb kCanvas = 0xffffff;
constexpr QRgb kChrome = 0xf5f5f5;
constexpr QRgb kChromeStrong = 0xececec;
constexpr QRgb kChromePressed = 0xe0e0e0;
constexpr QRgb kChromeMuted = 0xfefefe;
constexpr QRgb kChromeDisabled = 0xf4f6f8;
constexpr QRgb kSurfaceDisabled = 0xf2f2f2;
constexpr QRgb kBorder = 0xe3e6ea;
constexpr QRgb kBorderActive = 0xcdd1d6;
constexpr QRgb kBorderMuted = 0xe6e9ee;
constexpr QRgb kBorderDisabled = 0xebeef2;
constexpr QRgb kText = 0x1f2937;
constexpr QRgb kTextMuted = 0x4f627a;
constexpr QRgb kTextEmphasis = 0x24364d;
constexpr QRgb kDisabledText = 0xb2bac5;
constexpr QRgb kBlue = 0x007aff;
constexpr QRgb kBlueHover = 0x1a8aff;
constexpr QRgb kBluePressed = 0x0062cc;
constexpr QRgb kBlueDisabled = 0xb3d7ff;
constexpr QRgb kSelection = 0xd9eaff;
constexpr QRgb kSelectionDisabled = 0xeaeaea;
constexpr QRgb kItemHover = 0xf0f0f0;
constexpr QRgb kItemPressed = 0xe5e5e5;
constexpr QRgb kTooltipBase = 0x1f2937;
constexpr QRgb kTooltipText = 0xffffff;
}

namespace Dark {
constexpr QRgb kCanvas = 0x1e1f24;
constexpr QRgb kChrome = 0x272a30;
constexpr QRgb kChromeStrong = 0x333841;
constexpr QRgb kChromePressed = 0x414854;
constexpr QRgb kChromeMuted = 0x2d3138;
constexpr QRgb kChromeDisabled = 0x24272d;
constexpr QRgb kSurfaceDisabled = 0x21242a;
constexpr QRgb kBorder = 0x3b414c;
constexpr QRgb kBorderActive = 0x5b6575;
constexpr QRgb kBorderMuted = 0x313740;
constexpr QRgb kBorderDisabled = 0x2a2e36;
constexpr QRgb kText = 0xe7ecf3;
constexpr QRgb kTextMuted = 0xa0abbb;
constexpr QRgb kTextEmphasis = 0xffffff;
constexpr QRgb kDisabledText = 0x6c7582;
constexpr QRgb kBlue = 0x409cff;
constexpr QRgb kBlueHover = 0x66b2ff;
constexpr QRgb kBluePressed = 0x1f8bff;
constexpr QRgb kBlueDisabled = 0x27476a;
constexpr QRgb kSelection = 0x1d3c5c;
constexpr QRgb kSelectionDisabled = 0x2b3139;
constexpr QRgb kItemHover = 0x2e333b;
constexpr QRgb kItemPressed = 0x373d46;
constexpr QRgb kTooltipBase = 0x111318;
constexpr QRgb kTooltipText = 0xe7ecf3;
}

///
/// \brief Builds a fully transparent colour from an RGB value.
/// \param rgb Source RGB value.
/// \return Colour with alpha set to zero.
///
QColor transparent(QRgb rgb)
{
    QColor color(rgb);
    color.setAlpha(0);
    return color;
}

///
/// \brief Builds a colour from an RGB value with an explicit alpha.
/// \param rgb Source RGB value.
/// \param value Alpha channel, 0–255.
/// \return Colour with the given opacity.
///
QColor alpha(QRgb rgb, int value)
{
    QColor color(rgb);
    color.setAlpha(value);
    return color;
}

} // namespace
///
/// \brief Derives a light or dark Qlementine theme from a base theme using the app palette.
/// \param baseTheme Theme to copy structural defaults from.
/// \param darkMode True to produce the dark variant.
/// \return The customised theme.
///
Theme QlementineThemeFactory::make(const Theme& baseTheme, bool darkMode)
{
    const auto rgb = [darkMode](QRgb light, QRgb dark) { return darkMode ? dark : light; };
    const auto color = [&rgb](QRgb light, QRgb dark) { return QColor(rgb(light, dark)); };

    Theme theme = baseTheme;
    theme.meta.name = darkMode
        ? QStringLiteral("Open UaExplorer Qlementine Dark")
        : QStringLiteral("Open UaExplorer Qlementine Light");
    theme.meta.version = QStringLiteral("1.0");
    theme.meta.author = QStringLiteral("Open UaExplorer");

    theme.backgroundColorMain1 = color(Light::kCanvas, Dark::kCanvas);
    theme.backgroundColorMain2 = color(Light::kChrome, Dark::kChrome);
    theme.backgroundColorMain3 = color(Light::kChromeStrong, Dark::kChromeStrong);
    theme.backgroundColorMain4 = color(Light::kChromePressed, Dark::kChromePressed);
    theme.backgroundColorMainTransparent = transparent(rgb(Light::kCanvas, Dark::kCanvas));
    theme.backgroundColorWorkspace = color(Light::kCanvas, Dark::kCanvas);
    theme.backgroundColorTabBar = color(Light::kChrome, Dark::kChrome);

    theme.neutralColor = color(Light::kChromeStrong, Dark::kChromeStrong);
    theme.neutralColorHovered = color(Light::kChromePressed, Dark::kChromePressed);
    theme.neutralColorPressed = color(Light::kBorderActive, Dark::kBorderActive);
    theme.neutralColorDisabled = color(Light::kChromeDisabled, Dark::kChromeDisabled);
    theme.neutralColorTransparent = transparent(rgb(Light::kChromeStrong, Dark::kChromeStrong));

    theme.primaryColor = color(Light::kBlue, Dark::kBlue);
    theme.primaryColorHovered = color(Light::kBlueHover, Dark::kBlueHover);
    theme.primaryColorPressed = color(Light::kBluePressed, Dark::kBluePressed);
    theme.primaryColorDisabled = color(Light::kBlueDisabled, Dark::kBlueDisabled);
    theme.primaryColorTransparent = transparent(rgb(Light::kBlue, Dark::kBlue));

    theme.primaryColorForeground = color(Light::kTooltipText, Dark::kTextEmphasis);
    theme.primaryColorForegroundHovered = color(Light::kTooltipText, Dark::kTextEmphasis);
    theme.primaryColorForegroundPressed = color(Light::kTooltipText, Dark::kTextEmphasis);
    theme.primaryColorForegroundDisabled = color(Light::kChromeMuted, Dark::kDisabledText);
    theme.primaryColorForegroundTransparent = transparent(rgb(Light::kTooltipText, Dark::kTextEmphasis));

    theme.secondaryColor = color(Light::kText, Dark::kText);
    theme.secondaryColorHovered = color(Light::kTextEmphasis, Dark::kTextEmphasis);
    theme.secondaryColorPressed = color(Light::kTextEmphasis, Dark::kTextEmphasis);
    theme.secondaryColorDisabled = color(Light::kDisabledText, Dark::kDisabledText);
    theme.secondaryColorTransparent = transparent(rgb(Light::kText, Dark::kText));

    theme.secondaryColorForeground = color(Light::kTextMuted, Dark::kTextMuted);
    theme.secondaryColorForegroundHovered = color(Light::kBluePressed, Dark::kBlueHover);
    theme.secondaryColorForegroundPressed = color(Light::kBluePressed, Dark::kBluePressed);
    theme.secondaryColorForegroundDisabled = color(Light::kDisabledText, Dark::kDisabledText);
    theme.secondaryColorForegroundTransparent = transparent(rgb(Light::kTextMuted, Dark::kTextMuted));

    theme.secondaryAlternativeColor = color(Light::kTextMuted, Dark::kTextMuted);
    theme.secondaryAlternativeColorHovered = color(Light::kText, Dark::kText);
    theme.secondaryAlternativeColorPressed = color(Light::kText, Dark::kTextEmphasis);
    theme.secondaryAlternativeColorDisabled = color(Light::kDisabledText, Dark::kDisabledText);
    theme.secondaryAlternativeColorTransparent = transparent(rgb(Light::kTextMuted, Dark::kTextMuted));

    theme.borderColor = color(Light::kBorder, Dark::kBorder);
    theme.borderColorHovered = color(Light::kBorderActive, Dark::kBorderActive);
    theme.borderColorPressed = color(Light::kBorderActive, Dark::kBorderActive);
    theme.borderColorDisabled = color(Light::kBorderDisabled, Dark::kBorderDisabled);
    theme.borderColorTransparent = transparent(rgb(Light::kBorder, Dark::kBorder));

    theme.semiTransparentColor1 = alpha(rgb(Light::kText, Dark::kText), 0);
    theme.semiTransparentColor2 = alpha(rgb(Light::kText, Dark::kText), darkMode ? 28 : 18);
    theme.semiTransparentColor3 = alpha(rgb(Light::kText, Dark::kText), darkMode ? 42 : 26);
    theme.semiTransparentColor4 = alpha(rgb(Light::kText, Dark::kText), darkMode ? 58 : 36);
    theme.semiTransparentColorTransparent = transparent(rgb(Light::kText, Dark::kText));

    theme.shadowColor1 = alpha(0x000000, darkMode ? 32 : 14);
    theme.shadowColor2 = alpha(0x000000, darkMode ? 44 : 20);
    theme.shadowColor3 = alpha(0x000000, darkMode ? 64 : 32);
    theme.shadowColorTransparent = alpha(0x000000, 0);

    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Window, theme.backgroundColorMain2);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Base, theme.backgroundColorMain1);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::AlternateBase, theme.backgroundColorMain3);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Button, theme.neutralColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Text, theme.secondaryColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::WindowText, theme.secondaryColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::ButtonText, theme.secondaryColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::ToolTipBase, color(Light::kTooltipBase, Dark::kTooltipBase));
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::ToolTipText, color(Light::kTooltipText, Dark::kTooltipText));
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Highlight, theme.primaryColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::HighlightedText, theme.primaryColorForeground);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::PlaceholderText, theme.secondaryColorForeground);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Mid, theme.borderColor);
    theme.palette.setColor(QPalette::ColorGroup::Disabled, QPalette::Base, color(Light::kSurfaceDisabled, Dark::kSurfaceDisabled));
    theme.palette.setColor(QPalette::ColorGroup::Disabled, QPalette::Button, color(Light::kChromeDisabled, Dark::kChromeDisabled));
    theme.palette.setColor(QPalette::ColorGroup::Disabled, QPalette::Text, theme.secondaryColorDisabled);
    theme.palette.setColor(QPalette::ColorGroup::Disabled, QPalette::WindowText, theme.secondaryColorDisabled);
    theme.palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ButtonText, theme.secondaryColorDisabled);
    theme.palette.setColor(QPalette::ColorGroup::Disabled, QPalette::Highlight, theme.primaryColorDisabled);

    return theme;
}


#endif
