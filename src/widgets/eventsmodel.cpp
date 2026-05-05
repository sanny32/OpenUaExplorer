#include "eventsmodel.h"

///
/// \brief EventsModel::EventsModel
/// \param parent
///
EventsModel::EventsModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

///
/// \brief EventsModel::rowCount
/// \param parent
/// \return
///
int EventsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return _items.size();
}

///
/// \brief EventsModel::columnCount
/// \param parent
/// \return
///
int EventsModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return ColCount;
}

///
/// \brief EventsModel::headerData
/// \param section
/// \param orientation
/// \param role
/// \return
///
QVariant EventsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QAbstractTableModel::headerData(section, orientation, role);

    switch (section) {
    case ColTime:    return QStringLiteral("Time");
    case ColMessage: return QStringLiteral("Message");
    default:         return QVariant();
    }
}

///
/// \brief EventsModel::data
/// \param index
/// \param role
/// \return
///
QVariant EventsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    if (index.row() < 0 || index.row() >= _items.size()) return QVariant();
    const EventItem &item = _items.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ColTime:    return item.time;
        case ColMessage: return item.message;
        default:         return QVariant();
        }
    }

    if (role == Qt::TextAlignmentRole)
        return QVariant(_columnAlignments.value(index.column(), Qt::AlignLeft | Qt::AlignVCenter));

    return QVariant();
}

///
/// \brief EventsModel::setItems
/// \param items
///
void EventsModel::setItems(const QVector<EventItem> &items)
{
    beginResetModel();
    _items = items;
    endResetModel();
}

///
/// \brief EventsModel::clear
///
void EventsModel::clear()
{
    beginResetModel();
    _items.clear();
    endResetModel();
}

///
/// \brief EventsModel::setColumnAlignment
/// \param column
/// \param alignment
///
void EventsModel::setColumnAlignment(int column, Qt::Alignment alignment)
{
    _columnAlignments[column] = alignment;
    emit dataChanged(index(0, column), index(rowCount() - 1, column), {Qt::TextAlignmentRole});
}
