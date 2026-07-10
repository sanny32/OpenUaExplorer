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
    ///
    /// \brief Log severity level.
    ///
    enum class Level { Info, Warning, Error };

    /// \brief Formatted log timestamp.
    QString timestamp;
    /// \brief Severity level.
    Level   level;
    /// \brief Log source name.
    QString source;
    /// \brief Log message text.
    QString message;
};
