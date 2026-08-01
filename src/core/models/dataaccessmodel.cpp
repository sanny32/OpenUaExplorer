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
#include <QDateTime>
#include <QFont>
#include <QFontDatabase>
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

///
/// \brief Returns the platform fixed-pitch font scaled to the interface font size.
/// \return Monospace font at the application point size.
///
/// The system fixed font carries its own point size, which does not always match
/// the interface font, so only the family and style are taken from it.
///
QFont monospaceFont()
{
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    const QFont appFont = qApp->font();
    if (appFont.pointSizeF() > 0.0)
        font.setPointSizeF(appFont.pointSizeF());
    else if (appFont.pixelSize() > 0)
        font.setPixelSize(appFont.pixelSize());
    return font;
}

}

///
/// \brief Constructs an empty data-access model.
/// \param parent Owning QObject.
///
DataAccessModel::DataAccessModel(QObject *parent)
    : QAbstractTableModel(parent)
    , _timestampMode(AppSettings().timestampMode())
    , _defaultHighlightChanges(AppSettings().highlightValueChanges())
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
        const QString formatted = OpcUaFormat::displayValue(details.value);
        DataAccessItem &item = _items[row];
        item.displayName = details.displayName;
        item.typedValue = details.value;
        if (item.value != formatted)
            item.valueChangedAt = QDateTime::currentMSecsSinceEpoch();
        item.value = formatted;
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
    item.value = OpcUaFormat::displayValue(details.value);
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
/// \brief Appends a placeholder row for a node whose attributes are still being read.
/// \param node Browsed node to show.
///
/// Existing rows are left untouched so an already monitored node is never pushed
/// back into the pending state.
///
void DataAccessModel::addPending(const OpcUaNodeInfo &node)
{
    for (const DataAccessItem &item : _items) {
        if (item.nodeId == node.nodeId)
            return;
    }

    const int row = _items.size();
    beginInsertRows({}, row, row);
    DataAccessItem item;
    item.nodeId = node.nodeId;
    item.displayName = node.displayName.isEmpty() ? node.browseName : node.displayName;
    item.pending = true;
    _items.append(item);
    endInsertRows();
}

///
/// \brief Clears the pending mark of a row once its request chain has finished.
/// \param nodeId Node to update.
///
void DataAccessModel::clearPending(const QString &nodeId)
{
    for (int row = 0; row < _items.size(); ++row) {
        DataAccessItem &item = _items[row];
        if (item.nodeId != nodeId || !item.pending)
            continue;
        item.pending = false;
        emit dataChanged(index(row, 0), index(row, ColCount - 1));
        return;
    }
}

///
/// \brief Reports whether a row is still waiting for its attributes or subscription.
/// \param nodeId Node to query.
/// \return True while the row is pending.
///
bool DataAccessModel::isPending(const QString &nodeId) const
{
    for (const DataAccessItem &item : _items) {
        if (item.nodeId == nodeId)
            return item.pending;
    }
    return false;
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
            const QString formatted = OpcUaFormat::displayValue(value.value);
            item.typedValue = value.value;
            // A notification repeating the previous value is not a change, and must not re-flash.
            if (item.value != formatted)
                item.valueChangedAt = QDateTime::currentMSecsSinceEpoch();
            item.value = formatted;
            item.status = value.status;
            item.sourceTimestamp = value.sourceTimestamp;
            item.serverTimestamp = value.serverTimestamp;
            emit dataChanged(index(row, ColValue), index(row, ColStatus));
            break;
        }
    }
}

