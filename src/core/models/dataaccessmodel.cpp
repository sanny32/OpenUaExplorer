// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dataaccessmodel.cpp
/// \brief Implements the OPC UA data access table model.
///

#include <algorithm>
#include <functional>

#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QPalette>

#include "dataaccessmodel.h"
#include "csvexporter.h"
#include "formatters/attributeformatter.h"

namespace {
OpcUaFormat::TimestampMode toFormatMode(AppSettings::TimestampMode mode)
{
    return mode == AppSettings::TimestampMode::Utc
        ? OpcUaFormat::TimestampMode::Utc
        : OpcUaFormat::TimestampMode::LocalTime;
}

}

///
/// \brief Constructs an empty data-access model.
/// \param parent Owning QObject.
///
DataAccessModel::DataAccessModel(QObject *parent)
    : QAbstractTableModel(parent)
    , _timestampMode(AppSettings().timestampMode())
{
}

///
/// \brief Replaces all rows.
/// \param items New data-access rows.
///
void DataAccessModel::setItems(const QVector<DataAccessItem> &items)
{
    beginResetModel();
    _items = items;
    endResetModel();
}

///
/// \brief Updates the row matching the node, or appends a new row when absent.
/// \param details Node details to add or update.
///
void DataAccessModel::addOrUpdate(const OpcUaNodeDetails &details)
{
    for (int row = 0; row < _items.size(); ++row) {
        if (_items.at(row).nodeId != details.nodeId)
            continue;
        DataAccessItem &item = _items[row];
        item.displayName = details.displayName;
        item.typedValue = details.value;
        item.value = details.value.toString();
        item.valueType = details.valueType;
        item.dataTypeId = details.dataTypeId;
        item.dataType = OpcUaFormat::dataTypeDisplay(details.dataTypeId);
        item.status = details.status;
        item.sourceTimestamp = details.sourceTimestamp;
        item.serverTimestamp = details.serverTimestamp;
        item.userAccessLevel = details.userAccessLevel;
        emit dataChanged(index(row, 0), index(row, ColCount - 1));
        return;
    }

    const int row = _items.size();
    beginInsertRows({}, row, row);
    DataAccessItem item;
    item.nodeId = details.nodeId;
    item.displayName = details.displayName;
    item.typedValue = details.value;
    item.value = details.value.toString();
    item.valueType = details.valueType;
    item.dataTypeId = details.dataTypeId;
    item.dataType = OpcUaFormat::dataTypeDisplay(details.dataTypeId);
    item.status = details.status;
    item.sourceTimestamp = details.sourceTimestamp;
    item.serverTimestamp = details.serverTimestamp;
    item.userAccessLevel = details.userAccessLevel;
    _items.append(item);
    endInsertRows();
}

///
/// \brief Refreshes the value, status, and timestamps of rows matching the read results.
/// \param values Read results.
///
void DataAccessModel::updateValues(const QVector<OpcUaDataValue> &values)
{
    for (const OpcUaDataValue &value : values) {
        for (int row = 0; row < _items.size(); ++row) {
            DataAccessItem &item = _items[row];
            if (item.nodeId != value.nodeId)
                continue;
            item.typedValue = value.value;
            item.value = value.value.toString();
            item.status = value.status;
            item.sourceTimestamp = value.sourceTimestamp;
            item.serverTimestamp = value.serverTimestamp;
            emit dataChanged(index(row, ColValue), index(row, ColStatus));
            break;
        }
    }
}

///
/// \brief Removes the rows referenced by the given indexes, highest row first.
/// \param rows Selected model rows.
///
void DataAccessModel::removeRows(const QModelIndexList &rows)
{
    QList<int> rowNumbers;
    for (const QModelIndex &index : rows) {
        if (!rowNumbers.contains(index.row()))
            rowNumbers.append(index.row());
    }
    std::sort(rowNumbers.begin(), rowNumbers.end(), std::greater<int>());
    for (int row : rowNumbers) {
        if (row < 0 || row >= _items.size())
            continue;
        beginRemoveRows({}, row, row);
        _items.removeAt(row);
        endRemoveRows();
    }
}

