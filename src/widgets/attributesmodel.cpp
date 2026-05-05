#include "attributesmodel.h"

///
/// \brief AttributesModel::AttributesModel
/// \param parent
///
AttributesModel::AttributesModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

///
/// \brief AttributesModel::rowCount
/// \param parent
/// \return
///
int AttributesModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return _items.size();
}

///
/// \brief AttributesModel::columnCount
/// \param parent
/// \return
///
int AttributesModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return ColCount;
}

///
/// \brief AttributesModel::headerData
/// \param section
/// \param orientation
/// \param role
/// \return
///
QVariant AttributesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QAbstractTableModel::headerData(section, orientation, role);

    switch (section) {
    case ColAttribute: return QStringLiteral("Attribute");
    case ColValue:     return QStringLiteral("Value");
    default:           return QVariant();
    }
}

///
/// \brief AttributesModel::data
/// \param index
/// \param role
/// \return
///
QVariant AttributesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    if (index.row() < 0 || index.row() >= _items.size()) return QVariant();

    const auto &item = _items.at(index.row());
    const int col = index.column();

    if (role == Qt::DisplayRole) {
        switch (col) {
        case ColAttribute: return item.first;
        case ColValue:     return item.second;
        default:           return QVariant();
        }
    }

    if (role == Qt::TextAlignmentRole)
        return QVariant(_columnAlignments.value(col, Qt::AlignLeft | Qt::AlignVCenter));

    return QVariant();
}

///
/// \brief AttributesModel::setItems
/// \param items
///
void AttributesModel::setItems(const QVector<QPair<QString, QString>> &items)
{
    beginResetModel();
    _items = items;
    endResetModel();
}

///
/// \brief AttributesModel::clear
///
void AttributesModel::clear()
{
    beginResetModel();
    _items.clear();
    endResetModel();
}

///
/// \brief AttributesModel::setColumnAlignment
/// \param column
/// \param alignment
///
void AttributesModel::setColumnAlignment(int column, Qt::Alignment alignment)
{
    _columnAlignments[column] = alignment;
    emit dataChanged(index(0, column), index(rowCount() - 1, column), {Qt::TextAlignmentRole});
}
