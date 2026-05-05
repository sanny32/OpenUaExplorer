#pragma once

#include <QAbstractTableModel>
#include <QHash>
#include <QVector>

#include "subscriptionitem.h"

class SubscriptionsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit SubscriptionsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    void setItems(const QVector<SubscriptionItem> &items);
    void clear();

    void setColumnAlignment(int column, Qt::Alignment alignment);

    enum Column {
        ColName              = 0,
        ColPublishingInterval = 1,
        ColCount             = 2
    };

private:
    QVector<SubscriptionItem>  _items;
    QHash<int, Qt::Alignment>  _columnAlignments;
};
