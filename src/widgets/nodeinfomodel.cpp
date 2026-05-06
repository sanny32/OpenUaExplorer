// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file nodeinfomodel.cpp
/// \brief Implements the selected node information model.
///

#include "nodeinfomodel.h"

///
/// \brief NodeInfoModel::NodeInfoModel
/// \param parent
///
NodeInfoModel::NodeInfoModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

///
/// \brief NodeInfoModel::rowCount
/// \param parent
/// \return
///
int NodeInfoModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return _items.size();
}

///
/// \brief NodeInfoModel::columnCount
/// \param parent
/// \return
///
int NodeInfoModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return ColCount;
}

///
/// \brief NodeInfoModel::data
/// \param index
/// \param role
/// \return
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
/// \brief NodeInfoModel::setItems
/// \param items
///
void NodeInfoModel::setItems(const QVector<NodeInfoItem> &items)
{
    beginResetModel();
    _items = items;
    endResetModel();
}

///
/// \brief NodeInfoModel::clear
///
void NodeInfoModel::clear()
{
    beginResetModel();
    _items.clear();
    endResetModel();
}
