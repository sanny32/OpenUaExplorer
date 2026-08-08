// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file logmodel.cpp
/// \brief Implements the application log table model.
///

#include <QColor>

#include "appsettings.h"
#include "logmodel.h"

namespace {

///
/// \brief Returns the short label for a log level.
/// \param level Log level.
/// \return "DEBUG", "INFO", "WARN", or "ERROR".
///
QString levelText(LogItem::Level level)
{
    switch (level) {
    case LogItem::Level::Debug:   return "DEBUG";
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
    case LogItem::Level::Debug:   return QColor(130, 130, 130);
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
    , _maxRows(AppSettings::defaultMaxLogRows)
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
    case ColTimestamp: return tr("Time");
    case ColLevel:     return tr("Level");
    case ColSource:    return tr("Source");
    case ColMessage:   return tr("Message");
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
    if (matchesFilters(item)) {
        const int row = visibleItems().size();
        beginInsertRows(QModelIndex(), row, row);
        _items.append(item);
        endInsertRows();
    } else {
        _items.append(item);
    }
    trimToMaxRows();
}

///
/// \brief Returns how many entries the model keeps.
/// \return Current row cap.
///
int LogModel::maxRows() const
{
    return _maxRows;
}

///
/// \brief Caps how many entries the model keeps, dropping the oldest beyond the cap.
/// \param rows Row cap; values below one are raised to one.
///
void LogModel::setMaxRows(int rows)
{
    const int wanted = qMax(1, rows);
    if (_maxRows == wanted)
        return;
    _maxRows = wanted;
    trimToMaxRows();
}

///
/// \brief Drops the oldest entries until the model is back within its row cap.
///
/// The cap counts every entry the model holds, not only the rows a filter leaves visible, so
/// what the log costs in memory does not depend on what the user is currently looking at.
/// Dropped entries are the oldest ones, and those of them that are visible are always the
/// leading rows of the view.
///
void LogModel::trimToMaxRows()
{
    const int excess = _items.size() - _maxRows;
    if (excess <= 0)
        return;

    int visibleExcess = 0;
    for (int index = 0; index < excess; ++index) {
        if (matchesFilters(_items.at(index)))
            ++visibleExcess;
    }

    if (visibleExcess > 0)
        beginRemoveRows(QModelIndex(), 0, visibleExcess - 1);
    _items.remove(0, excess);
    if (visibleExcess > 0)
        endRemoveRows();
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
/// \brief Filters log rows to those originating from a single source.
/// \param source Exact source to show; empty clears the filter.
///
void LogModel::setSourceFilter(const QString &source)
{
    beginResetModel();
    _sourceFilter = source;
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
        if (matchesFilters(item))
            result.append(item);
    }
    return result;
}

///
/// \brief Reports whether an entry survives the active level, search and source filters.
/// \param item Entry to test.
/// \return True when the entry is shown.
///
bool LogModel::matchesFilters(const LogItem &item) const
{
    if (_filtered && item.level != _filterLevel)
        return false;
    if (!_searchText.isEmpty() && !item.message.contains(_searchText, Qt::CaseInsensitive))
        return false;
    if (!_sourceFilter.isEmpty() && item.source != _sourceFilter)
        return false;
    return true;
}

///
/// \brief Re-emits the header titles after a UI language change.
///
void LogModel::retranslate()
{
    emit headerDataChanged(Qt::Horizontal, 0, columnCount(QModelIndex()) - 1);
}
