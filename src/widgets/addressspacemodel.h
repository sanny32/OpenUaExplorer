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
    explicit AddressSpaceNode(const OpcUaNodeInfo &info, AddressSpaceNode *parent = nullptr);
    ~AddressSpaceNode();

    void appendChild(std::unique_ptr<AddressSpaceNode> child);
    void clearChildren();

    AddressSpaceNode *child(int row) const;
    int childCount() const;
    int row() const;
    AddressSpaceNode *parent() const;
    const OpcUaNodeInfo &info() const;

    bool browseStarted() const;
    bool browseComplete() const;
    void setBrowseStarted(bool value);
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
    explicit AddressSpaceModel(QObject *parent = nullptr);
    ~AddressSpaceModel() override;

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;

    void setRootNode(const OpcUaNodeInfo &root);
    void setChildren(const QString &parentNodeId, const QVector<OpcUaNodeInfo> &children);
    void setBrowseFailed(const QString &parentNodeId);
    void setItems(const QVector<AddressSpaceItem> &items);
    void clear();

    OpcUaNodeInfo nodeInfo(const QModelIndex &index) const;
    QModelIndex findByNodeId(const QString &nodeId) const;
    QModelIndex findFirst(const QString &displayName) const;
    void setIconProvider(std::function<QIcon(AddressSpaceItem::NodeType)> provider);

signals:
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
