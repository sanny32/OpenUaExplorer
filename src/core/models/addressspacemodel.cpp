// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file addressspacemodel.cpp
/// \brief Implements the lazy OPC UA address space tree model.
///

#include "addressspacemodel.h"

#include <QMimeData>
#include <QSet>
#include <QStringList>

#include "addressspacemimedata.h"
#include "addressspacenode.h"

namespace {

///
/// \brief Returns the user-facing name for an address-space node.
/// \param node Node metadata.
/// \return DisplayName, BrowseName, or NodeId.
///
QString nodeLabel(const OpcUaNodeInfo &node)
{
    if (!node.displayName.isEmpty())
        return node.displayName;
    if (!node.browseName.isEmpty())
        return node.browseName;
    return node.nodeId;
}

///
/// \brief Builds a display path by appending a node label to its parent's path.
/// \param parent Parent tree node.
/// \param node Child node metadata.
/// \return Slash-separated display path.
///
QString displayPath(AddressSpaceNode *parent, const OpcUaNodeInfo &node)
{
    const QString label = nodeLabel(node);
    if (label.isEmpty())
        return {};

    QStringList parts;
    if (parent && !parent->info().displayPath.isEmpty())
        parts.append(parent->info().displayPath);
    parts.append(label);
    return parts.join(QLatin1Char('/'));
}

///
/// \brief Returns browse results with repeated NodeIds removed.
/// \param children Browse results.
/// \return Deduplicated browse results.
///
QVector<OpcUaNodeInfo> uniqueChildrenByNodeId(const QVector<OpcUaNodeInfo> &children)
{
    QVector<OpcUaNodeInfo> uniqueChildren;
    uniqueChildren.reserve(children.size());

    QSet<QString> seenNodeIds;
    for (const OpcUaNodeInfo &child : children) {
        if (!child.nodeId.isEmpty()) {
            if (seenNodeIds.contains(child.nodeId))
                continue;
            seenNodeIds.insert(child.nodeId);
        }
        uniqueChildren.append(child);
    }

    return uniqueChildren;
}

} // namespace

///
/// \brief Constructs a tree node wrapping OPC UA node info.
/// \param info OPC UA node information.
/// \param parent Parent tree node.
///
AddressSpaceNode::AddressSpaceNode(const OpcUaNodeInfo &info, AddressSpaceNode *parent)
    : _info(info)
    , _parent(parent)
{
    if (_info.displayPath.isEmpty())
        _info.displayPath = displayPath(parent, _info);
}

///
/// \brief Destroys the node and its children.
///
AddressSpaceNode::~AddressSpaceNode() = default;

///
/// \brief Appends a child node.
/// \param child Child node.
///
void AddressSpaceNode::appendChild(std::unique_ptr<AddressSpaceNode> child)
{
    _children.push_back(std::move(child));
}

///
/// \brief Removes all child nodes.
///
void AddressSpaceNode::clearChildren()
{
    _children.clear();
}

///
/// \brief Returns the child at a row.
/// \param row Child row.
/// \return Child node.
///
AddressSpaceNode *AddressSpaceNode::child(int row) const
{
    return row >= 0 && row < childCount()
        ? _children.at(static_cast<std::size_t>(row)).get()
        : nullptr;
}

///
/// \brief Returns the number of loaded children.
/// \return Number of loaded children.
///
int AddressSpaceNode::childCount() const
{
    return static_cast<int>(_children.size());
}

///
/// \brief Returns this node's index within its parent.
/// \return Row in the parent.
///
int AddressSpaceNode::row() const
{
    if (!_parent)
        return 0;
    for (int index = 0; index < _parent->childCount(); ++index) {
        if (_parent->child(index) == this)
            return index;
    }
    return -1;
}

///
/// \brief Returns the parent node.
/// \return Parent node.
///
AddressSpaceNode *AddressSpaceNode::parent() const
{
    return _parent;
}

///
/// \brief Returns the wrapped OPC UA node information.
/// \return OPC UA node information.
///
const OpcUaNodeInfo &AddressSpaceNode::info() const
{
    return _info;
}

///
/// \brief Reports whether a browse request was issued for this node.
/// \return True after a browse request was issued.
///
bool AddressSpaceNode::browseStarted() const
{
    return _browseStarted;
}

///
/// \brief Reports whether browse results were received for this node.
/// \return True after browse results were received.
///
bool AddressSpaceNode::browseComplete() const
{
    return _browseComplete;
}

