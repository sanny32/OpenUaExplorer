// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dataaccessmodel.cpp
/// \brief Implements the OPC UA data access table model.
///

#include <algorithm>
#include <functional>
#include <utility>

#include <QBrush>
#include <QColor>
#include <QDataStream>
#include <QDateTime>
#include <QFont>
#include <QGuiApplication>
#include <QIODevice>
#include <QMimeData>
#include <QPalette>
#include <QSet>

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
    : QAbstractItemModel(parent)
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
    _roots.clear();
    _roots.resize(_items.size());
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
        refreshChildren(row, item.valueChangedAt);
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
    _roots.emplace_back();
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
    _roots.emplace_back();
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
            refreshChildren(row, item.valueChangedAt);
            // The NodeId cell carries the expander, and a value can gain or lose its elements.
            emit dataChanged(index(row, ColNodeId), index(row, ColStatus));
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
        if (index.parent().isValid())
            continue;
        if (!rowNumbers.contains(index.row()))
            rowNumbers.append(index.row());
    }
    std::sort(rowNumbers.begin(), rowNumbers.end(), std::greater<int>());
    for (int row : rowNumbers) {
        if (row < 0 || row >= _items.size())
            continue;
        beginRemoveRows({}, row, row);
        _items.removeAt(row);
        _roots.erase(_roots.begin() + row);
        endRemoveRows();
    }
}

///
/// \brief Moves the given rows, kept as one block, in front of a destination row.
/// \param rows Rows to move; indexes of this model, not of a proxy.
/// \param destinationRow Row the block is inserted before; the row count appends.
/// \return True when the row order changed.
///
/// The rows keep their relative order, and the destination is read in the order
/// before the move: dropping between two rows inserts in front of the lower one.
///
bool DataAccessModel::moveRows(const QModelIndexList &rows, int destinationRow)
{
    QSet<int> moving;
    for (const QModelIndex &index : rows) {
        if (!index.parent().isValid() && index.row() >= 0 && index.row() < _items.size())
            moving.insert(index.row());
    }
    if (moving.isEmpty())
        return false;

    const int destination = qBound(0, destinationRow < 0 ? _items.size() : destinationRow,
                                   _items.size());

    // The new order: the rows that stay, split at the destination, with the moved block between.
    QList<int> moved;
    QList<int> kept;
    int insertAt = -1;
    for (int row = 0; row < _items.size(); ++row) {
        if (row == destination)
            insertAt = kept.size();
        if (moving.contains(row))
            moved.append(row);
        else
            kept.append(row);
    }
    if (insertAt < 0)
        insertAt = kept.size();

    QList<int> order = kept.mid(0, insertAt);
    order.append(moved);
    order.append(kept.mid(insertAt));

    bool reordered = false;
    for (int row = 0; row < order.size() && !reordered; ++row)
        reordered = order.at(row) != row;
    if (!reordered)
        return false;

    QVector<DataAccessItem> items;
    items.reserve(order.size());
    std::vector<std::unique_ptr<ValueNode>> roots;
    roots.reserve(order.size());
    for (int sourceRow : std::as_const(order)) {
        items.append(_items.at(sourceRow));
        roots.push_back(std::move(_roots[sourceRow]));
    }

    QList<int> newRowOf(order.size(), 0);
    for (int row = 0; row < order.size(); ++row)
        newRowOf[order.at(row)] = row;

    // A layout change rather than row moves: the "#" column renumbers in one go.
    emit layoutAboutToBeChanged({}, QAbstractItemModel::VerticalSortHint);
    _items = items;
    _roots = std::move(roots);
    // The element rows travel with their item, but they address it by row number.
    for (int row = 0; row < int(_roots.size()); ++row) {
        if (_roots[row])
            _roots[row]->topRow = row;
    }

    const QModelIndexList before = persistentIndexList();
    QModelIndexList after;
    after.reserve(before.size());
    for (const QModelIndex &stale : before) {
        after.append(stale.isValid()
                         ? index(newRowOf.value(stale.row(), stale.row()), stale.column())
                         : stale);
    }
    changePersistentIndexList(before, after);
    emit layoutChanged({}, QAbstractItemModel::VerticalSortHint);
    return true;
}

