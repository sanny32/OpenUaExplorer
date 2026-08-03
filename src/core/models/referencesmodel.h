// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file referencesmodel.h
/// \brief Declares the selected node references model.
///

#pragma once

#include <QAbstractTableModel>
#include <QVector>

#include "nodeitem.h"

///
/// \brief Table model for references of the selected OPC UA node.
///
class ReferencesModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs an empty references model.
    /// \param parent Owning QObject.
    ///
    explicit ReferencesModel(QObject *parent = nullptr);

    ///
    /// \brief Returns the number of reference rows.
    /// \param parent Parent index; non-root parents have no rows.
    /// \return Reference count, or 0 for non-root parents.
    ///
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    ///
    /// \brief Returns the fixed column count.
    /// \param parent Parent index; non-root parents have no columns.
    /// \return Column count, or 0 for non-root parents.
    ///
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    ///
    /// \brief Returns the reference or target text for a cell.
    /// \param index Cell to query.
    /// \param role Only Qt::DisplayRole is handled.
    /// \return Cell text, or an invalid variant.
    ///
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    ///
    /// \brief Returns the Reference/Target column titles.
    /// \param section Column index.
    /// \param orientation Header orientation.
    /// \param role Display role.
    /// \return Column title, or the base implementation otherwise.
    ///
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    ///
    /// \brief Re-emits the header titles after a UI language change.
    ///
    void retranslate();

    ///
    /// \brief Replaces the reference rows.
    /// \param items New references to display.
    ///
    void setItems(const QVector<ReferenceItem> &items);

    ///
    /// \brief Removes all references.
    ///
    void clear();

    ///
    /// \brief Columns exposed by the references table.
    ///
    enum Column {
        ColReference = 0,
        ColTarget    = 1,
        ColCount     = 2
    };

private:
    QVector<ReferenceItem> _items;
};
