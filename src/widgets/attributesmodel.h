#pragma once

#include <QAbstractTableModel>
#include <QHash>
#include <QPair>
#include <QString>
#include <QVector>

///
/// \brief Table model for selected OPC UA node attributes.
///
class AttributesModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit AttributesModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    void setItems(const QVector<QPair<QString, QString>> &items);
    void clear();

    void setColumnAlignment(int column, Qt::Alignment alignment);

    enum Column {
        ColAttribute = 0,
        ColValue     = 1,
        ColCount     = 2
    };

private:
    QVector<QPair<QString, QString>> _items;
    QHash<int, Qt::Alignment>        _columnAlignments;
};