///
/// \brief Returns the MIME type carrying rows dragged inside the data-access table.
/// \return MIME type string.
///
QString DataAccessModel::rowMimeType()
{
    return QStringLiteral("application/x-ouaexp-data-access-rows");
}

///
/// \brief Reports the drop actions rows dragged inside the table may use.
/// \return Qt::MoveAction.
///
Qt::DropActions DataAccessModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

///
/// \brief Returns the MIME types the table produces and accepts.
/// \return Single-entry list holding rowMimeType().
///
QStringList DataAccessModel::mimeTypes() const
{
    return {rowMimeType()};
}

///
/// \brief Encodes the dragged rows by NodeId, in row order.
/// \param indexes Dragged cells; their rows are used.
/// \return MIME data owned by the caller, or nullptr when nothing is draggable.
///
/// NodeIds travel instead of row numbers so a drop always addresses the rows the
/// user picked up, whatever the table did in between.
///
QMimeData *DataAccessModel::mimeData(const QModelIndexList &indexes) const
{
    QStringList nodeIds;
    QSet<int> seen;
    for (const QModelIndex &index : indexes) {
        if (index.parent().isValid())
            continue;
        if (index.row() < 0 || index.row() >= _items.size() || seen.contains(index.row()))
            continue;
        seen.insert(index.row());
        nodeIds.append(_items.at(index.row()).nodeId);
    }
    if (nodeIds.isEmpty())
        return nullptr;

    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream << nodeIds;

    auto *mimeData = new QMimeData;
    mimeData->setData(rowMimeType(), payload);
    return mimeData;
}

///
/// \brief Accepts a drop of the table's own rows between two rows.
/// \param data Dragged MIME data.
/// \param action Proposed drop action.
/// \param row Row the data would be inserted before, or -1 to append.
/// \param column Unused; rows move as a whole.
/// \param parent Drop parent; only the root accepts rows.
/// \return True when the drop would reorder rows.
///
bool DataAccessModel::canDropMimeData(const QMimeData *data, Qt::DropAction action,
                                      int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(row)
    Q_UNUSED(column)
    return !_offline && !parent.isValid() && action == Qt::MoveAction
        && data && data->hasFormat(rowMimeType());
}

