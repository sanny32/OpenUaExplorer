// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file historymodel.cpp
/// \brief Implements the OPC UA history table model.
///

#include "historymodel.h"

#include <QStringList>

#include "formatters/attributeformatter.h"

namespace {

///
/// \brief Escapes a single CSV field.
/// \param value Field text.
/// \return Escaped CSV field text.
///
QString csvField(QString value)
{
    const bool quote = value.contains(QLatin1Char(','))
        || value.contains(QLatin1Char('"'))
        || value.contains(QLatin1Char('\n'))
        || value.contains(QLatin1Char('\r'));
    if (!quote)
        return value;
    value.replace(QStringLiteral("\""), QStringLiteral("\"\""));
    return QStringLiteral("\"%1\"").arg(value);
}

OpcUaFormat::TimestampMode toFormatMode(AppSettings::TimestampMode mode)
{
    return mode == AppSettings::TimestampMode::Utc
        ? OpcUaFormat::TimestampMode::Utc
        : OpcUaFormat::TimestampMode::LocalTime;
}
}

///
/// \brief Constructs an empty history model.
/// \param parent Owning QObject.
///
HistoryModel::HistoryModel(QObject *parent)
    : QAbstractTableModel(parent)
    , _timestampMode(AppSettings().timestampMode())
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
/// \brief Returns the column titles.
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
    case ColNumber:          return QStringLiteral("#");
    case ColSourceTimestamp: return QStringLiteral("Source Timestamp");
    case ColServerTimestamp: return QStringLiteral("Server Timestamp");
    case ColValue:           return QStringLiteral("Value");
    case ColStatus:          return QStringLiteral("Status");
    default:                 return QVariant();
    }
}

///
/// \brief Returns the cell text and column alignment for a history row.
/// \param index Cell to query.
/// \param role Requested data role.
/// \return Value for the role, or an invalid variant.
///
QVariant HistoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    if (index.row() < 0 || index.row() >= _items.size()) return QVariant();
    const OpcUaHistoryValue &item = _items.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ColNumber:          return index.row() + 1;
        case ColSourceTimestamp: return OpcUaFormat::isoTimestampWithZone(
                                            item.sourceTimestamp, toFormatMode(_timestampMode));
        case ColServerTimestamp: return OpcUaFormat::isoTimestampWithZone(
                                            item.serverTimestamp, toFormatMode(_timestampMode));
        case ColValue:           return OpcUaFormat::displayValue(item.value);
        case ColStatus:          return item.status;
        default:                 return QVariant();
        }
    }

    if (role == Qt::TextAlignmentRole)
        return QVariant(_columnAlignments.alignment(index.column()));

    return QVariant();
}

///
/// \brief Replaces all history rows.
/// \param items New history samples to display.
///
void HistoryModel::setItems(const QVector<OpcUaHistoryValue> &items)
{
    beginResetModel();
    _items = items;
    endResetModel();
}

///
/// \brief Appends history rows, preserving the existing ones.
/// \param items History samples to add.
///
void HistoryModel::append(const QVector<OpcUaHistoryValue> &items)
{
    if (items.isEmpty())
        return;
    const int first = _items.size();
    beginInsertRows({}, first, first + items.size() - 1);
    _items += items;
    endInsertRows();
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

///
/// \brief Applies the timestamp display mode used to render the timestamp columns.
/// \param mode Local time or UTC.
///
void HistoryModel::setTimestampMode(AppSettings::TimestampMode mode)
{
    if (_timestampMode == mode)
        return;
    _timestampMode = mode;
    if (_items.isEmpty())
        return;
    emit dataChanged(index(0, ColSourceTimestamp), index(rowCount() - 1, ColServerTimestamp),
                     {Qt::DisplayRole});
}

///
/// \brief Exports the visible history table data as CSV text.
/// \return CSV document with a header row.
///
QString HistoryModel::toCsv() const
{
    QStringList lines;
    QStringList header;
    for (int column = 0; column < ColCount; ++column)
        header.append(csvField(headerData(column, Qt::Horizontal).toString()));
    lines.append(header.join(QLatin1Char(',')));

    for (int row = 0; row < rowCount(); ++row) {
        QStringList fields;
        for (int column = 0; column < ColCount; ++column)
            fields.append(csvField(data(index(row, column)).toString()));
        lines.append(fields.join(QLatin1Char(',')));
    }

    return lines.join(QLatin1Char('\n')) + QLatin1Char('\n');
}
