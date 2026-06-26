// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file selectioncontext.cpp
/// \brief Implements the selected-node mediator shared by UI features.
///

#include "selectioncontext.h"

///
/// \brief Constructs an empty selection context.
/// \param parent Owning QObject.
///
SelectionContext::SelectionContext(QObject *parent)
    : QObject(parent)
{
}

///
/// \brief Returns the currently selected address-space node.
/// \return Selected node, or an empty node when nothing is selected.
///
OpcUaNodeInfo SelectionContext::currentNode() const
{
    return _currentNode;
}

///
/// \brief Returns the details for the selected node.
/// \return Selected node details, or an empty value when none are loaded.
///
OpcUaNodeDetails SelectionContext::currentDetails() const
{
    return _currentDetails;
}

///
/// \brief Selects a node and requests that listeners refresh their state.
/// \param node Node selected by a feature.
///
void SelectionContext::selectNode(const OpcUaNodeInfo &node)
{
    _currentNode = node;
    _currentDetails = {};
    emit nodeSelected(node);
}

///
/// \brief Publishes node details when they belong to the current selection.
/// \param details Read node details.
/// \param error Read error, empty on success.
///
void SelectionContext::setDetails(const OpcUaNodeDetails &details, const QString &error)
{
    if (error.isEmpty() && details.nodeId != _currentNode.nodeId)
        return;
    if (!error.isEmpty() && !details.nodeId.isEmpty() && details.nodeId != _currentNode.nodeId)
        return;

    if (error.isEmpty())
        _currentDetails = details;
    emit detailsReady(details, error);

    if (_pendingEventNodeId == details.nodeId) {
        _pendingEventNodeId.clear();
        if (error.isEmpty() && OpcUa::canMonitorEvents(details))
            emit eventMonitorRequested(_currentNode);
    }

    if (_pendingHistoryNodeId != details.nodeId)
        return;
    _pendingHistoryNodeId.clear();
    if (error.isEmpty() && OpcUa::canReadHistory(details))
        emit historyReadRequested(_currentNode);
}

///
/// \brief Clears the selected node and its details.
///
void SelectionContext::clear()
{
    _currentNode = {};
    _currentDetails = {};
    _pendingHistoryNodeId.clear();
    _pendingEventNodeId.clear();
    emit cleared();
}

///
/// \brief Requests a history read for a node selected from a feature.
/// \param node Node whose history should be read.
///
void SelectionContext::requestHistory(const OpcUaNodeInfo &node)
{
    if (_currentDetails.nodeId == node.nodeId && OpcUa::canReadHistory(_currentDetails)) {
        emit historyReadRequested(node);
        return;
    }

    _pendingHistoryNodeId = node.nodeId;
    if (_currentNode.nodeId != node.nodeId || _currentDetails.nodeId != node.nodeId)
        selectNode(node);
}

///
/// \brief Requests event monitoring for a node selected from a feature.
/// \param node Node to monitor for events.
///
void SelectionContext::requestEventMonitor(const OpcUaNodeInfo &node)
{
    if (_currentDetails.nodeId == node.nodeId && OpcUa::canMonitorEvents(_currentDetails)) {
        emit eventMonitorRequested(node);
        return;
    }

    _pendingEventNodeId = node.nodeId;
    if (_currentNode.nodeId != node.nodeId || _currentDetails.nodeId != node.nodeId)
        selectNode(node);
}
