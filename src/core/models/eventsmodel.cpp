// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file eventsmodel.cpp
/// \brief Implements the OPC UA events table model.
///

#include "eventsmodel.h"

namespace {
constexpr int kMaxEventRows = 1000;
}

///
/// \brief Constructs an empty events model.
/// \param parent Owning QObject.
///
EventsModel::EventsModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

///
/// \brief Returns the number of event rows.
/// \param parent Parent index; non-root parents have no rows.
/// \return Event count, or 0 for non-root parents.
///
int EventsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return _items.size();
}

///
/// \brief Returns the fixed column count.
/// \param parent Parent index; non-root parents have no columns.
/// \return Column count, or 0 for non-root parents.
///
int EventsModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return ColCount;
}

///
/// \brief Returns the Time/Message column titles.
/// \param section Column index.
/// \param orientation Header orientation.
/// \param role Display role.
/// \return Column title, or the base implementation otherwise.
///
QVariant EventsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QAbstractTableModel::headerData(section, orientation, role);

    switch (section) {
    case ColTime:      return QStringLiteral("Time");
    case ColSeverity:  return QStringLiteral("Severity");
    case ColSource:    return QStringLiteral("Source");
    case ColMessage:   return QStringLiteral("Message");
    case ColEventType: return QStringLiteral("Event Type");
    default:           return QVariant();
    }
}

///
/// \brief Returns the time/message text and column alignment for an event row.
/// \param index Cell to query.
/// \param role Requested data role.
/// \return Value for the role, or an invalid variant.
///
QVariant EventsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    if (index.row() < 0 || index.row() >= _items.size()) return QVariant();
    const EventItem &item = _items.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ColTime:      return item.time;
        case ColSeverity:  return item.severity;
        case ColSource:    return item.source;
        case ColMessage:   return item.message;
        case ColEventType: return item.eventType;
        default:           return QVariant();
        }
    }

    if (role == Qt::TextAlignmentRole)
        return QVariant(_columnAlignments.alignment(index.column()));

    return QVariant();
}

///
/// \brief Replaces all event rows.
/// \param items New events to display.
///
void EventsModel::setItems(const QVector<EventItem> &items)
{
    beginResetModel();
    _items = items;
    endResetModel();
}

///
/// \brief Appends streamed events, capping the table at a maximum row count.
/// \param items Events to add in arrival order.
///
void EventsModel::addEvents(const QVector<EventItem> &items)
{
    if (items.isEmpty())
        return;

    const int first = _items.size();
    beginInsertRows(QModelIndex(), first, first + items.size() - 1);
    _items.append(items);
    endInsertRows();

    if (_items.size() > kMaxEventRows) {
        const int excess = _items.size() - kMaxEventRows;
        beginRemoveRows(QModelIndex(), 0, excess - 1);
        _items.remove(0, excess);
        endRemoveRows();
    }
}

///
/// \brief Removes all events.
///
void EventsModel::clear()
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
void EventsModel::setColumnAlignment(int column, Qt::Alignment alignment)
{
    _columnAlignments.setAlignment(column, alignment);
    emit dataChanged(index(0, column), index(rowCount() - 1, column), {Qt::TextAlignmentRole});
}
