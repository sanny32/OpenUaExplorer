// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file addressspacemodel.cpp
/// \brief Implements the lazy OPC UA address space tree model.
///

#include "addressspacemodel.h"

///
/// \brief AddressSpaceNode::AddressSpaceNode
/// \param info OPC UA node information.
/// \param parent Parent tree node.
///
AddressSpaceNode::AddressSpaceNode(const OpcUaNodeInfo &info, AddressSpaceNode *parent)
    : _info(info)
    , _parent(parent)
{
}

///
/// \brief AddressSpaceNode::~AddressSpaceNode
///
AddressSpaceNode::~AddressSpaceNode()
{
    qDeleteAll(_children);
}

///
/// \brief AddressSpaceNode::appendChild
/// \param child Child node.
///
void AddressSpaceNode::appendChild(AddressSpaceNode *child)
{
    _children.append(child);
}

///
/// \brief AddressSpaceNode::clearChildren
///
void AddressSpaceNode::clearChildren()
{
    qDeleteAll(_children);
    _children.clear();
}

///
/// \brief AddressSpaceNode::child
/// \param row Child row.
/// \return Child node.
///
AddressSpaceNode *AddressSpaceNode::child(int row) const
{
    return _children.value(row, nullptr);
}

///
/// \brief AddressSpaceNode::childCount
/// \return Number of loaded children.
///
int AddressSpaceNode::childCount() const
{
    return _children.size();
}

///
/// \brief AddressSpaceNode::row
/// \return Row in the parent.
///
int AddressSpaceNode::row() const
{
    return _parent ? _parent->_children.indexOf(const_cast<AddressSpaceNode *>(this)) : 0;
}

///
/// \brief AddressSpaceNode::parent
/// \return Parent node.
///
AddressSpaceNode *AddressSpaceNode::parent() const
{
    return _parent;
}

///
/// \brief AddressSpaceNode::info
/// \return OPC UA node information.
///
const OpcUaNodeInfo &AddressSpaceNode::info() const
{
    return _info;
}

///
/// \brief AddressSpaceNode::browseStarted
/// \return True after a browse request was issued.
///
bool AddressSpaceNode::browseStarted() const
{
    return _browseStarted;
}

///
/// \brief AddressSpaceNode::browseComplete
/// \return True after browse results were received.
///
bool AddressSpaceNode::browseComplete() const
{
    return _browseComplete;
}

///
/// \brief AddressSpaceNode::setBrowseStarted
/// \param value New state.
///
void AddressSpaceNode::setBrowseStarted(bool value)
{
    _browseStarted = value;
}

///
/// \brief AddressSpaceNode::setBrowseComplete
/// \param value New state.
///
void AddressSpaceNode::setBrowseComplete(bool value)
{
    _browseComplete = value;
}

///
/// \brief AddressSpaceModel::AddressSpaceModel
/// \param parent Parent object.
///
AddressSpaceModel::AddressSpaceModel(QObject *parent)
    : QAbstractItemModel(parent)
    , _root(new AddressSpaceNode({}))
{
    _root->setBrowseComplete(true);
}

///
/// \brief AddressSpaceModel::~AddressSpaceModel
///
AddressSpaceModel::~AddressSpaceModel()
{
    delete _root;
}

///
/// \brief AddressSpaceModel::index
/// \param row Child row.
/// \param column Child column.
/// \param parent Parent index.
/// \return Model index.
///
QModelIndex AddressSpaceModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return {};
    AddressSpaceNode *child = nodeForIndex(parent)->child(row);
    return child ? createIndex(row, column, child) : QModelIndex();
}

///
/// \brief AddressSpaceModel::parent
/// \param index Child index.
/// \return Parent index.
///
QModelIndex AddressSpaceModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return {};
    AddressSpaceNode *parentNode = nodeForIndex(index)->parent();
    if (!parentNode || parentNode == _root)
        return {};
    return createIndex(parentNode->row(), 0, parentNode);
}

///
/// \brief AddressSpaceModel::rowCount
/// \param parent Parent index.
/// \return Loaded child count.
///
int AddressSpaceModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;
    return nodeForIndex(parent)->childCount();
}

///
/// \brief AddressSpaceModel::columnCount
/// \param parent Parent index.
/// \return Column count.
///
int AddressSpaceModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 1;
}

