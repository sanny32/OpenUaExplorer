// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file logmodel.cpp
/// \brief Implements the application log table model.
///

#include <QColor>

#include "logmodel.h"

namespace {

///
/// \brief Returns the short label for a log level.
/// \param level Log level.
/// \return "INFO", "WARN", or "ERROR".
///
QString levelText(LogItem::Level level)
{
    switch (level) {
    case LogItem::Level::Info:    return "INFO";
    case LogItem::Level::Warning: return "WARN";
    case LogItem::Level::Error:   return "ERROR";
    }
    return {};
}

///
/// \brief Returns the display colour for a log level.
/// \param level Log level.
/// \return Colour used for the level cell.
///
QColor levelColor(LogItem::Level level)
{
    switch (level) {
    case LogItem::Level::Info:    return QColor(0, 150, 64);
    case LogItem::Level::Warning: return QColor(200, 140, 0);
    case LogItem::Level::Error:   return QColor(200, 40, 40);
    }
    return {};
}

} // namespace

///
/// \brief Constructs an empty log model.
/// \param parent Owning QObject.
///
LogModel::LogModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

///
/// \brief Returns the number of visible (filtered) log rows.
/// \param parent Parent index; non-root parents have no rows.
/// \return Visible item count, or 0 for non-root parents.
///
int LogModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return visibleItems().size();
}

///
/// \brief Returns the fixed column count.
/// \param parent Parent index; non-root parents have no columns.
/// \return Column count, or 0 for non-root parents.
///
int LogModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return ColCount;
}

///
/// \brief Returns the Time/Level/Source/Message column titles.
/// \param section Column index.
/// \param orientation Header orientation.
/// \param role Display role.
/// \return Column title, or the base implementation otherwise.
///
QVariant LogModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QAbstractTableModel::headerData(section, orientation, role);

    switch (section) {
    case ColTimestamp: return QStringLiteral("Time");
    case ColLevel:     return QStringLiteral("Level");
    case ColSource:    return QStringLiteral("Source");
    case ColMessage:   return QStringLiteral("Message");
    default:           return QVariant();
    }
}

///
/// \brief Returns cell text, the level colour, and column alignment for a log row.
/// \param index Cell to query.
/// \param role Requested data role.
/// \return Value for the role, or an invalid variant.
///
QVariant LogModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();

    const auto items = visibleItems();
    if (index.row() < 0 || index.row() >= items.size()) return QVariant();

    const LogItem &item = items.at(index.row());
    const int col = index.column();

    if (role == Qt::DisplayRole) {
        switch (col) {
        case ColTimestamp: return item.timestamp;
        case ColLevel:     return levelText(item.level);
        case ColSource:    return item.source;
        case ColMessage:   return item.message;
        default:           return QVariant();
        }
    }

    if (role == Qt::ForegroundRole && col == ColLevel)
        return QColor(levelColor(item.level));

    if (role == Qt::TextAlignmentRole)
        return QVariant(_columnAlignments.alignment(col));

    return QVariant();
}

///
/// \brief Appends a log entry, inserting a row only when it passes the active filters.
/// \param item Log entry to add.
///
void LogModel::addItem(const LogItem &item)
{
    const bool levelOk  = !_filtered || item.level == _filterLevel;
    const bool searchOk = _searchText.isEmpty()
                          || item.message.contains(_searchText, Qt::CaseInsensitive);
    if (levelOk && searchOk) {
        const int row = visibleItems().size();
        beginInsertRows(QModelIndex(), row, row);
        _items.append(item);
        endInsertRows();
    } else {
        _items.append(item);
    }
}

///
/// \brief Removes all log entries.
///
void LogModel::clear()
{
    beginResetModel();
    _items.clear();
    endResetModel();
}

///
/// \brief Returns the level the log is filtered to.
/// \return Active filter level.
///
LogItem::Level LogModel::filterLevel() const
{
    return _filterLevel;
}

///
/// \brief Filters the log to a single level.
/// \param level Level to show.
///
void LogModel::setFilterLevel(LogItem::Level level)
{
    beginResetModel();
    _filtered = true;
    _filterLevel = level;
    endResetModel();
}

///
/// \brief Removes the level filter, showing all levels.
///
void LogModel::clearFilterLevel()
{
    beginResetModel();
    _filtered = false;
    endResetModel();
}

///
/// \brief Filters log rows to those whose message contains the search text.
/// \param text Case-insensitive substring filter; empty clears it.
///
void LogModel::setSearchFilter(const QString &text)
{
    beginResetModel();
    _searchText = text;
    endResetModel();
}

///
/// \brief Sets the text alignment for a column.
/// \param column Column index.
/// \param alignment Alignment to apply.
///
void LogModel::setColumnAlignment(int column, Qt::Alignment alignment)
{
    _columnAlignments.setAlignment(column, alignment);
    emit dataChanged(index(0, column), index(rowCount() - 1, column), {Qt::TextAlignmentRole});
}

///
/// \brief Returns the entries passing the level and search filters.
/// \return Filtered log entries in insertion order.
///
QVector<LogItem> LogModel::visibleItems() const
{
    QVector<LogItem> result;
    for (const LogItem &item : _items) {
        if (_filtered && item.level != _filterLevel)
            continue;
        if (!_searchText.isEmpty() && !item.message.contains(_searchText, Qt::CaseInsensitive))
            continue;
        result.append(item);
    }
    return result;
}
