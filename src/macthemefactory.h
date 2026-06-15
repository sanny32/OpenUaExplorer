// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#if defined(HAVE_QLEMENTINE_APP_STYLE)

#include <oclero/qlementine/style/Theme.hpp>

namespace MacThemeFactory {
oclero::qlementine::Theme makeLightTheme();
oclero::qlementine::Theme makeDarkTheme();
}

#endif
