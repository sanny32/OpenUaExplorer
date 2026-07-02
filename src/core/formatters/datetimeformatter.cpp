// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file datetimeformatter.cpp
/// \brief Implements the shared date-time display formatter.
///

#include "datetimeformatter.h"

#include <QDateTime>

///
/// \brief Formats a date-time in local time using the application's standard pattern.
/// \param value Date-time to format.
/// \return Text such as "2036-06-29 15:23:04", or an empty string when the value is invalid.
///
QString formatDateTime(const QDateTime &value)
{
    return value.isValid()
        ? value.toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))
        : QString();
}
