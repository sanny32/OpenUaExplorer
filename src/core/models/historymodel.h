// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file historymodel.h
/// \brief Declares the OPC UA history table model.
///

#pragma once

#include <QAbstractTableModel>
#include <QVector>

#include "appsettings.h"
#include "columnalignmentstore.h"
#include "opcua/opcuatypes.h"

///
/// \brief Table model for OPC UA HistoryRead samples.
///
class HistoryModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs an empty history model.
    /// \param parent Owning QObject.
    ///
    explicit HistoryModel(QObject *parent = nullptr);

    ///
    /// \brief Returns the number of history rows.
    /// \param parent Parent index; non-root parents have no rows.
    /// \return Item count, or 0 for non-root parents.
    ///
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    ///
    /// \brief Returns the fixed column count.
    /// \param parent Parent index; non-root parents have no columns.
    /// \return Column count, or 0 for non-root parents.
    ///
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    ///
    /// \brief Returns the cell text and column alignment for a history row.
    /// \param index Cell to query.
    /// \param role Requested data role.
    /// \return Value for the role, or an invalid variant.
    ///
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    ///
    /// \brief Returns the column titles.
    /// \param section Column index.
    /// \param orientation Header orientation.
    /// \param role Display role.
    /// \return Column title, or the base implementation otherwise.
    ///
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    ///
    /// \brief Replaces all history rows.
    /// \param items New history samples to display.
    ///
    void setItems(const QVector<OpcUaHistoryValue> &items);

    ///
    /// \brief Appends history rows, preserving the existing ones.
    /// \param items History samples to add.
    ///
    void append(const QVector<OpcUaHistoryValue> &items);

    ///
    /// \brief Removes all history rows.
    ///
    void clear();

    ///
    /// \brief Sets the text alignment for a column.
    /// \param column Column index.
    /// \param alignment Alignment to apply.
    ///
    void setColumnAlignment(int column, Qt::Alignment alignment);

    ///
    /// \brief Applies the timestamp display mode used to render the timestamp columns.
    /// \param mode Local time or UTC.
    ///
    void setTimestampMode(AppSettings::TimestampMode mode);

    ///
    /// \brief Exports the visible history table data as CSV text.
    /// \return CSV document with a header row.
    ///
    QString toCsv() const;

    ///
    /// \brief Columns exposed by the history table.
    ///
    enum Column {
        ColNumber          = 0,
        ColSourceTimestamp = 1,
        ColServerTimestamp = 2,
        ColValue           = 3,
        ColStatus          = 4,
        ColCount           = 5
    };

private:
    QVector<OpcUaHistoryValue>     _items;
    ColumnAlignmentStore           _columnAlignments;
    AppSettings::TimestampMode     _timestampMode;
};