///
/// \brief Reorders the dragged rows in front of the drop row.
/// \param data Dragged MIME data.
/// \param action Drop action; only Qt::MoveAction reorders.
/// \param row Row the data is inserted before, or -1 to append.
/// \param column Unused; rows move as a whole.
/// \param parent Drop parent; only the root accepts rows.
/// \return True when the drop was handled.
///
/// The move happens here, so the dragged rows must not be removed afterwards: the
/// view skips its follow-up removal once the model reports the move handled, and
/// QAbstractItemModel::removeRows() is left unimplemented as a second guard.
///
bool DataAccessModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
                                   int row, int column, const QModelIndex &parent)
{
    if (!canDropMimeData(data, action, row, column, parent))
        return false;

    QStringList nodeIds;
    QDataStream stream(data->data(rowMimeType()));
    stream >> nodeIds;
    if (stream.status() != QDataStream::Ok || nodeIds.isEmpty())
        return false;

    QModelIndexList dragged;
    for (int candidate = 0; candidate < _items.size(); ++candidate) {
        if (nodeIds.contains(_items.at(candidate).nodeId))
            dragged.append(index(candidate, 0));
    }

    moveRows(dragged, row);
    // Handled either way: an unchanged order is a drop that simply moved nothing.
    return true;
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
        if (!index.parent().isValid() && index.row() >= 0 && index.row() < _items.size()
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

    for (int row = 0; row < int(_roots.size()); ++row) {
        const ValueNode *root = _roots[row].get();
        if (!root || root->children.empty())
            continue;
        const QModelIndex parentIndex = index(row, 0);
        emit dataChanged(index(0, 0, parentIndex),
                         index(int(root->children.size()) - 1, ColCount - 1, parentIndex),
                         {Qt::ForegroundRole});
    }
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
/// \brief Returns the element tree of a top-level row, creating an empty root on first use.
/// \param topRow Top-level row.
/// \return Root node of the row, or null for an out-of-range row.
///
DataAccessModel::ValueNode *DataAccessModel::rootNode(int topRow) const
{
    if (topRow < 0 || topRow >= int(_roots.size()))
        return nullptr;
    if (!_roots[topRow]) {
        _roots[topRow] = std::make_unique<ValueNode>();
        _roots[topRow]->topRow = topRow;
    }
    return _roots[topRow].get();
}

///
/// \brief Returns the element node an index addresses.
/// \param index Index to resolve.
/// \return Element node, or null for a top-level row or an invalid index.
///
DataAccessModel::ValueNode *DataAccessModel::nodeForIndex(const QModelIndex &index) const
{
    return index.isValid() ? static_cast<ValueNode *>(index.internalPointer()) : nullptr;
}

///
/// \brief Returns the top-level row an element node belongs to.
/// \param node Element node.
/// \return Row of the monitored node owning the element.
///
/// Only the root of a tree tracks its row, so moving an item leaves the elements below it
/// untouched.
///
int DataAccessModel::topRowOf(const ValueNode *node)
{
    while (node->parent)
        node = node->parent;
    return node->topRow;
}

///
/// \brief Returns the item an element node belongs to.
/// \param node Element node.
/// \return Owning item, or an empty item when the row is gone.
///
DataAccessItem DataAccessModel::itemForNode(const ValueNode *node) const
{
    return itemAt(topRowOf(node));
}

///
/// \brief Fills a node with the elements of its value.
/// \param node Node to expand; its own value is used, or the item's value for a root.
/// \param item Row the node belongs to, used to name elements the value cannot name itself.
///
/// Only the first MaxExpandedElements elements become rows; the rest are summarised by a
/// trailing placeholder so a huge array still opens instantly.
///
void DataAccessModel::buildChildren(ValueNode *node, const DataAccessItem &item) const
{
    node->children.clear();
    node->childrenBuilt = true;

    const QVariant value = node->parent ? node->value : item.typedValue;
    int total = 0;
    const QVector<OpcUaFormat::ValueElement> elements =
        OpcUaFormat::valueElements(value, MaxExpandedElements, &total);

    node->children.reserve(elements.size() + 1);
    for (const OpcUaFormat::ValueElement &element : elements) {
        auto child = std::make_unique<ValueNode>();
        child->label = element.label;
        child->text = element.text;
        child->value = element.value;
        child->expandable = element.hasChildren;
        child->typeName = element.typeName.isEmpty()
            ? (element.hasChildren
                   ? QStringLiteral("%1[%2]").arg(item.dataType).arg(element.value.toList().size())
                   : item.dataType)
            : element.typeName;
        child->row = int(node->children.size());
        child->parent = node;
        node->children.push_back(std::move(child));
    }

    if (total > elements.size()) {
        auto more = std::make_unique<ValueNode>();
        more->label = tr("… %n more", nullptr, total - elements.size());
        more->placeholder = true;
        more->row = int(node->children.size());
        more->parent = node;
        node->children.push_back(std::move(more));
    }
}

///
/// \brief Applies a new value to the elements of an expanded row.
/// \param topRow Top-level row whose value changed.
/// \param changedAt Time to stamp on the elements that changed.
///
/// Rows that were never expanded keep no elements to refresh: they are built from the current
/// value the moment they are opened.
///
void DataAccessModel::refreshChildren(int topRow, qint64 changedAt)
{
    if (topRow < 0 || topRow >= int(_roots.size()) || !_roots[topRow])
        return;
    ValueNode *root = _roots[topRow].get();
    if (!root->childrenBuilt)
        return;

    const DataAccessItem &item = _items.at(topRow);
    updateNode(root, index(topRow, 0), item.typedValue, item, changedAt);
}

///
/// \brief Refreshes one node's elements in place, stamping the ones whose text changed.
/// \param node Node to refresh.
/// \param nodeIndex Index of the node, used as the parent of its element rows.
/// \param value New value of the node.
/// \param item Row the node belongs to.
/// \param changedAt Time to stamp on the elements that changed.
///
/// An array that keeps its length keeps its rows, so the view flashes the elements that
/// really moved instead of rebuilding the whole block on every notification.
///
void DataAccessModel::updateNode(ValueNode *node, const QModelIndex &nodeIndex,
                                 const QVariant &value, const DataAccessItem &item,
                                 qint64 changedAt)
{
    node->value = value;
    if (!node->childrenBuilt)
        return;

    int total = 0;
    const QVector<OpcUaFormat::ValueElement> elements =
        OpcUaFormat::valueElements(value, MaxExpandedElements, &total);
    const int expected = elements.size() + (total > elements.size() ? 1 : 0);

    if (expected != int(node->children.size())) {
        // The elements stay "built" while empty: a row count of zero between the two signals
        // is the truth, and rebuilding them early would contradict the insert about to follow.
        if (!node->children.empty()) {
            beginRemoveRows(nodeIndex, 0, int(node->children.size()) - 1);
            node->children.clear();
            endRemoveRows();
        }
        if (expected > 0) {
            beginInsertRows(nodeIndex, 0, expected - 1);
            buildChildren(node, item);
            endInsertRows();
        }
        return;
    }

    for (int row = 0; row < elements.size(); ++row) {
        ValueNode *child = node->children[row].get();
        const OpcUaFormat::ValueElement &element = elements.at(row);
        if (child->text != element.text)
            child->changedAt = changedAt;
        child->text = element.text;
        child->expandable = element.hasChildren;
        if (child->childrenBuilt)
            updateNode(child, index(row, 0, nodeIndex), element.value, item, changedAt);
        else
            child->value = element.value;
    }

    if (!node->children.empty()) {
        emit dataChanged(index(0, ColNodeId, nodeIndex),
                         index(int(node->children.size()) - 1, ColDataType, nodeIndex));
    }
}

///
/// \brief Returns the index of a row, either a monitored node or an element of one.
/// \param row Row within the parent.
/// \param column Model column.
/// \param parent Parent index; an invalid parent addresses the monitored nodes.
/// \return Model index, or an invalid index when the row does not exist.
///
QModelIndex DataAccessModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row < 0 || column < 0 || column >= ColCount)
        return QModelIndex();

    if (!parent.isValid()) {
        return row < _items.size() ? createIndex(row, column, nullptr) : QModelIndex();
    }

    ValueNode *parentNode = nodeForIndex(parent);
    if (!parentNode) {
        if (parent.row() >= _items.size())
            return QModelIndex();
        parentNode = rootNode(parent.row());
        if (!parentNode)
            return QModelIndex();
    }
    if (!parentNode->childrenBuilt)
        buildChildren(parentNode, itemForNode(parentNode));

    if (row >= int(parentNode->children.size()))
        return QModelIndex();
    return createIndex(row, column, parentNode->children[row].get());
}

