// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file recentconnectionstore.h
/// \brief Declares persistent storage for the most recent OPC UA connections.
///

#pragma once

#include <QList>

#include "connectionprofile.h"

///
/// \brief Persists the most recently used connection profiles, capped at a fixed count.
///
class RecentConnectionStore
{
public:
    /// \brief Maximum number of recent connections retained.
    static constexpr int maximumSize = 10;

    ///
    /// \brief Virtual destructor for polymorphic stores.
    ///
    virtual ~RecentConnectionStore() = default;

    ///
    /// \brief Returns the recent connections, most-recent first.
    /// \return Recent connection profiles.
    ///
    virtual QList<ConnectionProfile> connections() const;

    ///
    /// \brief Records a connection as most-recent, replacing any with the same endpoint URL
    ///        and trimming the list to its cap.
    /// \param profile Profile that was connected.
    ///
    virtual void record(const ConnectionProfile &profile);
};
