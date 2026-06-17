// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file subscriptionsmodel.cpp
/// \brief Implements the subscriptions table model.
///

#include "subscriptionsmodel.h"

///
/// \brief Constructs an empty subscriptions model.
/// \param parent Owning QObject.
///
SubscriptionsModel::SubscriptionsModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

///
/// \brief Returns the number of subscription rows.
/// \param parent Parent index; non-root parents have no rows.
/// \return Subscription count, or 0 for non-root parents.
///
int SubscriptionsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return _items.size();
}

///
/// \brief Returns the fixed column count.
/// \param parent Parent index; non-root parents have no columns.
/// \return Column count, or 0 for non-root parents.
///
int SubscriptionsModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return ColCount;
}

///
/// \brief Returns the Name/Publishing Interval column titles.
/// \param section Column index.
/// \param orientation Header orientation.
/// \param role Display role.
/// \return Column title, or the base implementation otherwise.
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
/// \brief Returns the name/interval text and column alignment for a subscription row.
/// \param index Cell to query.
/// \param role Requested data role.
/// \return Value for the role, or an invalid variant.
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
        return QVariant(_columnAlignments.alignment(index.column()));

    return QVariant();
}

///
/// \brief Replaces all subscription rows with row-change notifications.
/// \param items New subscriptions to display.
///
void SubscriptionsModel::setItems(const QVector<SubscriptionItem> &items)
{
    if (!_items.isEmpty()) {
        beginRemoveRows(QModelIndex(), 0, _items.size() - 1);
        _items.clear();
        endRemoveRows();
    }
    if (!items.isEmpty()) {
        beginInsertRows(QModelIndex(), 0, items.size() - 1);
        _items = items;
        endInsertRows();
    }
}

///
/// \brief Returns the names of all subscriptions.
/// \return Subscription names in row order.
///
QStringList SubscriptionsModel::names() const
{
    QStringList result;
    result.reserve(_items.size());
    for (const SubscriptionItem &item : _items)
        result.append(item.name);
    return result;
}

///
/// \brief Removes all subscriptions.
///
void SubscriptionsModel::clear()
{
    if (_items.isEmpty()) return;
    beginRemoveRows(QModelIndex(), 0, _items.size() - 1);
    _items.clear();
    endRemoveRows();
}

///
/// \brief Sets the text alignment for a column.
/// \param column Column index.
/// \param alignment Alignment to apply.
///
void SubscriptionsModel::setColumnAlignment(int column, Qt::Alignment alignment)
{
    _columnAlignments.setAlignment(column, alignment);
    emit dataChanged(index(0, column), index(rowCount() - 1, column), {Qt::TextAlignmentRole});
}
