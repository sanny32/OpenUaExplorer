// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file nodeinfomodel.h
/// \brief Declares the selected node information model.
///

#pragma once

#include <QAbstractTableModel>
#include <QVector>

#include "nodeitem.h"

///
/// \brief Table model for selected OPC UA node information.
///
class NodeInfoModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs an empty node-info model.
    /// \param parent Owning QObject.
    ///
    explicit NodeInfoModel(QObject *parent = nullptr);

    ///
    /// \brief Returns the number of info rows.
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
    /// \brief Returns the label or value text for a cell.
    /// \param index Cell to query.
    /// \param role Only Qt::DisplayRole is handled.
    /// \return Cell text, or an invalid variant.
    ///
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    ///
    /// \brief Replaces the info rows.
    /// \param items New label/value rows to display.
    ///
    void setItems(const QVector<NodeInfoItem> &items);

    ///
    /// \brief Removes all rows.
    ///
    void clear();

    ///
    /// \brief Columns exposed by the node-info table.
    ///
    enum Column {
        ColLabel = 0,
        ColValue = 1,
        ColCount = 2
    };

private:
    QVector<NodeInfoItem> _items;
};
