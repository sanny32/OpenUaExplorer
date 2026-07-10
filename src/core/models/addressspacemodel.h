// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file addressspacemodel.h
/// \brief Declares the lazy OPC UA address space tree model.
///

#pragma once

#include <functional>
#include <memory>

#include <QAbstractItemModel>
#include <QIcon>
#include <QVector>

#include "addressspaceitem.h"
#include "opcua/opcuatypes.h"

class QMimeData;
class AddressSpaceNode;

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
    /// \brief Returns item flags, including drag support for variable nodes.
    /// \param index Model index.
    /// \return Item flags.
    ///
    Qt::ItemFlags flags(const QModelIndex &index) const override;

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
    /// \brief Returns the MIME formats exported by dragged nodes.
    /// \return Supported MIME formats.
    ///
    QStringList mimeTypes() const override;

    ///
    /// \brief Encodes the dragged variable node.
    /// \param indexes Dragged model indexes.
    /// \return MIME data owned by the caller.
    ///
    QMimeData *mimeData(const QModelIndexList &indexes) const override;

    ///
    /// \brief Returns the supported drag action.
    /// \return Copy action.
    ///
    Qt::DropActions supportedDragActions() const override;

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
