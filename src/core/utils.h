// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file utils.h
/// \brief Declares small general-purpose helper functions.
///

#pragma once

#include <QString>

class QDateTime;

///
/// \brief General-purpose helper functions shared across the application.
///
namespace Utils {

///
/// \brief Returns the running executable's base name without its file suffix.
/// \return Executable base name, or an empty string when it cannot be determined.
///
QString executableBaseName();

///
/// \brief Sanitizes a text into a segment usable inside a file name.
/// \param value Text to sanitize; whitespace and characters illegal in file names are replaced.
/// \param fallback Returned when the sanitized text is empty.
/// \return File-name-safe segment.
///
QString fileNameSegment(QString value, const QString &fallback);

///
/// \brief Formats a date-time as a compact, file-name-safe stamp.
/// \param value Date-time to format.
/// \return File-name-safe date-time text.
///
QString fileNameDateTime(const QDateTime &value);

} // namespace Utils