///
/// \brief Collects the NodeIds of the given rows, or of every row when none are given.
/// \param rows Optional selected rows.
/// \return NodeIds for selected rows or all rows.
///
QStringList DataAccessModel::nodeIds(const QModelIndexList &rows) const
{
    QStringList result;
    if (rows.isEmpty()) {
        for (const DataAccessItem &item : _items)
            result.append(item.nodeId);
        return result;
    }
    for (const QModelIndex &index : rows) {
        if (index.row() >= 0 && index.row() < _items.size()
            && !result.contains(_items.at(index.row()).nodeId)) {
            result.append(_items.at(index.row()).nodeId);
        }
    }
    return result;
}

///
/// \brief Returns the item at a row.
/// \param row Model row.
/// \return Data item or an empty item.
///
DataAccessItem DataAccessModel::itemAt(int row) const
{
    return row >= 0 && row < _items.size() ? _items.at(row) : DataAccessItem();
}

///
/// \brief Exports the data-access rows as CSV text.
/// \return CSV document with a header row.
///
QString DataAccessModel::toCsv() const
{
    return CsvExporter::tableToCsv(*this);
}

///
/// \brief Removes all rows.
///
void DataAccessModel::clear()
{
    setItems({});
}

///
/// \brief Returns the number of rows.
/// \param parent Parent index; non-root parents have no rows.
/// \return Item count, or 0 for non-root parents.
///
int DataAccessModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return _items.size();
}

///
/// \brief Returns the fixed column count.
/// \param parent Parent index; non-root parents have no columns.
/// \return Column count, or 0 for non-root parents.
///
int DataAccessModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return ColCount;
}

///
/// \brief Returns the column titles.
/// \param section Column index.
/// \param orientation Header orientation.
/// \param role Display role.
/// \return Column title, or the base implementation otherwise.
///
QVariant DataAccessModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QAbstractTableModel::headerData(section, orientation, role);

    switch (section) {
    case ColNumber:       return tr("#");
    case ColNodeId:       return tr("Node Id");
    case ColDisplayName:  return tr("Display Name");
    case ColValue:        return tr("Value");
    case ColDataType:     return tr("Data Type");
    case ColTimestamp:    return tr("Source Timestamp");
    case ColStatus:       return tr("Status");
    case ColSubscription: return tr("Subscription");
    default:              return QVariant();
    }
}

///
/// \brief Marks the Subscription column editable.
/// \param index Cell to query.
/// \return Item flags for the cell.
///
Qt::ItemFlags DataAccessModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags f = QAbstractTableModel::flags(index);
    if (index.column() == ColSubscription)
        f |= Qt::ItemIsEditable;
    return f;
}

///
/// \brief Writes the edited subscription name into the Subscription column.
/// \param index Cell being edited.
/// \param value New subscription name.
/// \param role Only Qt::EditRole is accepted.
/// \return True when the value was applied.
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
/// \brief Returns cell text, alignment, and status/subscription colours for a row.
/// \param index Cell to query.
/// \param role Requested data role.
/// \return Value for the role, or an invalid variant.
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
        case ColTimestamp:    return OpcUaFormat::isoTimestampWithZone(item.sourceTimestamp,
                                                                       toFormatMode(_timestampMode));
        case ColStatus:       return item.status;
        case ColSubscription: return item.subscriptionName.isEmpty()
                                     ? QStringLiteral("\u2014")
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
/// \brief Returns the text alignment for a column.
/// \param column Column index.
/// \return Column alignment.
///
Qt::Alignment DataAccessModel::columnAlignment(int column) const
{
    return _columnAlignments.alignment(column);
}

///
/// \brief Sets the text alignment for a column.
/// \param column Column index.
/// \param alignment Alignment to apply.
///
void DataAccessModel::setColumnAlignment(int column, Qt::Alignment alignment)
{
    _columnAlignments.setAlignment(column, alignment);
    emit dataChanged(index(0, column), index(rowCount() - 1, column), {Qt::TextAlignmentRole});
}

///
/// \brief Sets the timestamp display mode and repaints the timestamp column.
/// \param mode Local time or UTC.
///
void DataAccessModel::setTimestampMode(AppSettings::TimestampMode mode)
{
    if (_timestampMode == mode)
        return;
    _timestampMode = mode;
    if (rowCount() > 0)
        emit dataChanged(index(0, ColTimestamp), index(rowCount() - 1, ColTimestamp),
                         {Qt::DisplayRole});
}

///
/// \brief Re-emits the header titles after a UI language change.
///
void DataAccessModel::retranslate()
{
    emit headerDataChanged(Qt::Horizontal, 0, columnCount(QModelIndex()) - 1);
}
