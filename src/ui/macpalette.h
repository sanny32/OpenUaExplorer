// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file macpalette.h
/// \brief Raw macOS palette shared by the macOS theme factory and app style.
///

#pragma once

#include <QColor>

///
/// \brief macOS-flavoured raw colour values used to build the light/dark themes
///        (MacThemeFactory) and to resolve per-control colours (MacAppStyle).
///
/// Keeping the single source of truth here prevents the two consumers from
/// drifting apart. Values are opaque RGB; alpha variants are derived at use.
///
namespace MacPalette {

/// Light mode macOS colors
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

/// Dark mode macOS colors
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

} // namespace MacPalette
