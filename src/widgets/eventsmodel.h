#pragma once

#include <QAbstractTableModel>
#include <QHash>
#include <QVector>

#include "subscriptionitem.h"

class EventsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit EventsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    void setItems(const QVector<EventItem> &items);
    void clear();

    void setColumnAlignment(int column, Qt::Alignment alignment);

    enum Column {
        ColTime    = 0,
        ColMessage = 1,
        ColCount   = 2
    };

private:
    QVector<EventItem>         _items;
    QHash<int, Qt::Alignment>  _columnAlignments;
};
