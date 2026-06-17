// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file addressspacewidget.h
/// \brief Declares the OPC UA address space browser widget.
///

#pragma once

#include <QWidget>

#include "addressspaceitem.h"
#include "opcua/opcuatypes.h"

namespace Ui {
class AddressSpaceWidget;
}

class AddressSpaceModel;
class NodeInfoModel;
class ReferencesModel;

///
/// \brief Widget for browsing the OPC UA address space and selected node details.
///
class AddressSpaceWidget : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the browser, wiring its tree, node-info, and references views.
    /// \param parent Parent widget.
    ///
    explicit AddressSpaceWidget(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the widget and its generated UI.
    ///
    ~AddressSpaceWidget() override;

    ///
    /// \brief Sets the tree's root node and selects it.
    /// \param root Root folder information.
    ///
    void setRootNode(const OpcUaNodeInfo &root);

    ///
    /// \brief Applies browse results to the tree and refreshes the references view if selected.
    /// \param parentNodeId Parent NodeId.
    /// \param children Browse result.
    /// \param error Browse error.
    ///
    void setBrowseChildren(const QString &parentNodeId,
                           const QVector<OpcUaNodeInfo> &children,
                           const QString &error);

    ///
    /// \brief Populates the node-info table from the selected node's details.
    /// \param details Selected node details.
    ///
    void setNodeDetails(const OpcUaNodeDetails &details);

    ///
    /// \brief Clears the tree, node-info, and references views.
    ///
    void clear();

    ///
    /// \brief Returns the node currently selected in the tree.
    /// \return Currently selected OPC UA node.
    ///
    OpcUaNodeInfo selectedNode() const;

signals:
    ///
    /// \brief Emitted when a node's children must be browsed.
    /// \param nodeId NodeId to browse.
    ///
    void browseRequested(QString nodeId);

    ///
    /// \brief Emitted when the user selects a node in the tree.
    /// \param node Selected node.
    ///
    void nodeSelected(OpcUaNodeInfo node);

    ///
    /// \brief Emitted when the user requests a refresh of a node.
    /// \param nodeId Node to re-browse.
    ///
    void refreshRequested(QString nodeId);

private:
    void setupTreeView();
    void setupNodeInfoView();
    void setupReferencesView();

    Ui::AddressSpaceWidget *ui;
    AddressSpaceModel      *_treeModel;
    NodeInfoModel          *_nodeInfoModel;
    ReferencesModel        *_referencesModel;
    QString                 _selectedNodeId;
};
