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
}

///
/// \brief Clears the selected node and its details.
///
void SelectionContext::clear()
{
    _currentNode = {};
    _currentDetails = {};
    emit cleared();
}
