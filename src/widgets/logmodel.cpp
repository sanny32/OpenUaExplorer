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
/// \brief levelText
/// \param level
/// \return
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
/// \brief levelColor
/// \param level
/// \return
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
/// \brief LogModel::LogModel
/// \param parent
///
LogModel::LogModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

///
/// \brief LogModel::rowCount
/// \param parent
/// \return
///
int LogModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return visibleItems().size();
}

///
/// \brief LogModel::columnCount
/// \param parent
/// \return
///
int LogModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return ColCount;
}

///
/// \brief LogModel::headerData
/// \param section
/// \param orientation
/// \param role
/// \return
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
/// \brief LogModel::data
/// \param index
/// \param role
/// \return
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
/// \brief LogModel::addItem
/// \param item
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
/// \brief LogModel::clear
///
void LogModel::clear()
{
    beginResetModel();
    _items.clear();
    endResetModel();
}

///
/// \brief LogModel::filterLevel
/// \return
///
LogItem::Level LogModel::filterLevel() const
{
    return _filterLevel;
}

///
/// \brief LogModel::setFilterLevel
/// \param level
///
void LogModel::setFilterLevel(LogItem::Level level)
{
    beginResetModel();
    _filtered = true;
    _filterLevel = level;
    endResetModel();
}

///
/// \brief LogModel::clearFilterLevel
///
void LogModel::clearFilterLevel()
{
    beginResetModel();
    _filtered = false;
    endResetModel();
}

///
/// \brief LogModel::setSearchFilter
/// \param text
///
void LogModel::setSearchFilter(const QString &text)
{
    beginResetModel();
    _searchText = text;
    endResetModel();
}

///
/// \brief LogModel::setColumnAlignment
/// \param column
/// \param alignment
///
void LogModel::setColumnAlignment(int column, Qt::Alignment alignment)
{
    _columnAlignments.setAlignment(column, alignment);
    emit dataChanged(index(0, column), index(rowCount() - 1, column), {Qt::TextAlignmentRole});
}

///
/// \brief LogModel::visibleItems
/// \return
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
