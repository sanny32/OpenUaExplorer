// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file durationformatter.cpp
/// \brief Implements shared duration formatting helpers.
///

#include "durationformatter.h"

#include <QStringList>

namespace {

///
/// \brief Appends a non-zero duration part.
/// \param parts Destination list.
/// \param value Unit value.
/// \param unit Unit label.
///
void appendDurationPart(QStringList *parts, qint64 value, const QString &unit)
{
    if (value > 0)
        parts->append(QStringLiteral("%1 %2").arg(value).arg(unit));
}

} // namespace

///
/// \brief Formats a millisecond duration as a compact label of its two largest units.
/// \param ms Duration in milliseconds.
/// \return A label such as "10 s", "10 min", "1 h 5 min" or "3 d 4 h".
///
QString formatDuration(qint64 ms)
{
    qint64 seconds = ms / 1000;
    if (seconds < 0)
        seconds = 0;

    const qint64 days = seconds / 86400;
    seconds %= 86400;
    const qint64 hours = seconds / 3600;
    seconds %= 3600;
    const qint64 minutes = seconds / 60;
    seconds %= 60;

    QStringList parts;
    appendDurationPart(&parts, days, QStringLiteral("d"));
    appendDurationPart(&parts, hours, QStringLiteral("h"));
    appendDurationPart(&parts, minutes, QStringLiteral("min"));
    appendDurationPart(&parts, seconds, QStringLiteral("s"));
    if (parts.isEmpty())
        parts.append(QStringLiteral("0 s"));
    while (parts.size() > 2)
        parts.removeLast();
    return parts.join(QLatin1Char(' '));
}
