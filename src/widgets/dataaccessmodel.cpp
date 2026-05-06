// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dataaccessmodel.cpp
/// \brief Implements the OPC UA data access table model.
///

#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QPalette>

#include "dataaccessmodel.h"

///
/// \brief DataAccessModel::DataAccessModel
/// \param parent
///
DataAccessModel::DataAccessModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

///
/// \brief DataAccessModel::setItems
/// \param items
///
void DataAccessModel::setItems(const QVector<DataAccessItem> &items)
{
    beginResetModel();
    _items = items;
    endResetModel();
}


///
/// \brief DataAccessModel::rowCount
/// \param parent
/// \return
///
int DataAccessModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return _items.size();
}

///
/// \brief DataAccessModel::columnCount
/// \param parent
/// \return
///
int DataAccessModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return ColCount;
}

///
/// \brief DataAccessModel::headerData
/// \param section
/// \param orientation
/// \param role
/// \return
///
QVariant DataAccessModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QAbstractTableModel::headerData(section, orientation, role);

    switch (section) {
    case ColNumber:       return QStringLiteral("#");
    case ColNodeId:       return QStringLiteral("Node Id");
    case ColDisplayName:  return QStringLiteral("Display Name");
    case ColValue:        return QStringLiteral("Value");
    case ColDataType:     return QStringLiteral("Data Type");
    case ColTimestamp:    return QStringLiteral("Source Timestamp");
    case ColStatus:       return QStringLiteral("Status");
    case ColSubscription: return QStringLiteral("Subscription");
    default:              return QVariant();
    }
}

///
/// \brief DataAccessModel::flags
/// \param index
/// \return
///
Qt::ItemFlags DataAccessModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags f = QAbstractTableModel::flags(index);
    if (index.column() == ColSubscription)
        f |= Qt::ItemIsEditable;
    return f;
}

///
/// \brief DataAccessModel::setData
/// \param index
/// \param value
/// \param role
/// \return
///
bool DataAccessModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole || !index.isValid()) return false;
    if (index.column() != ColSubscription) return false;
    if (index.row() < 0 || index.row() >= _items.size()) return false;

    _items[index.row()].subscriptionName = value.toString();
    emit dataChanged(index, index, {Qt::DisplayRole, Qt::ForegroundRole});
    return true;
}

///
/// \brief DataAccessModel::data
/// \param index
/// \param role
/// \return
///
QVariant DataAccessModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    if (index.row() < 0 || index.row() >= _items.size()) return QVariant();

    const DataAccessItem &item = _items.at(index.row());
    const int col = index.column();

    if (role == Qt::DisplayRole) {
        switch (col) {
        case ColNumber:       return index.row() + 1;
        case ColNodeId:       return item.nodeId;
        case ColDisplayName:  return item.displayName;
        case ColValue:        return item.value;
        case ColDataType:     return item.dataType;
        case ColTimestamp:    return item.sourceTimestamp;
        case ColStatus:       return item.status;
        case ColSubscription: return item.subscriptionName.isEmpty()
                                     ? QStringLiteral("—")
                                     : item.subscriptionName;
        default:              return QVariant();
        }
    }

    if (role == Qt::TextAlignmentRole)
        return QVariant(columnAlignment(col));

    if (role == Qt::EditRole && col == ColSubscription)
        return item.subscriptionName;

    if (role == Qt::ForegroundRole) {
        if (item.subscriptionName.isEmpty()) {
            return QBrush(qApp->palette().color(QPalette::Disabled, QPalette::Text));
        }
        if (col == ColValue)
            return QBrush(QColor(0, 150, 64));
        if (col == ColStatus && item.status == QLatin1String("Good"))
            return QBrush(QColor(0, 150, 64));
        if (col == ColSubscription)
            return QBrush(QColor(0, 120, 200));
    }

    return QVariant();
}

///
/// \brief DataAccessModel::columnAlignment
/// \param column
/// \return
///
Qt::Alignment DataAccessModel::columnAlignment(int column) const
{
    return _columnAlignments.value(column, Qt::AlignLeft | Qt::AlignVCenter);
}

///
/// \brief DataAccessModel::setColumnAlignment
/// \param column
/// \param alignment
///
void DataAccessModel::setColumnAlignment(int column, Qt::Alignment alignment)
{
    _columnAlignments[column] = alignment;
    emit dataChanged(index(0, column), index(rowCount() - 1, column), {Qt::TextAlignmentRole});
}
