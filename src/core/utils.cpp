// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file utils.cpp
/// \brief Implements small general-purpose helper functions.
///

#include <QCoreApplication>
#include <QFileInfo>

#include "utils.h"

///
/// \brief Returns the running executable's base name without its file suffix.
/// \return Executable base name, or an empty string when it cannot be determined.
///
QString Utils::executableBaseName()
{
    return QFileInfo(QCoreApplication::applicationFilePath()).completeBaseName().trimmed();
}
