// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file featurecommands.h
/// \brief Declares stable feature command identifiers.
///

#pragma once

#include <QString>

namespace FeatureCommandIds {
inline QString logExport()
{
    return QStringLiteral("log.export");
}

inline QString addressSpaceBrowse()
{
    return QStringLiteral("addressSpace.browse");
}
}
