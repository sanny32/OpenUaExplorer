// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file trendgraphstate.h
/// \brief Declares non-visual state for a trend graph.
///

#pragma once

#include <QSet>
#include <QString>

///
/// \brief Tracks live subscriptions and pending history reads for a trend graph.
///
class TrendGraphState
{
public:
    bool isSubscribed(const QString &nodeId) const;
    bool subscribe(const QString &nodeId);
    bool unsubscribe(const QString &nodeId);
    QSet<QString> subscribedNodes() const;
    void clearSubscriptions();

    void addPendingHistory(const QString &nodeId);
    bool consumePendingHistory(const QString &nodeId);
    void removePendingHistory(const QString &nodeId);
    void clearPendingHistory();

private:
    QSet<QString> _subscribedNodes;
    QSet<QString> _pendingHistoryNodes;
};
