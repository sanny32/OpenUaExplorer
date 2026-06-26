// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file addressspacewidget.h
/// \brief Declares the OPC UA address space browser widget.
///

#pragma once

#include <QHash>
#include <QWidget>

#include "models/addressspaceitem.h"
#include "models/nodeitem.h"
#include "opcua/opcuatypes.h"

class QModelIndex;

namespace Ui {
class AddressSpaceWidget;
}

class AppSettings;
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
    /// \brief Applies browse results to the tree.
    /// \param parentNodeId Parent NodeId.
    /// \param children Browse result.
    /// \param error Browse error.
    ///
    void setBrowseChildren(const QString &parentNodeId,
                           const QVector<OpcUaNodeInfo> &children,
                           const QString &error);

    ///
    /// \brief Applies reference browse results to the references view if selected.
    /// \param sourceNodeId Source NodeId.
    /// \param references Reference browse result.
    /// \param error Browse error.
    ///
    void setBrowseReferences(const QString &sourceNodeId,
                             const QVector<OpcUaNodeInfo> &references,
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

    ///
    /// \brief Detaches the node details panel so MainWindow can host it in a dock.
    /// \return Node details panel.
    ///
    QWidget *takeNodeDetailsPanel();

    ///
    /// \brief Persists the tree, node-info, and references header state.
    /// \param settings Settings store to write to.
    ///
    void saveViewState(AppSettings &settings) const;

    ///
    /// \brief Restores the tree, node-info, and references header state.
    /// \param settings Settings store to read from.
    ///
    void restoreViewState(AppSettings &settings);

signals:
    ///
    /// \brief Emitted when a node's children must be browsed.
    /// \param nodeId NodeId to browse.
    ///
    void browseRequested(QString nodeId);

    ///
    /// \brief Emitted when a node's references must be browsed.
    /// \param nodeId NodeId to browse.
    ///
    void referencesRequested(QString nodeId);

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

    ///
    /// \brief Emitted when the user requests a history read for a node.
    /// \param node Node whose history should be read.
    ///
    void readHistoryRequested(OpcUaNodeInfo node);

    ///
    /// \brief Emitted when the user requests event monitoring for a node.
    /// \param node Node to monitor for events.
    ///
    void monitorEventsRequested(OpcUaNodeInfo node);

private:
    void setupTreeView();
    void setupNodeInfoView();
    void setupReferencesView();
    void showTreeContextMenu(const QPoint &pos);
    void onCurrentNodeChanged(const QModelIndex &current);
    void updateReferencesForNode(const QString &nodeId);
    QVector<ReferenceItem> referencesFromChildren(const QVector<OpcUaNodeInfo> &children) const;
    QString referenceTypeDisplayName(const QString &referenceTypeId) const;

    Ui::AddressSpaceWidget *ui;
    AddressSpaceModel      *_treeModel;
    NodeInfoModel          *_nodeInfoModel;
    ReferencesModel        *_referencesModel;
    QString                 _selectedNodeId;
    QHash<QString, QVector<ReferenceItem>> _referencesByNodeId;
};
