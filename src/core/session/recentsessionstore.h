// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file recentsessionstore.h
/// \brief Declares persistent storage for the most recently used session files.
///

#pragma once

#include <QString>
#include <QStringList>

///
/// \brief Persists the paths of recently opened or saved session files, capped at a fixed count.
///
class RecentSessionStore
{
public:
    /// \brief Maximum number of recent sessions retained.
    static constexpr int maximumSize = 10;

    ///
    /// \brief Returns the recent session file paths, most-recent first.
    /// \return Recent session paths.
    ///
    QStringList sessions() const;

    ///
    /// \brief Records a session file as most-recent, de-duplicating and trimming to the cap.
    /// \param path Session file path that was opened or saved.
    ///
    void record(const QString &path);

    ///
    /// \brief Removes a session file from the recent list.
    /// \param path Session file path to forget.
    ///
    void remove(const QString &path);
};