///
/// \brief Returns the parent index of an element row.
/// \param child Child index.
/// \return Parent index, or an invalid index for monitored nodes.
///
QModelIndex DataAccessModel::parent(const QModelIndex &child) const
{
    ValueNode *node = nodeForIndex(child);
    if (!node || !node->parent)
        return QModelIndex();

    ValueNode *parentNode = node->parent;
    if (!parentNode->parent)
        return createIndex(parentNode->topRow, 0, nullptr);
    return createIndex(parentNode->row, 0, parentNode);
}

///
/// \brief Reports whether a row expands, without building its elements.
/// \param parent Row to query.
/// \return True when the row has child rows.
///
bool DataAccessModel::hasChildren(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return !_items.isEmpty();
    if (ValueNode *node = nodeForIndex(parent))
        return node->expandable;
    return parent.row() < _items.size()
        && OpcUaFormat::hasValueElements(_items.at(parent.row()).typedValue);
}

///
/// \brief Returns the number of rows, building the elements of an expanded row on demand.
/// \param parent Parent index.
/// \return Item count for the root, or the element count of a composite value.
///
int DataAccessModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return _items.size();
    if (parent.column() > 0)
        return 0;

    ValueNode *node = nodeForIndex(parent);
    if (!node) {
        if (parent.row() >= _items.size()
            || !OpcUaFormat::hasValueElements(_items.at(parent.row()).typedValue)) {
            return 0;
        }
        node = rootNode(parent.row());
        if (!node)
            return 0;
    } else if (!node->expandable) {
        return 0;
    }

    if (!node->childrenBuilt)
        buildChildren(node, itemForNode(node));
    return int(node->children.size());
}

