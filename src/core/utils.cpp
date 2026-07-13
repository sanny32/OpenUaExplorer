// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file utils.cpp
/// \brief Implements small general-purpose helper functions.
///

#include <QCoreApplication>
#include <QDateTime>
#include <QFileInfo>
#include <QRegularExpression>

#include "utils.h"

///
/// \brief Returns the running executable's base name without its file suffix.
/// \return Executable base name, or an empty string when it cannot be determined.
///
QString Utils::executableBaseName()
{
    return QFileInfo(QCoreApplication::applicationFilePath()).completeBaseName().trimmed();
}

///
/// \brief Sanitizes a text into a segment usable inside a file name.
/// \param value Text to sanitize; whitespace and characters illegal in file names are replaced.
/// \param fallback Returned when the sanitized text is empty.
/// \return File-name-safe segment.
///
QString Utils::fileNameSegment(QString value, const QString &fallback)
{
    value = value.trimmed();
    static const QRegularExpression invalidChars(QStringLiteral(R"([<>:"/\\|?*\x00-\x1f]+)"));
    value.replace(invalidChars, QStringLiteral("_"));
    value.replace(QRegularExpression(QStringLiteral(R"(\s+)")), QStringLiteral("_"));
    while (value.endsWith(QLatin1Char('.')) || value.endsWith(QLatin1Char(' ')))
        value.chop(1);
    return value.isEmpty() ? fallback : value;
}

///
/// \brief Formats a date-time as a compact, file-name-safe stamp.
/// \param value Date-time to format.
/// \return File-name-safe date-time text.
///
QString Utils::fileNameDateTime(const QDateTime &value)
{
    return value.toString(QStringLiteral("yyyyMMdd_HHmmss"));
}
