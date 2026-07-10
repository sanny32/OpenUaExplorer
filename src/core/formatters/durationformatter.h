// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file durationformatter.h
/// \brief Declares shared duration formatting helpers.
///

#pragma once

#include <QtGlobal>
#include <QString>

///
/// \brief Formats a millisecond duration as a compact label of its two largest units.
/// \param ms Duration in milliseconds.
/// \return A label such as "10 s", "10 min", "1 h 5 min" or "3 d 4 h".
///
QString formatDuration(qint64 ms);
