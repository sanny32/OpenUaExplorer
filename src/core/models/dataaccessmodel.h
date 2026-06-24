// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dataaccessmodel.h
/// \brief Declares the OPC UA data access table model.
///

#pragma once

#include <QAbstractTableModel>
#include <QStringList>
#include <QVector>

#include "appsettings.h"
#include "columnalignmentstore.h"
#include "dataaccessitem.h"
#include "opcua/opcuatypes.h"

///
/// \brief Table model for OPC UA data access monitored items.
///
class DataAccessModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs an empty data-access model.
    /// \param parent Owning QObject.
    ///
    explicit DataAccessModel(QObject *parent = nullptr);

    ///
    /// \brief Replaces all rows.
    /// \param items New data-access rows.
    ///
    void setItems(const QVector<DataAccessItem> &items);

    ///
    /// \brief Updates the row matching the node, or appends a new row when absent.
    /// \param details Node details to add or update.
    ///
    void addOrUpdate(const OpcUaNodeDetails &details);

    ///
    /// \brief Refreshes the value, status, and timestamps of rows matching the read results.
    /// \param values Read results.
    ///
    void updateValues(const QVector<OpcUaDataValue> &values);

    ///
    /// \brief Removes the rows referenced by the given indexes, highest row first.
    /// \param rows Selected model rows.
    ///
    void removeRows(const QModelIndexList &rows);

    ///
    /// \brief Collects the NodeIds of the given rows, or of every row when none are given.
    /// \param rows Optional selected rows.
    /// \return NodeIds for selected rows or all rows.
    ///
    QStringList nodeIds(const QModelIndexList &rows = {}) const;

    ///
    /// \brief Returns the item at a row.
    /// \param row Model row.
    /// \return Data item or an empty item.
    ///
    DataAccessItem itemAt(int row) const;

    ///
    /// \brief Removes all rows.
    ///
    void clear();

    ///
    /// \brief Returns the number of rows.
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
    /// \brief Returns cell text, alignment, and status/subscription colours for a row.
    /// \param index Cell to query.
    /// \param role Requested data role.
    /// \return Value for the role, or an invalid variant.
    ///
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    ///
    /// \brief Writes the edited subscription name into the Subscription column.
    /// \param index Cell being edited.
    /// \param value New subscription name.
    /// \param role Only Qt::EditRole is accepted.
    /// \return True when the value was applied.
    ///
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

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
    /// \brief Marks the Subscription column editable.
    /// \param index Cell to query.
    /// \return Item flags for the cell.
    ///
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    ///
    /// \brief Returns the text alignment for a column.
    /// \param column Column index.
    /// \return Column alignment.
    ///
    Qt::Alignment columnAlignment(int column) const;

    ///
    /// \brief Sets the text alignment for a column.
    /// \param column Column index.
    /// \param alignment Alignment to apply.
    ///
    void setColumnAlignment(int column, Qt::Alignment alignment);

public slots:
    ///
    /// \brief Sets the timestamp display mode and repaints the timestamp column.
    /// \param mode Local time or UTC.
    ///
    void setTimestampMode(AppSettings::TimestampMode mode);

public:

    ///
    /// \brief Columns exposed by the data-access table.
    ///
    enum Column {
        ColNumber       = 0,
        ColNodeId       = 1,
        ColDisplayName  = 2,
        ColValue        = 3,
        ColDataType     = 4,
        ColTimestamp    = 5,
        ColStatus       = 6,
        ColSubscription = 7,
        ColCount        = 8
    };

private:
    QVector<DataAccessItem> _items;
    ColumnAlignmentStore _columnAlignments;
    AppSettings::TimestampMode _timestampMode;
};
