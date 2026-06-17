// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file nodeinfomodel.cpp
/// \brief Implements the selected node information model.
///

#include "nodeinfomodel.h"

///
/// \brief Constructs an empty node-info model.
/// \param parent Owning QObject.
///
NodeInfoModel::NodeInfoModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

///
/// \brief Returns the number of info rows.
/// \param parent Parent index; non-root parents have no rows.
/// \return Item count, or 0 for non-root parents.
///
int NodeInfoModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return _items.size();
}

///
/// \brief Returns the fixed column count.
/// \param parent Parent index; non-root parents have no columns.
/// \return Column count, or 0 for non-root parents.
///
int NodeInfoModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return ColCount;
}

///
/// \brief Returns the label or value text for a cell.
/// \param index Cell to query.
/// \param role Only Qt::DisplayRole is handled.
/// \return Cell text, or an invalid variant.
///
QVariant NodeInfoModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    if (index.row() < 0 || index.row() >= _items.size()) return QVariant();

    if (role != Qt::DisplayRole) return QVariant();

    const NodeInfoItem &item = _items.at(index.row());
    switch (index.column()) {
    case ColLabel: return item.label;
    case ColValue: return item.value;
    default:       return QVariant();
    }
}

///
/// \brief Replaces the info rows.
/// \param items New label/value rows to display.
///
void NodeInfoModel::setItems(const QVector<NodeInfoItem> &items)
{
    beginResetModel();
    _items = items;
    endResetModel();
}

///
/// \brief Removes all rows.
///
void NodeInfoModel::clear()
{
    beginResetModel();
    _items.clear();
    endResetModel();
}
