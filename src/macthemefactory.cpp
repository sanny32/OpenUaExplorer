// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#if defined(HAVE_QLEMENTINE_APP_STYLE)

#include "macthemefactory.h"

#include <QColor>
#include <QFontDatabase>
#include <QPalette>

namespace {

using oclero::qlementine::Theme;
// Light mode macOS colors
namespace Light {
    constexpr QRgb kCanvas         = 0xffffff;
    constexpr QRgb kChrome         = 0xf2f2f7;
    constexpr QRgb kChromeStrong   = 0xe5e5ea;
    constexpr QRgb kChromePressed  = 0xd1d1d6;
    constexpr QRgb kBorder         = 0xd1d1d6;
    constexpr QRgb kBorderActive   = 0xaeaeb2;
    constexpr QRgb kText           = 0x000000;
    constexpr QRgb kMutedText      = 0x8e8e93;
    constexpr QRgb kDisabledText   = 0xc7c7cc;
    constexpr QRgb kBlue           = 0x007aff;
    constexpr QRgb kBlueHover      = 0x1a8aff;
    constexpr QRgb kBluePressed    = 0x0062cc;
    constexpr QRgb kBlueDisabled   = 0xb3d7ff;
    constexpr QRgb kBlueDeepPress  = 0x0051a8;
    constexpr QRgb kGreen          = 0x34c759;
    constexpr QRgb kIconNormal     = 0x3c3c43;
    constexpr QRgb kIconActive     = 0x0062cc;
    constexpr QRgb kChromeDimmed   = 0xf4f6f8;
    constexpr QRgb kCanvasWarm     = 0xfefefe;
}

// Dark mode macOS colors
namespace Dark {
    constexpr QRgb kCanvas         = 0x1c1c1e;
    constexpr QRgb kChrome         = 0x2c2c2e;
    constexpr QRgb kChromeStrong   = 0x3a3a3c;
    constexpr QRgb kChromePressed  = 0x48484a;
    constexpr QRgb kBorder         = 0x38383a;
    constexpr QRgb kBorderActive   = 0x545456;
    constexpr QRgb kText           = 0xffffff;
    constexpr QRgb kMutedText      = 0x8e8e93;
    constexpr QRgb kDisabledText   = 0x48484a;
    constexpr QRgb kBlue           = 0x0a84ff;
    constexpr QRgb kBlueHover      = 0x409cff;
    constexpr QRgb kBluePressed    = 0x0071e3;
    constexpr QRgb kBlueDisabled   = 0x0a3d73;
    constexpr QRgb kGreen          = 0x30d158;
    constexpr QRgb kIconNormal     = 0xebebf5;
    constexpr QRgb kIconActive     = 0x409cff;
}

QColor transparent(QRgb rgb)
{
    QColor color(rgb);
    color.setAlpha(0);
    return color;
}

QColor alpha(QRgb rgb, int value)
{
    QColor color(rgb);
    color.setAlpha(value);
    return color;
}

} // namespace
///
/// \brief makeMacLightTheme
/// \return
///
Theme MacThemeFactory::makeLightTheme()
{
    using namespace Light;
    Theme theme;
    theme.meta.name = QStringLiteral("Open UaExplorer macOS Light");
    theme.meta.version = QStringLiteral("1.0");
    theme.meta.author = QStringLiteral("Open UaExplorer");

    theme.backgroundColorMain1 = QColor(kCanvas);
    theme.backgroundColorMain2 = QColor(kChrome);
    theme.backgroundColorMain3 = QColor(kChromeStrong);
    theme.backgroundColorMain4 = QColor(kChromePressed);
    theme.backgroundColorMainTransparent = transparent(kCanvas);
    theme.backgroundColorWorkspace = QColor(kCanvas);
    theme.backgroundColorTabBar = QColor(kChrome);

    theme.neutralColor = QColor(kChromeStrong);
    theme.neutralColorHovered = QColor(kChromePressed);
    theme.neutralColorPressed = QColor(0xc7c7cc);
    theme.neutralColorDisabled = QColor(0xf2f2f7);
    theme.neutralColorTransparent = transparent(kChromeStrong);

    theme.primaryColor = QColor(kBlue);
    theme.primaryColorHovered = QColor(kBlueHover);
    theme.primaryColorPressed = QColor(kBluePressed);
    theme.primaryColorDisabled = QColor(kBlueDisabled);
    theme.primaryColorTransparent = transparent(kBlue);

    theme.primaryColorForeground = QColor(kCanvas);
    theme.primaryColorForegroundHovered = QColor(kCanvas);
    theme.primaryColorForegroundPressed = QColor(kCanvas);
    theme.primaryColorForegroundDisabled = QColor(0xebf4fe);
    theme.primaryColorForegroundTransparent = transparent(kCanvas);

    theme.secondaryColor = QColor(kText);
    theme.secondaryColorHovered = QColor(0x1c1c1e);
    theme.secondaryColorPressed = QColor(0x2c2c2e);
    theme.secondaryColorDisabled = QColor(kDisabledText);
    theme.secondaryColorTransparent = transparent(kText);

    theme.secondaryColorForeground = QColor(0x3c3c43);
    theme.secondaryColorForegroundHovered = QColor(kBluePressed);
    theme.secondaryColorForegroundPressed = QColor(0x0051a8);
    theme.secondaryColorForegroundDisabled = QColor(kDisabledText);
    theme.secondaryColorForegroundTransparent = transparent(0x3c3c43);

    theme.secondaryAlternativeColor = QColor(kMutedText);
    theme.secondaryAlternativeColorHovered = QColor(kText);
    theme.secondaryAlternativeColorPressed = QColor(kText);
    theme.secondaryAlternativeColorDisabled = QColor(kDisabledText);
    theme.secondaryAlternativeColorTransparent = transparent(kMutedText);

    theme.statusColorSuccess = QColor(kGreen);
    theme.statusColorSuccessHovered = QColor(0x2fbd54);
    theme.statusColorSuccessPressed = QColor(0x29a94a);
    theme.statusColorSuccessDisabled = QColor(0xccefd6);
    theme.statusColorInfo = QColor(kBlue);
    theme.statusColorInfoHovered = QColor(kBlueHover);
    theme.statusColorInfoPressed = QColor(kBluePressed);
    theme.statusColorInfoDisabled = QColor(kBlueDisabled);

    theme.borderColor = QColor(kBorder);
    theme.borderColorHovered = QColor(kBorderActive);
    theme.borderColorPressed = QColor(0x9e9ea3);
    theme.borderColorDisabled = QColor(0xe5e5ea);
    theme.borderColorTransparent = transparent(kBorder);

    theme.semiTransparentColor1 = alpha(kText, 0);
    theme.semiTransparentColor2 = alpha(kText, 18);
    theme.semiTransparentColor3 = alpha(kText, 26);
    theme.semiTransparentColor4 = alpha(kText, 36);
    theme.semiTransparentColorTransparent = transparent(kText);

    theme.shadowColor1 = alpha(0x000000, 14);
    theme.shadowColor2 = alpha(0x000000, 20);
    theme.shadowColor3 = alpha(0x000000, 32);
    theme.shadowColorTransparent = alpha(0x000000, 0);

    theme.useSystemFonts = true;
    theme.fontSize = 13;
    theme.fontSizeMonospace = 13;
    theme.fontSizeS1 = 11;
    theme.fontRegular = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    theme.fontRegular.setPointSize(theme.fontSize);
    theme.fontBold = theme.fontRegular;
    theme.fontBold.setWeight(QFont::DemiBold);
    theme.fontCaption = theme.fontRegular;
    theme.fontCaption.setPointSize(theme.fontSizeS1);
    theme.fontMonospace = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    theme.fontMonospace.setPointSize(theme.fontSizeMonospace);
    theme.borderRadius = 6.0;
    theme.checkBoxBorderRadius = 4.0;
    theme.borderWidth = 1;
    theme.controlHeightLarge = 30;
    theme.controlHeightMedium = 26;
    theme.controlHeightSmall = 18;
    theme.controlDefaultWidth = 104;
    theme.iconSize = QSize(16, 16);
    theme.iconSizeMedium = QSize(22, 22);
    theme.iconSizeLarge = QSize(24, 24);
    theme.iconSizeExtraSmall = QSize(12, 12);
    theme.spacing = 8;
    theme.scrollBarThicknessFull = 12;
    theme.scrollBarThicknessSmall = 6;
    theme.tabBarPaddingTop = 2;
    theme.tabBarTabMinWidth = 0;
    theme.tabBarTabMaxWidth = 0;

    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Window, theme.backgroundColorMain2);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Base, theme.backgroundColorMain1);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::AlternateBase, QColor(kChromeStrong));
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Button, theme.neutralColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Text, theme.secondaryColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::WindowText, theme.secondaryColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::ButtonText, theme.secondaryColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::ToolTipBase, QColor(kText));
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::ToolTipText, QColor(kCanvas));
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Highlight, theme.primaryColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::HighlightedText, theme.primaryColorForeground);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::PlaceholderText, theme.secondaryAlternativeColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Mid, theme.borderColor);
    theme.palette.setColor(QPalette::ColorGroup::Disabled, QPalette::Text, theme.secondaryColorDisabled);
    theme.palette.setColor(QPalette::ColorGroup::Disabled, QPalette::WindowText, theme.secondaryColorDisabled);
    theme.palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ButtonText, theme.secondaryColorDisabled);
    theme.palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ToolTipText, theme.primaryColorForegroundDisabled);

    return theme;
}

