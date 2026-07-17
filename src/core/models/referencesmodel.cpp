// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file referencesmodel.cpp
/// \brief Implements the selected node references model.
///

#include "referencesmodel.h"

///
/// \brief Constructs an empty references model.
/// \param parent Owning QObject.
///
ReferencesModel::ReferencesModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

///
/// \brief Returns the number of reference rows.
/// \param parent Parent index; non-root parents have no rows.
/// \return Reference count, or 0 for non-root parents.
///
int ReferencesModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return _items.size();
}

///
/// \brief Returns the fixed column count.
/// \param parent Parent index; non-root parents have no columns.
/// \return Column count, or 0 for non-root parents.
///
int ReferencesModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return ColCount;
}

///
/// \brief Returns the Reference/Target column titles.
/// \param section Column index.
/// \param orientation Header orientation.
/// \param role Display role.
/// \return Column title, or the base implementation otherwise.
///
QVariant ReferencesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QAbstractTableModel::headerData(section, orientation, role);

    switch (section) {
    case ColReference: return tr("Reference");
    case ColTarget:    return tr("Target");
    default:           return QVariant();
    }
}

///
/// \brief Returns the reference or target text for a cell.
/// \param index Cell to query.
/// \param role Only Qt::DisplayRole is handled.
/// \return Cell text, or an invalid variant.
///
QVariant ReferencesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    if (index.row() < 0 || index.row() >= _items.size()) return QVariant();

    if (role != Qt::DisplayRole) return QVariant();

    const ReferenceItem &item = _items.at(index.row());
    switch (index.column()) {
    case ColReference: return item.reference;
    case ColTarget:    return item.target;
    default:           return QVariant();
    }
}

///
/// \brief Replaces the reference rows.
/// \param items New references to display.
///
void ReferencesModel::setItems(const QVector<ReferenceItem> &items)
{
    beginResetModel();
    _items = items;
    endResetModel();
}

///
/// \brief Removes all references.
///
void ReferencesModel::clear()
{
    beginResetModel();
    _items.clear();
    endResetModel();
}

///
/// \brief Re-emits the header titles after a UI language change.
///
void ReferencesModel::retranslate()
{
    emit headerDataChanged(Qt::Horizontal, 0, columnCount(QModelIndex()) - 1);
}
