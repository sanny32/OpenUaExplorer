// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file logitem.h
/// \brief Declares log item data types.
///

#pragma once

#include <QString>

///
/// \brief Describes one application log message.
///
struct LogItem
{
    enum class Level { Info, Warning, Error };

    QString timestamp;
    Level   level;
    QString source;
    QString message;
};