///
/// \brief Returns the fixed column count.
/// \param parent Parent index.
/// \return Column count.
///
int DataAccessModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
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
        return QAbstractItemModel::headerData(section, orientation, role);

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
/// \brief Marks the Subscription column editable and every row draggable.
/// \param index Cell to query.
/// \return Item flags for the cell.
///
/// Pending rows stay selectable so they can still be removed, but they offer no
/// editing until their attribute read and subscription have finished. Rows are
/// draggable whatever their state: their order is a display preference. They are
/// never drop targets themselves, so a drop only ever lands between two rows.
///
Qt::ItemFlags DataAccessModel::flags(const QModelIndex &index) const
{
    // The root accepts drops so rows can be dragged past the last one.
    if (!index.isValid())
        return QAbstractItemModel::flags(index) | Qt::ItemIsDropEnabled;

    // An element belongs to its row: it is read here, and acted on through its node.
    if (nodeForIndex(index))
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    Qt::ItemFlags f = QAbstractItemModel::flags(index) | Qt::ItemIsDragEnabled;
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
    if (index.parent().isValid()) return false;
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
    if (const ValueNode *node = nodeForIndex(index))
        return nodeData(node, index.column(), role);
    if (index.row() < 0 || index.row() >= _items.size()) return QVariant();

    const DataAccessItem &item = _items.at(index.row());
    const int col = index.column();

    if (role == Qt::DisplayRole) {
        switch (col) {
        case ColNumber:       return index.row() + 1;
        case ColNodeId:       return item.nodeId;
        case ColDisplayName:  return item.displayName;
        // The elements carry the values once the row is open; the cell names the array instead.
        case ColValue:        return OpcUaFormat::hasValueElements(item.typedValue)
                                     ? OpcUaFormat::valueSummary(
                                           item.typedValue,
                                           static_cast<QOpcUa::Types>(item.valueType),
                                           item.dataTypeId)
                                     : item.value;
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

    if (role == Qt::FontRole && item.pending) {
        QFont font = qApp->font();
        font.setItalic(true);
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
    case ExpectedIntervalRole: return item.subscriptionName.isEmpty()
                                      ? 0.0
                                      : item.revisedPublishingInterval;
    case HighlightChangesRole: return resolveHighlight(item);
    default: break;
    }

    return QVariant();
}

///
/// \brief Returns the cell data of an element row.
/// \param node Element the row shows.
/// \param column Model column.
/// \param role Requested data role.
/// \return Value for the role, or an invalid variant.
///
/// An element belongs to its monitored node: quality, publishing interval and highlight
/// preference are the item's, while the change stamp is the element's own so only the
/// elements that moved are flashed.
///
QVariant DataAccessModel::nodeData(const ValueNode *node, int column, int role) const
{
    const DataAccessItem item = itemForNode(node);

    switch (role) {
    case Qt::DisplayRole:
        switch (column) {
        case ColNodeId:   return node->label;
        case ColValue:    return node->placeholder ? QString() : node->text;
        case ColDataType: return node->placeholder ? QString() : node->typeName;
        default:          return QVariant();
        }
    case Qt::TextAlignmentRole:
        return QVariant(columnAlignment(column));
    case Qt::ForegroundRole:
        if (_offline || node->placeholder || item.subscriptionName.isEmpty())
            return QBrush(qApp->palette().color(QPalette::Disabled, QPalette::Text));
        break;
    case ValueChangedAtRole:
        return node->changedAt;
    case StatusSeverityRole:
        return int(OpcUaFormat::statusSeverity(item.status));
    case ExpectedIntervalRole:
        return item.subscriptionName.isEmpty() ? 0.0 : item.revisedPublishingInterval;
    case HighlightChangesRole:
        return !node->placeholder && resolveHighlight(item);
    default:
        break;
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
        if (row.parent().isValid() || row.row() < 0 || row.row() >= _items.size())
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
