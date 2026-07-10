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
        case ColPublishingInterval: return QStringLiteral("%1 ms").arg(item.publishingInterval);
        default:                    return QVariant();
        }
    }

    if (role == Qt::EditRole) {
        switch (index.column()) {
        case ColName:               return item.name;
        case ColPublishingInterval: return item.publishingInterval;
        default:                    return QVariant();
        }
    }

    if (role == Qt::DecorationRole && index.column() == ColName
        && item.isBuiltin() && !_builtinIcon.isNull()) {
        return _builtinIcon;
    }

    if (role == Qt::BackgroundRole && item.isBuiltin()
        && _builtinBackground.style() != Qt::NoBrush) {
        return _builtinBackground;
    }

    if (role == Qt::TextAlignmentRole)
        return QVariant(_columnAlignments.alignment(index.column()));

    return QVariant();
}

///
/// \brief Reports that the Name and Publishing Interval cells are editable.
/// \param index Cell to query.
/// \return Item flags including Qt::ItemIsEditable for non-built-in rows.
///
Qt::ItemFlags SubscriptionsModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QAbstractTableModel::flags(index);
    if (index.isValid()
        && (index.column() == ColName || index.column() == ColPublishingInterval)
        && !itemAt(index.row()).isBuiltin()) {
        flags |= Qt::ItemIsEditable;
    }
    return flags;
}

///
/// \brief Stores an edited subscription name or publishing interval.
/// \param index Cell being edited.
/// \param value New value.
/// \param role Edit role; other roles are ignored.
/// \return True when the value was accepted and stored.
///
bool SubscriptionsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole || !index.isValid())
        return false;
    if (index.row() < 0 || index.row() >= _items.size())
        return false;
    SubscriptionItem &item = _items[index.row()];
    if (item.isBuiltin())
        return false;

    if (index.column() == ColName) {
        const QString newName = value.toString().trimmed();
        if (newName.isEmpty() || containsName(newName, index.row()))
            return false;
        const QString oldName = item.name;
        if (newName == oldName)
            return true;
        item.name = newName;
        emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
        emit subscriptionRenamed(oldName, newName);
        return true;
    }

    if (index.column() == ColPublishingInterval) {
        bool ok = false;
        const double interval = value.toDouble(&ok);
        if (!ok || interval <= 0.0)
            return false;
        if (qFuzzyCompare(interval, item.publishingInterval))
            return true;
        item.publishingInterval = interval;
        emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
        emit subscriptionIntervalChanged(item.name, interval);
        return true;
    }

    return false;
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
/// \brief Appends a subscription row with row-change notifications.
/// \param item Subscription to add.
/// \return Row index of the inserted subscription.
///
int SubscriptionsModel::addSubscription(const SubscriptionItem &item)
{
    const int row = _items.size();
    beginInsertRows(QModelIndex(), row, row);
    _items.append(item);
    endInsertRows();
    return row;
}

///
/// \brief Removes a single subscription row.
/// \param row Row to remove.
///
void SubscriptionsModel::removeRow(int row)
{
    if (row < 0 || row >= _items.size())
        return;
    beginRemoveRows(QModelIndex(), row, row);
    _items.removeAt(row);
    endRemoveRows();
}

///
/// \brief Returns the subscription at a row.
/// \param row Row to read.
/// \return Subscription, or a default-constructed item for invalid rows.
///
SubscriptionItem SubscriptionsModel::itemAt(int row) const
{
    if (row < 0 || row >= _items.size())
        return SubscriptionItem();
    return _items.at(row);
}

///
/// \brief Reports whether a subscription name already exists.
/// \param name Name to look for.
/// \param exceptRow Row to ignore in the search, or -1 to check all rows.
/// \return True when another row carries the name.
///
bool SubscriptionsModel::containsName(const QString &name, int exceptRow) const
{
    for (int row = 0; row < _items.size(); ++row) {
        if (row == exceptRow)
            continue;
        if (_items.at(row).name == name)
            return true;
    }
    return false;
}

///
/// \brief Returns the publishing interval of a named subscription.
/// \param name Subscription name.
/// \return Publishing interval in milliseconds, or 1000 when not found.
///
double SubscriptionsModel::intervalFor(const QString &name) const
{
    for (const SubscriptionItem &item : _items) {
        if (item.name == name)
            return item.publishingInterval;
    }
    return 1000.0;
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

///
/// \brief Sets the decoration icon shown next to built-in subscription names.
/// \param icon Icon to display, typically a lock glyph.
///
void SubscriptionsModel::setBuiltinIcon(const QIcon &icon)
{
    _builtinIcon = icon;
    if (_items.isEmpty())
        return;
    emit dataChanged(index(0, ColName), index(rowCount() - 1, ColName), {Qt::DecorationRole});
}

///
/// \brief Sets the row background brush used for built-in subscriptions.
/// \param brush Background brush distinguishing built-in rows.
///
void SubscriptionsModel::setBuiltinBackground(const QBrush &brush)
{
    _builtinBackground = brush;
    if (_items.isEmpty())
        return;
    emit dataChanged(index(0, 0), index(rowCount() - 1, ColCount - 1), {Qt::BackgroundRole});
}
