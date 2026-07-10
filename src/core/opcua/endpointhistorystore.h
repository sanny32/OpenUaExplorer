// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QStringList>

///
/// \brief Persists the recently used OPC UA endpoint URLs.
///
class EndpointHistoryStore
{
public:
    ///
    /// \brief Returns the endpoint URL history, with the last-used URL moved to the front.
    /// \return Most-recent-first list of endpoint URLs.
    ///
    QStringList history() const;

    ///
    /// \brief Records an endpoint URL as most-recent, trimming the history to its cap.
    /// \param endpointUrl URL to store; blank values are ignored.
    ///
    void save(const QString &endpointUrl) const;

    ///
    /// \brief Drops an endpoint URL from the history, clearing it as the last-used URL.
    /// \param endpointUrl URL to forget; blank values are ignored.
    ///
    void remove(const QString &endpointUrl) const;
};
