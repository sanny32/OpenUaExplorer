// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file attributesmodel.cpp
/// \brief Implements the selected node attributes tree model.
///

#include <QBrush>
#include <QColor>

#include "attributesmodel.h"

///
/// \brief One node in the attributes tree.
///
struct AttributesModel::Item
{
    QString attribute;
    QString value;
    Item *parent = nullptr;
    std::vector<std::unique_ptr<Item>> children;
};

///
/// \brief AttributesModel::AttributesModel
/// \param parent Parent object.
///
AttributesModel::AttributesModel(QObject *parent)
    : QAbstractItemModel(parent)
    , _root(std::make_unique<Item>())
{
}

///
/// \brief AttributesModel::~AttributesModel
///
AttributesModel::~AttributesModel() = default;

///
/// \brief AttributesModel::index
/// \param row Child row.
/// \param column Model column.
/// \param parent Parent index.
/// \return Model index for the requested item.
///
QModelIndex AttributesModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return {};
    Item *parentItem = itemForIndex(parent);
    if (!parentItem || row >= static_cast<int>(parentItem->children.size()))
        return {};
    return createIndex(row, column, parentItem->children.at(row).get());
}

///
/// \brief AttributesModel::parent
/// \param child Child index.
/// \return Parent index or an invalid index for top-level items.
///
QModelIndex AttributesModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return {};
    auto *childItem = static_cast<Item *>(child.internalPointer());
    Item *parentItem = childItem ? childItem->parent : nullptr;
    if (!parentItem || parentItem == _root.get())
        return {};
    Item *grandParent = parentItem->parent;
    if (!grandParent)
        return {};
    for (int row = 0; row < static_cast<int>(grandParent->children.size()); ++row) {
        if (grandParent->children.at(row).get() == parentItem)
            return createIndex(row, 0, parentItem);
    }
    return {};
}

///
/// \brief AttributesModel::rowCount
/// \param parent Parent index.
/// \return Number of child rows.
///
int AttributesModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;
    const Item *item = itemForIndex(parent);
    return item ? static_cast<int>(item->children.size()) : 0;
}

///
/// \brief AttributesModel::columnCount
/// \param parent Parent index.
/// \return Number of columns.
///
int AttributesModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return ColCount;
}

///
/// \brief AttributesModel::headerData
/// \param section Header section.
/// \param orientation Header orientation.
/// \param role Data role.
/// \return Header value.
///
QVariant AttributesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QAbstractItemModel::headerData(section, orientation, role);

    switch (section) {
    case ColAttribute: return QStringLiteral("Attribute");
    case ColValue: return QStringLiteral("Value");
    default: return {};
    }
}

///
/// \brief AttributesModel::data
/// \param index Model index.
/// \param role Data role.
/// \return Item data.
///
QVariant AttributesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};
    const auto *item = static_cast<Item *>(index.internalPointer());
    if (!item)
        return {};

    if (role == Qt::DisplayRole)
        return index.column() == ColAttribute ? item->attribute : item->value;

    if (role == Qt::TextAlignmentRole)
        return QVariant(_columnAlignments.alignment(index.column()));

    if (role == Qt::ForegroundRole && index.column() == ColValue) {
        if (item->value.startsWith(QLatin1String("Bad")))
            return QBrush(QColor(210, 70, 70));
        if (item->value.startsWith(QLatin1String("Good")))
            return QBrush(QColor(0, 150, 64));
        if (item->value.isEmpty())
            return QBrush(QColor(128, 128, 128));
    }

    return {};
}

///
/// \brief AttributesModel::setAttributes
/// \param attributes Structured OPC UA attributes.
///
void AttributesModel::setAttributes(const QVector<OpcUaNodeAttribute> &attributes)
{
    beginResetModel();
    _root = std::make_unique<Item>();
    for (const OpcUaNodeAttribute &attribute : attributes)
        appendAttribute(_root.get(), attribute);
    endResetModel();
}

///
/// \brief AttributesModel::setItems
/// \param items Flat attribute rows used by UI sample data.
///
void AttributesModel::setItems(const QVector<QPair<QString, QString>> &items)
{
    QVector<OpcUaNodeAttribute> attributes;
    attributes.reserve(items.size());
    for (const auto &item : items) {
        OpcUaNodeAttribute attribute;
        attribute.name = item.first;
        attribute.displayValue = item.second;
        attributes.append(attribute);
    }
    setAttributes(attributes);
}

///
/// \brief AttributesModel::clear
///
void AttributesModel::clear()
{
    setAttributes({});
}

///
/// \brief AttributesModel::setColumnAlignment
/// \param column Model column.
/// \param alignment Text alignment.
///
void AttributesModel::setColumnAlignment(int column, Qt::Alignment alignment)
{
    _columnAlignments.setAlignment(column, alignment);
    if (rowCount() > 0) {
        emit dataChanged(index(0, column), index(rowCount() - 1, column),
                         {Qt::TextAlignmentRole});
    }
}

///
/// \brief AttributesModel::appendAttribute
/// \param parent Parent tree item.
/// \param attribute Attribute to append.
///
void AttributesModel::appendAttribute(Item *parent, const OpcUaNodeAttribute &attribute)
{
    auto item = std::make_unique<Item>();
    item->attribute = attribute.name;
    item->value = attribute.displayValue;
    item->parent = parent;
    Item *itemPointer = item.get();
    parent->children.push_back(std::move(item));
    for (const OpcUaNodeAttribute &child : attribute.children)
        appendAttribute(itemPointer, child);
}

///
/// \brief AttributesModel::itemForIndex
/// \param index Model index.
/// \return Associated tree item or the root item.
///
AttributesModel::Item *AttributesModel::itemForIndex(const QModelIndex &index) const
{
    return index.isValid() ? static_cast<Item *>(index.internalPointer()) : _root.get();
}
