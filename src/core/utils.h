// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file utils.h
/// \brief Declares small general-purpose helper functions.
///

#pragma once

#include <QString>

///
/// \brief General-purpose helper functions shared across the application.
///
namespace Utils {

///
/// \brief Returns the running executable's base name without its file suffix.
/// \return Executable base name, or an empty string when it cannot be determined.
///
QString executableBaseName();

} // namespace Utils
