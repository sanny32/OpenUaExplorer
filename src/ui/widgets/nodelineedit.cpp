// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file nodelineedit.cpp
/// \brief Implements a line edit that displays an OPC UA node by path and NodeId.
///

#include "nodelineedit.h"

///
/// \brief Constructs the line edit and wires the clear action.
/// \param parent Parent widget.
///
NodeLineEdit::NodeLineEdit(QWidget *parent)
    : ValueLineEdit(parent)
{
    setResetToolTip(tr("Clear node"));
    connect(this, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (text.isEmpty() && hasNode()) {
            emit nodeCleared();
            _nodeId.clear();
            _nodeDisplayName.clear();
            _nodeDisplayPath.clear();
            setToolTip(QString());
        }
    });
}

///
/// \brief Returns the NodeId of the displayed node.
/// \return NodeId string, or empty when no node is set.
///
QString NodeLineEdit::nodeId() const
{
    return _nodeId;
}

///
/// \brief Returns the display name of the displayed node.
/// \return Display name, or empty when no node is set.
///
QString NodeLineEdit::nodeDisplayName() const
{
    return _nodeDisplayName;
}

///
/// \brief Returns the display path of the displayed node.
/// \return Display path, or empty when none was provided.
///
QString NodeLineEdit::nodeDisplayPath() const
{
    return _nodeDisplayPath;
}

///
/// \brief Reports whether a node is currently displayed.
/// \return True while a non-empty NodeId is set.
///
bool NodeLineEdit::hasNode() const
{
    return !_nodeId.isEmpty();
}

///
/// \brief Displays a node by its path and NodeId.
/// \param nodeId Node's NodeId.
/// \param displayName Human-readable node name.
/// \param displayPath Human-readable path; preferred over the name when present.
///
void NodeLineEdit::setNode(const QString &nodeId, const QString &displayName,
                           const QString &displayPath)
{
    _nodeId = nodeId;
    _nodeDisplayName = displayName;
    _nodeDisplayPath = displayPath;
    updateDisplay();
}

///
/// \brief Clears the displayed node without emitting nodeCleared().
///
void NodeLineEdit::clearNode()
{
    _nodeId.clear();
    _nodeDisplayName.clear();
    _nodeDisplayPath.clear();
    setToolTip(QString());
    clear();
}

///
/// \brief Renders the current node as "path (NodeId)" with the NodeId as tooltip.
///
void NodeLineEdit::updateDisplay()
{
    const QString label = _nodeDisplayPath.isEmpty() ? _nodeDisplayName : _nodeDisplayPath;
    setText(label.isEmpty() || label == _nodeId
                ? _nodeId
                : QStringLiteral("%1 (%2)").arg(label, _nodeId));
    setToolTip(_nodeId);
}
