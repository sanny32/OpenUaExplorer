// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dataaccessmonitoringstate.cpp
/// \brief Implements monitoring state tracked by the data-access coordinator.
///

#include "dataaccessmonitoringstate.h"

///
/// \brief Reports whether a node is currently subscribed.
/// \param nodeId Node to query.
/// \return True when the node is subscribed.
///
bool DataAccessMonitoringState::isSubscribed(const QString &nodeId) const
{
    return _subscribedNodeIds.contains(nodeId);
}

///
/// \brief Reports whether a monitoring request is in flight for a node.
/// \param nodeId Node to query.
/// \return True when a request is pending.
///
bool DataAccessMonitoringState::isPending(const QString &nodeId) const
{
    return _pendingNodeIds.contains(nodeId);
}

///
/// \brief Marks a node's monitoring request as pending.
/// \param nodeId Node being requested.
///
void DataAccessMonitoringState::beginRequest(const QString &nodeId)
{
    if (!nodeId.isEmpty())
        _pendingNodeIds.insert(nodeId);
}

///
/// \brief Applies the result of a monitoring request.
/// \param nodeId Node whose request finished.
/// \param subscribed Resulting subscription state.
/// \param success Whether the request succeeded.
///
void DataAccessMonitoringState::finishRequest(const QString &nodeId, bool subscribed, bool success)
{
    _pendingNodeIds.remove(nodeId);
    if (!success)
        return;
    if (subscribed)
        _subscribedNodeIds.insert(nodeId);
    else
        _subscribedNodeIds.remove(nodeId);
}

///
/// \brief Clears all subscribed and pending nodes.
///
void DataAccessMonitoringState::clear()
{
    _subscribedNodeIds.clear();
    _pendingNodeIds.clear();
}