///
/// \brief AddressSpaceModel::data
/// \param index Model index.
/// \param role Data role.
/// \return Requested data.
///
QVariant AddressSpaceModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};
    const OpcUaNodeInfo &info = nodeForIndex(index)->info();
    if (role == Qt::DisplayRole)
        return info.displayName.isEmpty() ? info.browseName : info.displayName;
    if (role == Qt::ToolTipRole)
        return info.nodeId;
    if (role == Qt::UserRole)
        return info.nodeId;
    if (role == Qt::DecorationRole && _iconProvider)
        return _iconProvider(iconType(info.nodeClass));
    return {};
}

///
/// \brief AddressSpaceModel::hasChildren
/// \param parent Parent index.
/// \return True for loaded children or nodes that may be browsed.
///
bool AddressSpaceModel::hasChildren(const QModelIndex &parent) const
{
    AddressSpaceNode *node = nodeForIndex(parent);
    return node->childCount() > 0 || (!node->browseComplete() && node->info().hasChildren);
}

///
/// \brief AddressSpaceModel::canFetchMore
/// \param parent Parent index.
/// \return True when the node still needs browsing.
///
bool AddressSpaceModel::canFetchMore(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return false;
    AddressSpaceNode *node = nodeForIndex(parent);
    return node->info().hasChildren && !node->browseStarted() && !node->browseComplete();
}

///
/// \brief AddressSpaceModel::fetchMore
/// \param parent Parent index.
///
void AddressSpaceModel::fetchMore(const QModelIndex &parent)
{
    AddressSpaceNode *node = nodeForIndex(parent);
    if (!canFetchMore(parent))
        return;
    node->setBrowseStarted(true);
    emit browseRequested(node->info().nodeId);
}

///
/// \brief AddressSpaceModel::setRootNode
/// \param root Visible root node.
///
void AddressSpaceModel::setRootNode(const OpcUaNodeInfo &root)
{
    beginResetModel();
    delete _root;
    _root = new AddressSpaceNode({});
    _root->setBrowseComplete(true);
    _root->appendChild(new AddressSpaceNode(root, _root));
    endResetModel();
}

///
/// \brief AddressSpaceModel::setChildren
/// \param parentNodeId Parent NodeId.
/// \param children Browse results.
///
void AddressSpaceModel::setChildren(const QString &parentNodeId,
                                    const QVector<OpcUaNodeInfo> &children)
{
    AddressSpaceNode *parentNode = findNode(_root, parentNodeId);
    if (!parentNode)
        return;
    const QModelIndex parentIndex = indexForNode(parentNode);
    if (parentNode->childCount() > 0) {
        beginRemoveRows(parentIndex, 0, parentNode->childCount() - 1);
        parentNode->clearChildren();
        endRemoveRows();
    }
    if (!children.isEmpty()) {
        beginInsertRows(parentIndex, 0, children.size() - 1);
        for (const OpcUaNodeInfo &child : children)
            parentNode->appendChild(new AddressSpaceNode(child, parentNode));
        endInsertRows();
    }
    parentNode->setBrowseStarted(true);
    parentNode->setBrowseComplete(true);
}

///
/// \brief AddressSpaceModel::setBrowseFailed
/// \param parentNodeId Parent NodeId.
///
void AddressSpaceModel::setBrowseFailed(const QString &parentNodeId)
{
    AddressSpaceNode *node = findNode(_root, parentNodeId);
    if (node)
        node->setBrowseStarted(false);
}

///
/// \brief AddressSpaceModel::setItems
/// \param items Test tree items.
///
void AddressSpaceModel::setItems(const QVector<AddressSpaceItem> &items)
{
    beginResetModel();
    delete _root;
    _root = new AddressSpaceNode({});
    _root->setBrowseComplete(true);
    appendTestItems(_root, items, QStringLiteral("test"));
    endResetModel();
}

///
/// \brief AddressSpaceModel::clear
///
void AddressSpaceModel::clear()
{
    beginResetModel();
    delete _root;
    _root = new AddressSpaceNode({});
    _root->setBrowseComplete(true);
    endResetModel();
}

///
/// \brief AddressSpaceModel::nodeInfo
/// \param index Model index.
/// \return Node information.
///
OpcUaNodeInfo AddressSpaceModel::nodeInfo(const QModelIndex &index) const
{
    return index.isValid() ? nodeForIndex(index)->info() : OpcUaNodeInfo();
}

