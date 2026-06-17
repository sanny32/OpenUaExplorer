// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file historymodel.h
/// \brief Declares the OPC UA history table model.
///

#pragma once

#include <QAbstractTableModel>
#include <QVector>

#include "columnalignmentstore.h"
#include "subscriptionitem.h"

///
/// \brief Table model for OPC UA history read entries.
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
    /// \brief Returns the node/range text and column alignment for a history row.
    /// \param index Cell to query.
    /// \param role Requested data role.
    /// \return Value for the role, or an invalid variant.
    ///
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    ///
    /// \brief Returns the Node/Range column titles.
    /// \param section Column index.
    /// \param orientation Header orientation.
    /// \param role Display role.
    /// \return Column title, or the base implementation otherwise.
    ///
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    ///
    /// \brief Replaces all history rows.
    /// \param items New history entries to display.
    ///
    void setItems(const QVector<HistoryItem> &items);

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
    /// \brief Columns exposed by the history table.
    ///
    enum Column {
        ColNode  = 0,
        ColRange = 1,
        ColCount = 2
    };

private:
    QVector<HistoryItem>       _items;
    ColumnAlignmentStore _columnAlignments;
};
