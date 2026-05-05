#pragma once

#include <QAbstractTableModel>
#include <QHash>
#include <QVector>

#include "subscriptionitem.h"

class HistoryModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit HistoryModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    void setItems(const QVector<HistoryItem> &items);
    void clear();

    void setColumnAlignment(int column, Qt::Alignment alignment);

    enum Column {
        ColNode  = 0,
        ColRange = 1,
        ColCount = 2
    };

private:
    QVector<HistoryItem>       _items;
    QHash<int, Qt::Alignment>  _columnAlignments;
};
