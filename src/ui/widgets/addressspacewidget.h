// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file addressspacewidget.h
/// \brief Declares the OPC UA address space browser widget.
///

#pragma once

#include <QHash>
#include <QSet>
#include <QStringList>
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
class SpinnerAction;

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
    /// \brief Updates the tracked monitoring state shown in the context menu.
    /// \param nodeId Affected node.
    /// \param subscribed True when the node is being monitored.
    ///
    void setNodeSubscribed(const QString &nodeId, bool subscribed);

    ///
    /// \brief Returns the node currently selected in the tree.
    /// \return Currently selected OPC UA node.
    ///
    OpcUaNodeInfo selectedNode() const;

    ///
    /// \brief Returns the node ids of the expanded tree items, parents before children.
    /// \return Expanded node ids in top-down order.
    ///
    QStringList expandedNodeIds() const;

    ///
    /// \brief Re-expands saved tree nodes and reselects a node as they load.
    ///
    /// Expansion is applied incrementally: nodes already loaded are expanded now,
    /// and deeper nodes are expanded as their parents' browse results arrive.
    /// \param expandedNodeIds Node ids to expand, parents before children.
    /// \param selectedNodeId Node id to select once it is loaded, or empty.
    ///
    void restoreExpansion(const QStringList &expandedNodeIds, const QString &selectedNodeId);

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

    ///
    /// \brief Reports how many nodes the running search has visited.
    /// \param visitedNodes Number of unique nodes visited so far.
    ///
    void setSearchProgress(int visitedNodes);

    ///
    /// \brief Reveals the search match, or reports that the search found nothing.
    /// \param ancestorNodeIds Node ids from the search root down to the match's parent.
    /// \param nodeId Matched NodeId, empty when nothing matched.
    /// \param error Search error, empty on success.
    ///
    void setSearchResult(const QStringList &ancestorNodeIds, const QString &nodeId,
                         const QString &error);

signals:
    ///
    /// \brief Emitted when the user starts a server-side search of the address space.
    /// \param startNodeId Node whose subtree is searched.
    /// \param pattern Case-insensitive substring matched against display names.
    ///
    void searchRequested(QString startNodeId, QString pattern);

    ///
    /// \brief Emitted when the user abandons a running search.
    ///
    void searchCancelRequested();

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

    ///
    /// \brief Emitted when the user requests an event history read for a node.
    /// \param node Node whose event history should be read.
    ///
    void readEventsHistoryRequested(OpcUaNodeInfo node);

    ///
    /// \brief Emitted when the user requests charting a node in the trend panel.
    /// \param node Variable node to chart.
    ///
    void addToTrendRequested(OpcUaNodeInfo node);

    ///
    /// \brief Emitted when the user requests monitoring a node in the node monitor.
    /// \param node Variable node to monitor.
    ///
    void monitorNodeRequested(OpcUaNodeInfo node);

    ///
    /// \brief Emitted when the user requests monitoring a variable node.
    /// \param node Variable node to subscribe.
    ///
    void subscribeRequested(OpcUaNodeInfo node);

    ///
    /// \brief Emitted when the user requests to stop monitoring a variable node.
    /// \param node Variable node to unsubscribe.
    ///
    void unsubscribeRequested(OpcUaNodeInfo node);

    ///
    /// \brief Emitted when the user requests calling a method node.
    /// \param object Object node that owns the method.
    /// \param method Method node to call.
    ///
    void callMethodRequested(OpcUaNodeInfo object, OpcUaNodeInfo method);

private:
    void setupTreeView();
    void setupNodeInfoView();
    void setupReferencesView();
    void setupSearch();
    void startSearch();
    void cancelSearch();
    void setSearchBusy(bool busy);
    void setSearchFailure(const QString &text);
    void applyPendingExpansion();
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
    QSet<QString>           _subscribedNodeIds;
    QHash<QString, QVector<ReferenceItem>> _referencesByNodeId;
    QStringList             _pendingExpand;
    QString                 _pendingSelect;
    SpinnerAction          *_searchSpinner = nullptr;
    QString                 _searchPattern;
    bool                    _searchMatched = false;
    bool                    _searching = false;
};
