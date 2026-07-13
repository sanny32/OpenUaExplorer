// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file csvexporter.cpp
/// \brief Implements CSV export helpers for Qt item models.
///

#include "csvexporter.h"

#include <QAbstractItemModel>
#include <QStringList>

namespace {
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
}

///
/// \brief Exports the display text of a table model with a horizontal header.
/// \param model Model to export.
/// \return CSV document with a header row.
///
QString CsvExporter::tableToCsv(const QAbstractItemModel &model)
{
    QStringList lines;
    QStringList header;
    for (int column = 0; column < model.columnCount(); ++column) {
        header.append(csvField(model.headerData(column, Qt::Horizontal).toString()));
    }
    lines.append(header.join(QLatin1Char(',')));

    for (int row = 0; row < model.rowCount(); ++row) {
        QStringList fields;
        for (int column = 0; column < model.columnCount(); ++column) {
            fields.append(csvField(model.data(model.index(row, column), Qt::DisplayRole).toString()));
        }
        lines.append(fields.join(QLatin1Char(',')));
    }

    return lines.join(QLatin1Char('\n')) + QLatin1Char('\n');
}
