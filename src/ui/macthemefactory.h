// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#if defined(HAVE_QLEMENTINE_APP_STYLE)

#include <oclero/qlementine/style/Theme.hpp>

///
/// \brief Factory functions for macOS-flavoured Qlementine themes.
///
namespace MacThemeFactory {

///
/// \brief Builds the macOS light theme: palette, status colours, fonts, and metrics.
/// \return The configured light theme.
///
oclero::qlementine::Theme makeLightTheme();

///
/// \brief Builds the macOS dark theme: palette, status colours, fonts, and metrics.
/// \return The configured dark theme.
///
oclero::qlementine::Theme makeDarkTheme();

}

#endif