///
/// \brief Records the publishing interval the server granted for a monitored node.
/// \param nodeId Affected node.
/// \param publishingInterval Granted interval in milliseconds; 0 clears the shown value.
///
void DataAccessModel::setRevisedInterval(const QString &nodeId, double publishingInterval)
{
    for (int row = 0; row < _items.size(); ++row) {
        DataAccessItem &item = _items[row];
        if (item.nodeId != nodeId)
            continue;
        if (qFuzzyCompare(item.revisedPublishingInterval, publishingInterval))
            return;
        item.revisedPublishingInterval = publishingInterval;
        const QModelIndex changed = index(row, ColActualInterval);
        emit dataChanged(changed, changed, {Qt::DisplayRole});
        return;
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
/// \brief Marks the listed values as belonging to a connection that is gone.
/// \param offline True while the server connection is gone.
///
void DataAccessModel::setOffline(bool offline)
{
    if (_offline == offline)
        return;
    _offline = offline;
    if (_items.isEmpty())
        return;
    emit dataChanged(index(0, 0), index(_items.size() - 1, ColCount - 1),
                     {Qt::ForegroundRole});
}

///
/// \brief Reports whether the rows show values of a lost connection.
/// \return True while the rows are offline.
///
bool DataAccessModel::isOffline() const
{
    return _offline;
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
    case ColNumber:       return QStringLiteral("#");
    case ColNodeId:       return tr("Node Id");
    case ColDisplayName:  return tr("Display Name");
    case ColValue:        return tr("Value");
    case ColDataType:     return tr("Data Type");
    case ColTimestamp:    return tr("Source Timestamp");
    case ColStatus:       return tr("Status");
    case ColSubscription: return tr("Subscription");
    case ColActualInterval: return tr("Actual Interval");
    default:              return QVariant();
    }
}

///
/// \brief Marks the Subscription column editable, except on pending rows.
/// \param index Cell to query.
/// \return Item flags for the cell.
///
/// Pending rows stay selectable so they can still be removed, but they offer no
/// editing until their attribute read and subscription have finished.
///
Qt::ItemFlags DataAccessModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags f = QAbstractTableModel::flags(index);
    if (_offline)
        return f;
    if (index.row() >= 0 && index.row() < _items.size() && _items.at(index.row()).pending)
        return f;
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
    if (role != Qt::EditRole || !index.isValid() || _offline) return false;
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
        case ColStatus:       return item.pending && item.status.isEmpty()
                                     ? tr("Pending\u2026")
                                     : item.status;
        case ColSubscription: return item.subscriptionName.isEmpty()
                                     ? QStringLiteral("\u2014")
                                     : item.subscriptionName;
        case ColActualInterval: return item.revisedPublishingInterval > 0.0
                                     ? QStringLiteral("%1 ms").arg(item.revisedPublishingInterval)
                                     : QStringLiteral("\u2014");
        default:              return QVariant();
        }
    }

    if (role == Qt::TextAlignmentRole)
        return QVariant(columnAlignment(col));

    if (role == Qt::EditRole && col == ColSubscription)
        return item.subscriptionName;

    // Values line up across rows only in a fixed-pitch font; pending rows stay italic either way.
    if (role == Qt::FontRole && (item.pending || col == ColValue)) {
        QFont font = col == ColValue ? monospaceFont() : qApp->font();
        font.setItalic(item.pending);
        return font;
    }

    if (role == Qt::ForegroundRole) {
        if (_offline)
            return QBrush(qApp->palette().color(QPalette::Disabled, QPalette::Text));
        if (item.pending || item.subscriptionName.isEmpty()) {
            return QBrush(qApp->palette().color(QPalette::Disabled, QPalette::Text));
        }
        if (col == ColSubscription)
            return QBrush(QColor(0, 120, 200));
    }

    switch (role) {
    case ValueChangedAtRole:   return item.valueChangedAt;
    case StatusSeverityRole:   return int(OpcUaFormat::statusSeverity(item.status));
    case HighlightChangesRole: return resolveHighlight(item);
    default: break;
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
/// \brief Overrides the change-highlight preference of the given rows.
/// \param rows Rows to change; indexes of this model, not of a proxy.
/// \param mode Preference to store.
///
void DataAccessModel::setHighlightMode(const QModelIndexList &rows, HighlightMode mode)
{
    for (const QModelIndex &row : rows) {
        if (row.row() < 0 || row.row() >= _items.size())
            continue;
        DataAccessItem &item = _items[row.row()];
        if (item.highlight == mode)
            continue;
        item.highlight = mode;
        const QModelIndex changed = index(row.row(), ColValue);
        emit dataChanged(changed, changed, {HighlightChangesRole});
    }
}

///
/// \brief Returns the stored change-highlight preference of a row.
/// \param row Model row.
/// \return Stored preference, or FollowDefault for an out-of-range row.
///
HighlightMode DataAccessModel::highlightMode(int row) const
{
    return row >= 0 && row < _items.size() ? _items.at(row).highlight
                                           : HighlightMode::FollowDefault;
}

///
/// \brief Reports the change-highlight preference a row resolves to.
/// \param row Model row.
/// \return True when changes of that row should be highlighted.
///
bool DataAccessModel::highlightsChanges(int row) const
{
    return row >= 0 && row < _items.size() ? resolveHighlight(_items.at(row))
                                           : _defaultHighlightChanges;
}

///
/// \brief Resolves a row's highlight preference against the application-wide default.
/// \param item Row to resolve.
/// \return True when changes of that row should be highlighted.
///
bool DataAccessModel::resolveHighlight(const DataAccessItem &item) const
{
    switch (item.highlight) {
    case HighlightMode::Enabled:  return true;
    case HighlightMode::Disabled: return false;
    case HighlightMode::FollowDefault: break;
    }
    return _defaultHighlightChanges;
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
/// \brief Sets the change-highlight preference rows follow unless overridden.
/// \param enabled True to highlight value changes by default.
///
void DataAccessModel::setDefaultHighlightChanges(bool enabled)
{
    if (_defaultHighlightChanges == enabled)
        return;
    _defaultHighlightChanges = enabled;
    if (rowCount() > 0)
        emit dataChanged(index(0, ColValue), index(rowCount() - 1, ColValue),
                         {HighlightChangesRole});
}

///
/// \brief Re-emits the header titles after a UI language change.
///
void DataAccessModel::retranslate()
{
    emit headerDataChanged(Qt::Horizontal, 0, columnCount(QModelIndex()) - 1);
}
