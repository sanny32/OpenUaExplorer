// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file addressspacemodel.cpp
/// \brief Implements the OPC UA address space tree model.
///

#include "addressspacemodel.h"

// ── AddressSpaceNode ──────────────────────────────────────────────────────────

///
/// \brief AddressSpaceNode::AddressSpaceNode
/// \param displayName
/// \param nodeType
/// \param parent
///
AddressSpaceNode::AddressSpaceNode(const QString &displayName,
                                   AddressSpaceItem::NodeType nodeType,
                                   AddressSpaceNode *parent)
    : _displayName(displayName)
    , _nodeType(nodeType)
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
/// \param child
///
void AddressSpaceNode::appendChild(AddressSpaceNode *child)
{
    _children.append(child);
}

///
/// \brief AddressSpaceNode::child
/// \param row
/// \return
///
AddressSpaceNode *AddressSpaceNode::child(int row) const
{
    return _children.value(row, nullptr);
}

///
/// \brief AddressSpaceNode::childCount
/// \return
///
int AddressSpaceNode::childCount() const
{
    return _children.size();
}

///
/// \brief AddressSpaceNode::row
/// \return
///
int AddressSpaceNode::row() const
{
    if (_parent)
        return _parent->_children.indexOf(const_cast<AddressSpaceNode *>(this));
    return 0;
}

///
/// \brief AddressSpaceNode::parent
/// \return
///
AddressSpaceNode *AddressSpaceNode::parent() const
{
    return _parent;
}

///
/// \brief AddressSpaceNode::displayName
/// \return
///
QString AddressSpaceNode::displayName() const
{
    return _displayName;
}

///
/// \brief AddressSpaceNode::nodeType
/// \return
///
AddressSpaceItem::NodeType AddressSpaceNode::nodeType() const
{
    return _nodeType;
}

// ── AddressSpaceModel ─────────────────────────────────────────────────────────

///
/// \brief AddressSpaceModel::AddressSpaceModel
/// \param parent
///
AddressSpaceModel::AddressSpaceModel(QObject *parent)
    : QAbstractItemModel(parent)
    , _root(new AddressSpaceNode(QString(), AddressSpaceItem::NodeType::Folder))
{
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
/// \param row
/// \param column
/// \param parent
/// \return
///
QModelIndex AddressSpaceModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    AddressSpaceNode *parentNode = parent.isValid()
        ? static_cast<AddressSpaceNode *>(parent.internalPointer())
        : _root;

    AddressSpaceNode *child = parentNode->child(row);
    if (child)
        return createIndex(row, column, child);
    return QModelIndex();
}

///
/// \brief AddressSpaceModel::parent
/// \param index
/// \return
///
QModelIndex AddressSpaceModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    auto *child = static_cast<AddressSpaceNode *>(index.internalPointer());
    AddressSpaceNode *parentNode = child->parent();

    if (parentNode == _root)
        return QModelIndex();

    return createIndex(parentNode->row(), 0, parentNode);
}

///
/// \brief AddressSpaceModel::rowCount
/// \param parent
/// \return
///
int AddressSpaceModel::rowCount(const QModelIndex &parent) const
{
    AddressSpaceNode *parentNode = parent.isValid()
        ? static_cast<AddressSpaceNode *>(parent.internalPointer())
        : _root;
    return parentNode->childCount();
}

///
/// \brief AddressSpaceModel::columnCount
/// \param parent
/// \return
///
int AddressSpaceModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 1;
}

///
/// \brief AddressSpaceModel::data
/// \param index
/// \param role
/// \return
///
QVariant AddressSpaceModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    auto *node = static_cast<AddressSpaceNode *>(index.internalPointer());

    if (role == Qt::DisplayRole)
        return node->displayName();

    if (role == Qt::DecorationRole && _iconProvider)
        return _iconProvider(node->nodeType());

    return QVariant();
}

///
/// \brief AddressSpaceModel::setItems
/// \param items
///
void AddressSpaceModel::setItems(const QVector<AddressSpaceItem> &items)
{
    beginResetModel();
    delete _root;
    _root = new AddressSpaceNode(QString(), AddressSpaceItem::NodeType::Folder);
    buildNode(_root, items);
    endResetModel();
}

///
/// \brief AddressSpaceModel::clear
///
void AddressSpaceModel::clear()
{
    beginResetModel();
    delete _root;
    _root = new AddressSpaceNode(QString(), AddressSpaceItem::NodeType::Folder);
    endResetModel();
}

///
/// \brief AddressSpaceModel::findFirst
/// \param displayName
/// \return
///
QModelIndex AddressSpaceModel::findFirst(const QString &displayName) const
{
    return findFirstRecursive(_root, displayName);
}

///
/// \brief AddressSpaceModel::setIconProvider
/// \param provider
///
void AddressSpaceModel::setIconProvider(std::function<QIcon(AddressSpaceItem::NodeType)> provider)
{
    _iconProvider = std::move(provider);
}

///
/// \brief AddressSpaceModel::buildNode
/// \param parent
/// \param items
///
void AddressSpaceModel::buildNode(AddressSpaceNode *parent, const QVector<AddressSpaceItem> &items)
{
    for (const AddressSpaceItem &item : items) {
        auto *node = new AddressSpaceNode(item.displayName, item.nodeType, parent);
        parent->appendChild(node);
        if (!item.children.isEmpty())
            buildNode(node, item.children);
    }
}

///
/// \brief AddressSpaceModel::findFirstRecursive
/// \param node
/// \param displayName
/// \return
///
QModelIndex AddressSpaceModel::findFirstRecursive(AddressSpaceNode *node, const QString &displayName) const
{
    for (int i = 0; i < node->childCount(); ++i) {
        AddressSpaceNode *child = node->child(i);
        if (child->displayName() == displayName)
            return createIndex(i, 0, child);
        QModelIndex found = findFirstRecursive(child, displayName);
        if (found.isValid())
            return found;
    }
    return QModelIndex();
}