///
/// \brief AddressSpaceModel::findByNodeId
/// \param nodeId NodeId to locate.
/// \return Matching model index.
///
QModelIndex AddressSpaceModel::findByNodeId(const QString &nodeId) const
{
    return indexForNode(findNode(_root, nodeId));
}

///
/// \brief AddressSpaceModel::findFirst
/// \param displayName Display name to locate.
/// \return Matching model index.
///
QModelIndex AddressSpaceModel::findFirst(const QString &displayName) const
{
    return findFirstRecursive(_root, displayName);
}

///
/// \brief AddressSpaceModel::setIconProvider
/// \param provider Icon provider callback.
///
void AddressSpaceModel::setIconProvider(
    std::function<QIcon(AddressSpaceItem::NodeType)> provider)
{
    _iconProvider = std::move(provider);
}

///
/// \brief AddressSpaceModel::nodeForIndex
/// \param index Model index.
/// \return Internal node or invisible root.
///
AddressSpaceNode *AddressSpaceModel::nodeForIndex(const QModelIndex &index) const
{
    return index.isValid()
        ? static_cast<AddressSpaceNode *>(index.internalPointer())
        : _root;
}

///
/// \brief AddressSpaceModel::findNode
/// \param node Search root.
/// \param nodeId NodeId to locate.
/// \return Matching node.
///
AddressSpaceNode *AddressSpaceModel::findNode(AddressSpaceNode *node,
                                              const QString &nodeId) const
{
    if (!node)
        return nullptr;
    if (node->info().nodeId == nodeId)
        return node;
    for (int row = 0; row < node->childCount(); ++row) {
        if (AddressSpaceNode *found = findNode(node->child(row), nodeId))
            return found;
    }
    return nullptr;
}

///
/// \brief AddressSpaceModel::indexForNode
/// \param node Internal node.
/// \return Matching model index.
///
QModelIndex AddressSpaceModel::indexForNode(AddressSpaceNode *node) const
{
    if (!node || node == _root)
        return {};
    return createIndex(node->row(), 0, node);
}

///
/// \brief AddressSpaceModel::findFirstRecursive
/// \param node Search root.
/// \param displayName Display name to locate.
/// \return Matching model index.
///
QModelIndex AddressSpaceModel::findFirstRecursive(AddressSpaceNode *node,
                                                  const QString &displayName) const
{
    for (int row = 0; row < node->childCount(); ++row) {
        AddressSpaceNode *child = node->child(row);
        if (child->info().displayName == displayName)
            return createIndex(row, 0, child);
        const QModelIndex found = findFirstRecursive(child, displayName);
        if (found.isValid())
            return found;
    }
    return {};
}

///
/// \brief AddressSpaceModel::appendTestItems
/// \param parent Parent node.
/// \param items Test items.
/// \param path Synthetic NodeId prefix.
///
void AddressSpaceModel::appendTestItems(AddressSpaceNode *parent,
                                        const QVector<AddressSpaceItem> &items,
                                        const QString &path)
{
    for (int row = 0; row < items.size(); ++row) {
        const AddressSpaceItem &item = items.at(row);
        OpcUaNodeInfo info;
        info.nodeId = QStringLiteral("%1/%2").arg(path).arg(row);
        info.browseName = item.displayName;
        info.displayName = item.displayName;
        switch (item.nodeType) {
        case AddressSpaceItem::NodeType::Folder: info.nodeClass = 1; break;
        case AddressSpaceItem::NodeType::Node: info.nodeClass = 1; break;
        case AddressSpaceItem::NodeType::Variable: info.nodeClass = 2; break;
        case AddressSpaceItem::NodeType::Method: info.nodeClass = 4; break;
        }
        info.hasChildren = !item.children.isEmpty();
        auto *child = new AddressSpaceNode(info, parent);
        child->setBrowseStarted(true);
        child->setBrowseComplete(true);
        parent->appendChild(child);
        appendTestItems(child, item.children, info.nodeId);
    }
}

///
/// \brief AddressSpaceModel::iconType
/// \param nodeClass OPC UA NodeClass numeric value.
/// \return Existing application icon type.
///
AddressSpaceItem::NodeType AddressSpaceModel::iconType(int nodeClass) const
{
    if (nodeClass & 2)
        return AddressSpaceItem::NodeType::Variable;
    if (nodeClass & 4)
        return AddressSpaceItem::NodeType::Method;
    if (nodeClass & 1)
        return AddressSpaceItem::NodeType::Folder;
    return AddressSpaceItem::NodeType::Node;
}