///
/// \brief Sets whether a browse request was issued.
/// \param value New state.
///
void AddressSpaceNode::setBrowseStarted(bool value)
{
    _browseStarted = value;
}

///
/// \brief Sets whether browse results were received.
/// \param value New state.
///
void AddressSpaceNode::setBrowseComplete(bool value)
{
    _browseComplete = value;
}

///
/// \brief Constructs the model with an empty invisible root.
/// \param parent Parent object.
///
AddressSpaceModel::AddressSpaceModel(QObject *parent)
    : QAbstractItemModel(parent)
    , _root(std::make_unique<AddressSpaceNode>(OpcUaNodeInfo{}))
{
    _root->setBrowseComplete(true);
}

///
/// \brief Destroys the model and its node tree.
///
AddressSpaceModel::~AddressSpaceModel() = default;

///
/// \brief Returns the index for a child cell.
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
/// \brief Returns the parent index of an item.
/// \param index Child index.
/// \return Parent index.
///
QModelIndex AddressSpaceModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return {};
    AddressSpaceNode *parentNode = nodeForIndex(index)->parent();
    if (!parentNode || parentNode == _root.get())
        return {};
    return createIndex(parentNode->row(), 0, parentNode);
}

///
/// \brief Returns the number of loaded children.
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
/// \brief Returns the single-column count.
/// \param parent Parent index.
/// \return Column count.
///
int AddressSpaceModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 1;
}

///
/// \brief Returns display name, tooltip, NodeId, or icon for an item.
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
/// \brief Returns item flags, including drag support for variable nodes.
/// \param index Model index.
/// \return Item flags.
///
Qt::ItemFlags AddressSpaceModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags itemFlags = QAbstractItemModel::flags(index);
    if (!index.isValid())
        return itemFlags;

    const OpcUaNodeInfo &info = nodeForIndex(index)->info();
    if (!info.nodeId.isEmpty())
        itemFlags |= Qt::ItemIsDragEnabled;
    return itemFlags;
}

///
/// \brief Reports whether a node has, or may yet have, children.
/// \param parent Parent index.
/// \return True for loaded children or nodes that may be browsed.
///
bool AddressSpaceModel::hasChildren(const QModelIndex &parent) const
{
    AddressSpaceNode *node = nodeForIndex(parent);
    return node->childCount() > 0 || (!node->browseComplete() && node->info().hasChildren);
}

///
/// \brief Reports whether a node still needs browsing.
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
/// \brief Requests a browse for a node that needs loading.
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
/// \brief Returns the MIME formats exported by dragged nodes.
/// \return Supported MIME formats.
///
QStringList AddressSpaceModel::mimeTypes() const
{
    return {AddressSpaceMime::nodeMimeType()};
}

///
/// \brief Encodes the dragged node.
/// \param indexes Dragged model indexes.
/// \return MIME data owned by the caller.
///
QMimeData *AddressSpaceModel::mimeData(const QModelIndexList &indexes) const
{
    for (const QModelIndex &index : indexes) {
        if (!index.isValid() || index.column() != 0)
            continue;
        const OpcUaNodeInfo info = nodeForIndex(index)->info();
        if (!info.nodeId.isEmpty())
            return AddressSpaceMime::createNodeMimeData(info);
    }
    return new QMimeData;
}

///
/// \brief Returns the supported drag action.
/// \return Copy action.
///
Qt::DropActions AddressSpaceModel::supportedDragActions() const
{
    return Qt::CopyAction;
}

///
/// \brief Sets the single visible root node.
/// \param root Visible root node.
///
void AddressSpaceModel::setRootNode(const OpcUaNodeInfo &root)
{
    beginResetModel();
    _root = std::make_unique<AddressSpaceNode>(OpcUaNodeInfo{});
    _root->setBrowseComplete(true);
    _root->appendChild(std::make_unique<AddressSpaceNode>(root, _root.get()));
    endResetModel();
}

///
/// \brief Applies browse results under a parent node.
/// \param parentNodeId Parent NodeId.
/// \param children Browse results.
///
void AddressSpaceModel::setChildren(const QString &parentNodeId,
                                    const QVector<OpcUaNodeInfo> &children)
{
    AddressSpaceNode *parentNode = findNode(_root.get(), parentNodeId);
    if (!parentNode)
        return;
    const QVector<OpcUaNodeInfo> uniqueChildren = uniqueChildrenByNodeId(children);
    const QModelIndex parentIndex = indexForNode(parentNode);
    if (parentNode->childCount() > 0) {
        beginRemoveRows(parentIndex, 0, parentNode->childCount() - 1);
        parentNode->clearChildren();
        endRemoveRows();
    }
    if (!uniqueChildren.isEmpty()) {
        beginInsertRows(parentIndex, 0, uniqueChildren.size() - 1);
        for (const OpcUaNodeInfo &child : uniqueChildren)
            parentNode->appendChild(
                std::make_unique<AddressSpaceNode>(child, parentNode));
        endInsertRows();
    }
    parentNode->setBrowseStarted(true);
    parentNode->setBrowseComplete(true);
}

