// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file trendgraphstate.cpp
/// \brief Implements non-visual state for a trend graph.
///

#include "trendgraphstate.h"

///
/// \brief Reports whether a node has an active live subscription.
/// \param nodeId Node to query.
/// \return True when subscribed.
///
bool TrendGraphState::isSubscribed(const QString &nodeId) const
{
    return _subscribedNodes.contains(nodeId);
}

///
/// \brief Adds a live subscription if it is not already tracked.
/// \param nodeId Node to subscribe.
/// \return True when the node was newly tracked.
///
bool TrendGraphState::subscribe(const QString &nodeId)
{
    if (nodeId.isEmpty() || _subscribedNodes.contains(nodeId))
        return false;
    _subscribedNodes.insert(nodeId);
    return true;
}

///
/// \brief Removes a live subscription.
/// \param nodeId Node to unsubscribe.
/// \return True when the node was tracked.
///
bool TrendGraphState::unsubscribe(const QString &nodeId)
{
    return _subscribedNodes.remove(nodeId);
}

///
/// \brief Returns a copy of the subscribed nodes.
/// \return Subscribed node ids.
///
QSet<QString> TrendGraphState::subscribedNodes() const
{
    return _subscribedNodes;
}

///
/// \brief Clears all live subscriptions.
///
void TrendGraphState::clearSubscriptions()
{
    _subscribedNodes.clear();
}

///
/// \brief Marks a node's history read as pending.
/// \param nodeId Node being read.
///
void TrendGraphState::addPendingHistory(const QString &nodeId)
{
    if (!nodeId.isEmpty())
        _pendingHistoryNodes.insert(nodeId);
}

///
/// \brief Consumes a pending history marker.
/// \param nodeId Node whose history arrived.
/// \return True when the node had a pending history read.
///
bool TrendGraphState::consumePendingHistory(const QString &nodeId)
{
    return _pendingHistoryNodes.remove(nodeId);
}

///
/// \brief Removes a pending history marker without reporting it consumed.
/// \param nodeId Node to remove.
///
void TrendGraphState::removePendingHistory(const QString &nodeId)
{
    _pendingHistoryNodes.remove(nodeId);
}

///
/// \brief Clears all pending history reads.
///
void TrendGraphState::clearPendingHistory()
{
    _pendingHistoryNodes.clear();
}
