// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file referencesmodel.cpp
/// \brief Implements the selected node references model.
///

#include "referencesmodel.h"

///
/// \brief ReferencesModel::ReferencesModel
/// \param parent
///
ReferencesModel::ReferencesModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

///
/// \brief ReferencesModel::rowCount
/// \param parent
/// \return
///
int ReferencesModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return _items.size();
}

///
/// \brief ReferencesModel::columnCount
/// \param parent
/// \return
///
int ReferencesModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return ColCount;
}

///
/// \brief ReferencesModel::headerData
/// \param section
/// \param orientation
/// \param role
/// \return
///
QVariant ReferencesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QAbstractTableModel::headerData(section, orientation, role);

    switch (section) {
    case ColReference: return QStringLiteral("Reference");
    case ColTarget:    return QStringLiteral("Target");
    default:           return QVariant();
    }
}

///
/// \brief ReferencesModel::data
/// \param index
/// \param role
/// \return
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
/// \brief ReferencesModel::setItems
/// \param items
///
void ReferencesModel::setItems(const QVector<ReferenceItem> &items)
{
    beginResetModel();
    _items = items;
    endResetModel();
}

///
/// \brief ReferencesModel::clear
///
void ReferencesModel::clear()
{
    beginResetModel();
    _items.clear();
    endResetModel();
}
