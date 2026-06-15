// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#if defined(HAVE_QLEMENTINE_APP_STYLE)

#include <oclero/qlementine/style/Theme.hpp>

namespace QlementineThemeFactory {
oclero::qlementine::Theme make(const oclero::qlementine::Theme &baseTheme,
                               bool darkMode);
}

#endif
