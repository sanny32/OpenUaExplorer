// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file subscriptionsmodel.cpp
/// \brief Implements the subscriptions table model.
///

#include "subscriptionsmodel.h"

///
/// \brief SubscriptionsModel::SubscriptionsModel
/// \param parent
///
SubscriptionsModel::SubscriptionsModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

///
/// \brief SubscriptionsModel::rowCount
/// \param parent
/// \return
///
int SubscriptionsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return _items.size();
}

///
/// \brief SubscriptionsModel::columnCount
/// \param parent
/// \return
///
int SubscriptionsModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return ColCount;
}

///
/// \brief SubscriptionsModel::headerData
/// \param section
/// \param orientation
/// \param role
/// \return
///
QVariant SubscriptionsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QAbstractTableModel::headerData(section, orientation, role);

    switch (section) {
    case ColName:               return QStringLiteral("Name");
    case ColPublishingInterval: return QStringLiteral("Publishing Interval");
    default:                    return QVariant();
    }
}

///
/// \brief SubscriptionsModel::data
/// \param index
/// \param role
/// \return
///
QVariant SubscriptionsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    if (index.row() < 0 || index.row() >= _items.size()) return QVariant();
    const SubscriptionItem &item = _items.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ColName:               return item.name;
        case ColPublishingInterval: return item.publishingInterval;
        default:                    return QVariant();
        }
    }

    if (role == Qt::TextAlignmentRole)
        return QVariant(_columnAlignments.value(index.column(), Qt::AlignLeft | Qt::AlignVCenter));

    return QVariant();
}

///
/// \brief SubscriptionsModel::setItems
/// \param items
///
void SubscriptionsModel::setItems(const QVector<SubscriptionItem> &items)
{
    beginResetModel();
    _items = items;
    endResetModel();
}

///
/// \brief SubscriptionsModel::clear
///
void SubscriptionsModel::clear()
{
    beginResetModel();
    _items.clear();
    endResetModel();
}

///
/// \brief SubscriptionsModel::setColumnAlignment
/// \param column
/// \param alignment
///
void SubscriptionsModel::setColumnAlignment(int column, Qt::Alignment alignment)
{
    _columnAlignments[column] = alignment;
    emit dataChanged(index(0, column), index(rowCount() - 1, column), {Qt::TextAlignmentRole});
}
