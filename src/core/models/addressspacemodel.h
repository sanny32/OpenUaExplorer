// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file addressspacemodel.h
/// \brief Declares the lazy OPC UA address space tree model.
///

#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <QAbstractItemModel>
#include <QIcon>
#include <QVector>

#include "addressspaceitem.h"
#include "opcua/opcuatypes.h"

///
/// \brief Tree node used internally by AddressSpaceModel.
///
class AddressSpaceNode
{
public:
    ///
    /// \brief Constructs a tree node wrapping OPC UA node info.
    /// \param info OPC UA node information.
    /// \param parent Parent tree node.
    ///
    explicit AddressSpaceNode(const OpcUaNodeInfo &info, AddressSpaceNode *parent = nullptr);

    ///
    /// \brief Destroys the node and its children.
    ///
    ~AddressSpaceNode();

    ///
    /// \brief Appends a child node.
    /// \param child Child node.
    ///
    void appendChild(std::unique_ptr<AddressSpaceNode> child);

    ///
    /// \brief Removes all child nodes.
    ///
    void clearChildren();

    ///
    /// \brief Returns the child at a row.
    /// \param row Child row.
    /// \return Child node.
    ///
    AddressSpaceNode *child(int row) const;

    ///
    /// \brief Returns the number of loaded children.
    /// \return Number of loaded children.
    ///
    int childCount() const;

    ///
    /// \brief Returns this node's index within its parent.
    /// \return Row in the parent.
    ///
    int row() const;

    ///
    /// \brief Returns the parent node.
    /// \return Parent node.
    ///
    AddressSpaceNode *parent() const;

    ///
    /// \brief Returns the wrapped OPC UA node information.
    /// \return OPC UA node information.
    ///
    const OpcUaNodeInfo &info() const;

    ///
    /// \brief Reports whether a browse request was issued for this node.
    /// \return True after a browse request was issued.
    ///
    bool browseStarted() const;

    ///
    /// \brief Reports whether browse results were received for this node.
    /// \return True after browse results were received.
    ///
    bool browseComplete() const;

    ///
    /// \brief Sets whether a browse request was issued.
    /// \param value New state.
    ///
    void setBrowseStarted(bool value);

    ///
    /// \brief Sets whether browse results were received.
    /// \param value New state.
    ///
    void setBrowseComplete(bool value);

private:
    OpcUaNodeInfo _info;
    AddressSpaceNode *_parent;
    std::vector<std::unique_ptr<AddressSpaceNode>> _children;
    bool _browseStarted = false;
    bool _browseComplete = false;
};

///
/// \brief Lazy tree model for the OPC UA address space browser.
///
class AddressSpaceModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs the model with an empty invisible root.
    /// \param parent Parent object.
    ///
    explicit AddressSpaceModel(QObject *parent = nullptr);

    ///
    /// \brief Destroys the model and its node tree.
    ///
    ~AddressSpaceModel() override;

    ///
    /// \brief Returns the index for a child cell.
    /// \param row Child row.
    /// \param column Child column.
    /// \param parent Parent index.
    /// \return Model index.
    ///
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;

    ///
    /// \brief Returns the parent index of an item.
    /// \param index Child index.
    /// \return Parent index.
    ///
    QModelIndex parent(const QModelIndex &index) const override;

    ///
    /// \brief Returns the number of loaded children.
    /// \param parent Parent index.
    /// \return Loaded child count.
    ///
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    ///
    /// \brief Returns the single-column count.
    /// \param parent Parent index.
    /// \return Column count.
    ///
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    ///
    /// \brief Returns display name, tooltip, NodeId, or icon for an item.
    /// \param index Model index.
    /// \param role Data role.
    /// \return Requested data.
    ///
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    ///
    /// \brief Reports whether a node has, or may yet have, children.
    /// \param parent Parent index.
    /// \return True for loaded children or nodes that may be browsed.
    ///
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;

    ///
    /// \brief Reports whether a node still needs browsing.
    /// \param parent Parent index.
    /// \return True when the node still needs browsing.
    ///
    bool canFetchMore(const QModelIndex &parent) const override;

    ///
    /// \brief Requests a browse for a node that needs loading.
    /// \param parent Parent index.
    ///
    void fetchMore(const QModelIndex &parent) override;

    ///
    /// \brief Sets the single visible root node.
    /// \param root Visible root node.
    ///
    void setRootNode(const OpcUaNodeInfo &root);

    ///
    /// \brief Applies browse results under a parent node.
    /// \param parentNodeId Parent NodeId.
    /// \param children Browse results.
    ///
    void setChildren(const QString &parentNodeId, const QVector<OpcUaNodeInfo> &children);

    ///
    /// \brief Resets the browse state so a failed node can be retried.
    /// \param parentNodeId Parent NodeId.
    ///
    void setBrowseFailed(const QString &parentNodeId);

    ///
    /// \brief Builds a synthetic tree from test items.
    /// \param items Test tree items.
    ///
    void setItems(const QVector<AddressSpaceItem> &items);

    ///
    /// \brief Empties the tree.
    ///
    void clear();

    ///
    /// \brief Returns the node information for an index.
    /// \param index Model index.
    /// \return Node information.
    ///
    OpcUaNodeInfo nodeInfo(const QModelIndex &index) const;

    ///
    /// \brief Finds the index of a node by NodeId.
    /// \param nodeId NodeId to locate.
    /// \return Matching model index.
    ///
    QModelIndex findByNodeId(const QString &nodeId) const;

    ///
    /// \brief Finds the first index with a display name.
    /// \param displayName Display name to locate.
    /// \return Matching model index.
    ///
    QModelIndex findFirst(const QString &displayName) const;

    ///
    /// \brief Sets the callback that supplies node-class icons.
    /// \param provider Icon provider callback.
    ///
    void setIconProvider(std::function<QIcon(AddressSpaceItem::NodeType)> provider);

signals:
    ///
    /// \brief Emitted when a node's children must be browsed.
    /// \param nodeId NodeId to browse.
    ///
    void browseRequested(QString nodeId);

private:
    AddressSpaceNode *nodeForIndex(const QModelIndex &index) const;
    AddressSpaceNode *findNode(AddressSpaceNode *node, const QString &nodeId) const;
    QModelIndex indexForNode(AddressSpaceNode *node) const;
    QModelIndex findFirstRecursive(AddressSpaceNode *node, const QString &displayName) const;
    void appendTestItems(AddressSpaceNode *parent, const QVector<AddressSpaceItem> &items,
                         const QString &path);
    AddressSpaceItem::NodeType iconType(int nodeClass) const;

    std::unique_ptr<AddressSpaceNode> _root;
    std::function<QIcon(AddressSpaceItem::NodeType)> _iconProvider;
};
