#include "historymodel.h"

///
/// \brief HistoryModel::HistoryModel
/// \param parent
///
HistoryModel::HistoryModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

///
/// \brief HistoryModel::rowCount
/// \param parent
/// \return
///
int HistoryModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return _items.size();
}

///
/// \brief HistoryModel::columnCount
/// \param parent
/// \return
///
int HistoryModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return ColCount;
}

///
/// \brief HistoryModel::headerData
/// \param section
/// \param orientation
/// \param role
/// \return
///
QVariant HistoryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QAbstractTableModel::headerData(section, orientation, role);

    switch (section) {
    case ColNode:  return QStringLiteral("Node");
    case ColRange: return QStringLiteral("Range");
    default:       return QVariant();
    }
}

///
/// \brief HistoryModel::data
/// \param index
/// \param role
/// \return
///
QVariant HistoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    if (index.row() < 0 || index.row() >= _items.size()) return QVariant();
    const HistoryItem &item = _items.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ColNode:  return item.node;
        case ColRange: return item.range;
        default:       return QVariant();
        }
    }

    if (role == Qt::TextAlignmentRole)
        return QVariant(_columnAlignments.value(index.column(), Qt::AlignLeft | Qt::AlignVCenter));

    return QVariant();
}

///
/// \brief HistoryModel::setItems
/// \param items
///
void HistoryModel::setItems(const QVector<HistoryItem> &items)
{
    beginResetModel();
    _items = items;
    endResetModel();
}

///
/// \brief HistoryModel::clear
///
void HistoryModel::clear()
{
    beginResetModel();
    _items.clear();
    endResetModel();
}

///
/// \brief HistoryModel::setColumnAlignment
/// \param column
/// \param alignment
///
void HistoryModel::setColumnAlignment(int column, Qt::Alignment alignment)
{
    _columnAlignments[column] = alignment;
    emit dataChanged(index(0, column), index(rowCount() - 1, column), {Qt::TextAlignmentRole});
}
