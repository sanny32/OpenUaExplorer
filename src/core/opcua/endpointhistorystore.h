// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QString>
#include <QStringList>

///
/// \brief Persists the recently used OPC UA endpoint URLs.
///
/// Each settings group keeps an independent list, so dialogs that collect a different
/// kind of URL (an endpoint versus a discovery server) do not share a history.
///
class EndpointHistoryStore
{
public:
    ///
    /// \brief Binds the store to a settings group.
    /// \param group Settings group holding the history keys.
    ///
    explicit EndpointHistoryStore(QString group = QStringLiteral("connectionDialog"));

    ///
    /// \brief Stores default endpoint URLs only when history settings do not exist yet.
    /// \param endpointUrls Default URLs shown on the first application run.
    ///
    void seedIfUninitialized(const QStringList &endpointUrls) const;

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

private:
    QString _lastUrlKey;
    QString _historyKey;
};
