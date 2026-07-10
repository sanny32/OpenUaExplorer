// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file nodelineedit.cpp
/// \brief Implements a line edit that displays an OPC UA node by path and NodeId.
///

#include "nodelineedit.h"

#include <utility>

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>

#include "models/addressspacemimedata.h"

///
/// \brief Constructs the line edit and wires the clear action.
/// \param parent Parent widget.
///
NodeLineEdit::NodeLineEdit(QWidget *parent)
    : ValueLineEdit(parent)
{
    setAcceptDrops(true);
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
/// \brief Displays a node described by browsed address-space information.
/// \param node Node to display; the display name falls back to the browse name
///        and then the NodeId.
///
void NodeLineEdit::setNode(const OpcUaNodeInfo &node)
{
    const QString label = node.displayName.isEmpty()
        ? (node.browseName.isEmpty() ? node.nodeId : node.browseName)
        : node.displayName;
    setNode(node.nodeId, label, node.displayPath);
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
/// \brief Restricts which dragged nodes the field accepts.
/// \param acceptor Predicate returning true for droppable nodes; an empty
///        acceptor accepts any node with a NodeId.
///
void NodeLineEdit::setNodeAcceptor(NodeAcceptor acceptor)
{
    _acceptNode = std::move(acceptor);
}

///
/// \brief Mirrors the drag-move policy so entering the field shows the verdict.
///
void NodeLineEdit::dragEnterEvent(QDragEnterEvent *event)
{
    dragMoveEvent(event);
}

///
/// \brief Accepts the drag as a copy while it carries a droppable node.
///
void NodeLineEdit::dragMoveEvent(QDragMoveEvent *event)
{
    OpcUaNodeInfo node;
    if (acceptsDrag(event->mimeData(), &node)) {
        event->setDropAction(Qt::CopyAction);
        event->accept();
    } else {
        event->ignore();
    }
}

///
/// \brief Displays the dropped node and notifies observers.
///
void NodeLineEdit::dropEvent(QDropEvent *event)
{
    OpcUaNodeInfo node;
    if (!acceptsDrag(event->mimeData(), &node)) {
        event->ignore();
        return;
    }
    setNode(node);
    event->setDropAction(Qt::CopyAction);
    event->accept();
    emit nodeDropped(node);
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

///
/// \brief Decodes a dragged node and checks it against the acceptor.
/// \param mimeData Drag MIME data.
/// \param node Destination for the decoded node.
/// \return True when the data holds a node with a NodeId the acceptor allows.
///
bool NodeLineEdit::acceptsDrag(const QMimeData *mimeData, OpcUaNodeInfo *node) const
{
    if (!AddressSpaceMime::decodeNode(mimeData, node) || node->nodeId.isEmpty())
        return false;
    return !_acceptNode || _acceptNode(*node);
}