///
/// \brief makeMacDarkTheme
/// \return
///
Theme MacThemeFactory::makeDarkTheme()
{
    using namespace Dark;
    Theme theme;
    theme.meta.name = QStringLiteral("Open UaExplorer macOS Dark");
    theme.meta.version = QStringLiteral("1.0");
    theme.meta.author = QStringLiteral("Open UaExplorer");

    theme.backgroundColorMain1 = QColor(kCanvas);
    theme.backgroundColorMain2 = QColor(kChrome);
    theme.backgroundColorMain3 = QColor(kChromeStrong);
    theme.backgroundColorMain4 = QColor(kChromePressed);
    theme.backgroundColorMainTransparent = transparent(kCanvas);
    theme.backgroundColorWorkspace = QColor(kCanvas);
    theme.backgroundColorTabBar = QColor(kChrome);

    theme.neutralColor = QColor(kChromeStrong);
    theme.neutralColorHovered = QColor(kChromePressed);
    theme.neutralColorPressed = QColor(0x545456);
    theme.neutralColorDisabled = QColor(0x2c2c2e);
    theme.neutralColorTransparent = transparent(kChromeStrong);

    theme.primaryColor = QColor(kBlue);
    theme.primaryColorHovered = QColor(kBlueHover);
    theme.primaryColorPressed = QColor(kBluePressed);
    theme.primaryColorDisabled = QColor(kBlueDisabled);
    theme.primaryColorTransparent = transparent(kBlue);

    theme.primaryColorForeground = QColor(kCanvas);
    theme.primaryColorForegroundHovered = QColor(kCanvas);
    theme.primaryColorForegroundPressed = QColor(kCanvas);
    theme.primaryColorForegroundDisabled = QColor(0x0a2a4a);
    theme.primaryColorForegroundTransparent = transparent(kCanvas);

    theme.secondaryColor = QColor(kText);
    theme.secondaryColorHovered = QColor(0xebebf5);
    theme.secondaryColorPressed = QColor(0xd1d1d6);
    theme.secondaryColorDisabled = QColor(kDisabledText);
    theme.secondaryColorTransparent = transparent(kText);

    theme.secondaryColorForeground = QColor(0xebebf5);
    theme.secondaryColorForegroundHovered = QColor(kBlueHover);
    theme.secondaryColorForegroundPressed = QColor(kBlue);
    theme.secondaryColorForegroundDisabled = QColor(kDisabledText);
    theme.secondaryColorForegroundTransparent = transparent(0xebebf5);

    theme.secondaryAlternativeColor = QColor(kMutedText);
    theme.secondaryAlternativeColorHovered = QColor(kText);
    theme.secondaryAlternativeColorPressed = QColor(kText);
    theme.secondaryAlternativeColorDisabled = QColor(kDisabledText);
    theme.secondaryAlternativeColorTransparent = transparent(kMutedText);

    theme.statusColorSuccess = QColor(kGreen);
    theme.statusColorSuccessHovered = QColor(0x26b84e);
    theme.statusColorSuccessPressed = QColor(0x1fa344);
    theme.statusColorSuccessDisabled = QColor(0x0f3d20);
    theme.statusColorInfo = QColor(kBlue);
    theme.statusColorInfoHovered = QColor(kBlueHover);
    theme.statusColorInfoPressed = QColor(kBluePressed);
    theme.statusColorInfoDisabled = QColor(kBlueDisabled);

    theme.borderColor = QColor(kBorder);
    theme.borderColorHovered = QColor(kBorderActive);
    theme.borderColorPressed = QColor(0x636366);
    theme.borderColorDisabled = QColor(0x2c2c2e);
    theme.borderColorTransparent = transparent(kBorder);

    theme.semiTransparentColor1 = alpha(kText, 0);
    theme.semiTransparentColor2 = alpha(kText, 20);
    theme.semiTransparentColor3 = alpha(kText, 30);
    theme.semiTransparentColor4 = alpha(kText, 45);
    theme.semiTransparentColorTransparent = transparent(kText);

    theme.shadowColor1 = alpha(0x000000, 40);
    theme.shadowColor2 = alpha(0x000000, 55);
    theme.shadowColor3 = alpha(0x000000, 70);
    theme.shadowColorTransparent = alpha(0x000000, 0);

    theme.useSystemFonts = true;
    theme.fontSize = 13;
    theme.fontSizeMonospace = 13;
    theme.fontSizeS1 = 11;
    theme.fontRegular = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    theme.fontRegular.setPointSize(theme.fontSize);
    theme.fontBold = theme.fontRegular;
    theme.fontBold.setWeight(QFont::DemiBold);
    theme.fontCaption = theme.fontRegular;
    theme.fontCaption.setPointSize(theme.fontSizeS1);
    theme.fontMonospace = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    theme.fontMonospace.setPointSize(theme.fontSizeMonospace);
    theme.borderRadius = 6.0;
    theme.checkBoxBorderRadius = 4.0;
    theme.borderWidth = 1;
    theme.controlHeightLarge = 30;
    theme.controlHeightMedium = 26;
    theme.controlHeightSmall = 18;
    theme.controlDefaultWidth = 104;
    theme.iconSize = QSize(16, 16);
    theme.iconSizeMedium = QSize(22, 22);
    theme.iconSizeLarge = QSize(24, 24);
    theme.iconSizeExtraSmall = QSize(12, 12);
    theme.spacing = 8;
    theme.scrollBarThicknessFull = 12;
    theme.scrollBarThicknessSmall = 6;
    theme.tabBarPaddingTop = 2;
    theme.tabBarTabMinWidth = 0;
    theme.tabBarTabMaxWidth = 0;

    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Window, theme.backgroundColorMain2);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Base, theme.backgroundColorMain1);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::AlternateBase, QColor(kChromeStrong));
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Button, theme.neutralColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Text, theme.secondaryColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::WindowText, theme.secondaryColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::ButtonText, theme.secondaryColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::ToolTipBase, QColor(kChromeStrong));
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::ToolTipText, QColor(kText));
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Highlight, theme.primaryColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::HighlightedText, theme.primaryColorForeground);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::PlaceholderText, theme.secondaryAlternativeColor);
    theme.palette.setColor(QPalette::ColorGroup::All, QPalette::Mid, theme.borderColor);
    theme.palette.setColor(QPalette::ColorGroup::Disabled, QPalette::Text, theme.secondaryColorDisabled);
    theme.palette.setColor(QPalette::ColorGroup::Disabled, QPalette::WindowText, theme.secondaryColorDisabled);
    theme.palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ButtonText, theme.secondaryColorDisabled);
    theme.palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ToolTipText, theme.primaryColorForegroundDisabled);

    return theme;
}


#endif
