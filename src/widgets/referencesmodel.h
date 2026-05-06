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
    explicit ReferencesModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    void setItems(const QVector<ReferenceItem> &items);
    void clear();

    enum Column {
        ColReference = 0,
        ColTarget    = 1,
        ColCount     = 2
    };

private:
    QVector<ReferenceItem> _items;
};