///
/// \brief Resets the browse state so a failed node can be retried.
/// \param parentNodeId Parent NodeId.
///
void AddressSpaceModel::setBrowseFailed(const QString &parentNodeId)
{
    AddressSpaceNode *node = findNode(_root.get(), parentNodeId);
    if (node)
        node->setBrowseStarted(false);
}

///
/// \brief Builds a synthetic tree from test items.
/// \param items Test tree items.
///
void AddressSpaceModel::setItems(const QVector<AddressSpaceItem> &items)
{
    beginResetModel();
    _root = std::make_unique<AddressSpaceNode>(OpcUaNodeInfo{});
    _root->setBrowseComplete(true);
    appendTestItems(_root.get(), items, QStringLiteral("test"));
    endResetModel();
}

///
/// \brief Empties the tree.
///
void AddressSpaceModel::clear()
{
    beginResetModel();
    _root = std::make_unique<AddressSpaceNode>(OpcUaNodeInfo{});
    _root->setBrowseComplete(true);
    endResetModel();
}

///
/// \brief Returns the node information for an index.
/// \param index Model index.
/// \return Node information.
///
OpcUaNodeInfo AddressSpaceModel::nodeInfo(const QModelIndex &index) const
{
    return index.isValid() ? nodeForIndex(index)->info() : OpcUaNodeInfo();
}

///
/// \brief Finds the index of a node by NodeId.
/// \param nodeId NodeId to locate.
/// \return Matching model index.
///
QModelIndex AddressSpaceModel::findByNodeId(const QString &nodeId) const
{
    return indexForNode(findNode(_root.get(), nodeId));
}

///
/// \brief Finds the first index with a display name.
/// \param displayName Display name to locate.
/// \return Matching model index.
///
QModelIndex AddressSpaceModel::findFirst(const QString &displayName) const
{
    return findFirstRecursive(_root.get(), displayName);
}

///
/// \brief Sets the callback that supplies node-class icons.
/// \param provider Icon provider callback.
///
void AddressSpaceModel::setIconProvider(
    std::function<QIcon(AddressSpaceItem::NodeType)> provider)
{
    _iconProvider = std::move(provider);
}

///
/// \brief Returns the internal node for an index, or the invisible root.
/// \param index Model index.
/// \return Internal node or invisible root.
///
AddressSpaceNode *AddressSpaceModel::nodeForIndex(const QModelIndex &index) const
{
    return index.isValid()
        ? static_cast<AddressSpaceNode *>(index.internalPointer())
        : _root.get();
}

///
/// \brief Recursively searches a subtree for a node by NodeId.
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
/// \brief Returns the model index for an internal node.
/// \param node Internal node.
/// \return Matching model index.
///
QModelIndex AddressSpaceModel::indexForNode(AddressSpaceNode *node) const
{
    if (!node || node == _root.get())
        return {};
    return createIndex(node->row(), 0, node);
}

///
/// \brief Recursively searches for the first node with a display name.
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
/// \brief Recursively builds synthetic nodes from test items.
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
        auto child = std::make_unique<AddressSpaceNode>(info, parent);
        AddressSpaceNode *childNode = child.get();
        childNode->setBrowseStarted(true);
        childNode->setBrowseComplete(true);
        parent->appendChild(std::move(child));
        appendTestItems(childNode, item.children, info.nodeId);
    }
}

///
/// \brief Maps an OPC UA node class to an icon node type.
/// \param nodeClass OPC UA NodeClass numeric value.
/// \return Existing application icon type.
///
AddressSpaceItem::NodeType AddressSpaceModel::iconType(int nodeClass) const
{
    if (nodeClass & OpcUa::Variable)
        return AddressSpaceItem::NodeType::Variable;
    if (nodeClass & OpcUa::Method)
        return AddressSpaceItem::NodeType::Method;
    if (nodeClass & OpcUa::Object)
        return AddressSpaceItem::NodeType::Folder;
    return AddressSpaceItem::NodeType::Node;
}
