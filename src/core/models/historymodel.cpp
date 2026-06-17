// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file historymodel.cpp
/// \brief Implements the OPC UA history table model.
///

#include "historymodel.h"

///
/// \brief Constructs an empty history model.
/// \param parent Owning QObject.
///
HistoryModel::HistoryModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

///
/// \brief Returns the number of history rows.
/// \param parent Parent index; non-root parents have no rows.
/// \return Item count, or 0 for non-root parents.
///
int HistoryModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return _items.size();
}

///
/// \brief Returns the fixed column count.
/// \param parent Parent index; non-root parents have no columns.
/// \return Column count, or 0 for non-root parents.
///
int HistoryModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return ColCount;
}

///
/// \brief Returns the Node/Range column titles.
/// \param section Column index.
/// \param orientation Header orientation.
/// \param role Display role.
/// \return Column title, or the base implementation otherwise.
///
QVariant HistoryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QAbstractTableModel::headerData(section, orientation, role);

    switch (section) {
    case ColNode:  return QStringLiteral("Node");
    case ColRange: return QStringLiteral("Range");
    default:       return QVariant();
    }
}

///
/// \brief Returns the node/range text and column alignment for a history row.
/// \param index Cell to query.
/// \param role Requested data role.
/// \return Value for the role, or an invalid variant.
///
QVariant HistoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    if (index.row() < 0 || index.row() >= _items.size()) return QVariant();
    const HistoryItem &item = _items.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ColNode:  return item.node;
        case ColRange: return item.range;
        default:       return QVariant();
        }
    }

    if (role == Qt::TextAlignmentRole)
        return QVariant(_columnAlignments.alignment(index.column()));

    return QVariant();
}

///
/// \brief Replaces all history rows.
/// \param items New history entries to display.
///
void HistoryModel::setItems(const QVector<HistoryItem> &items)
{
    beginResetModel();
    _items = items;
    endResetModel();
}

///
/// \brief Removes all history rows.
///
void HistoryModel::clear()
{
    beginResetModel();
    _items.clear();
    endResetModel();
}

///
/// \brief Sets the text alignment for a column.
/// \param column Column index.
/// \param alignment Alignment to apply.
///
void HistoryModel::setColumnAlignment(int column, Qt::Alignment alignment)
{
    _columnAlignments.setAlignment(column, alignment);
    emit dataChanged(index(0, column), index(rowCount() - 1, column), {Qt::TextAlignmentRole});
}
