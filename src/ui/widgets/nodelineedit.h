// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file nodelineedit.h
/// \brief Declares a line edit that displays an OPC UA node by path and NodeId.
///

#pragma once

#include "valuelineedit.h"

///
/// \brief Read-only line edit that shows a selected OPC UA node.
///
/// The field renders the node's display path (falling back to its display name)
/// together with the NodeId, e.g. "Objects/Device/Temperature (ns=2;s=Temp)",
/// and exposes the NodeId as its tooltip. The inherited reset action clears the
/// node and emits nodeCleared().
///
class NodeLineEdit : public ValueLineEdit
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs the line edit and wires the clear action.
    /// \param parent Parent widget.
    ///
    explicit NodeLineEdit(QWidget *parent = nullptr);

    ///
    /// \brief Returns the NodeId of the displayed node.
    /// \return NodeId string, or empty when no node is set.
    ///
    QString nodeId() const;

    ///
    /// \brief Returns the display name of the displayed node.
    /// \return Display name, or empty when no node is set.
    ///
    QString nodeDisplayName() const;

    ///
    /// \brief Returns the display path of the displayed node.
    /// \return Display path, or empty when none was provided.
    ///
    QString nodeDisplayPath() const;

    ///
    /// \brief Reports whether a node is currently displayed.
    /// \return True while a non-empty NodeId is set.
    ///
    bool hasNode() const;

    ///
    /// \brief Displays a node by its path and NodeId.
    /// \param nodeId Node's NodeId.
    /// \param displayName Human-readable node name.
    /// \param displayPath Human-readable path; preferred over the name when present.
    ///
    void setNode(const QString &nodeId, const QString &displayName,
                 const QString &displayPath = {});

    ///
    /// \brief Clears the displayed node without emitting nodeCleared().
    ///
    void clearNode();

signals:
    ///
    /// \brief Emitted when the user clears the field through the reset action.
    ///
    void nodeCleared();

private:
    void updateDisplay();

    QString _nodeId;
    QString _nodeDisplayName;
    QString _nodeDisplayPath;
};
