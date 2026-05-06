// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dataaccessmodel.h
/// \brief Declares the OPC UA data access table model.
///

#pragma once

#include <QAbstractTableModel>
#include <QHash>
#include <QStringList>
#include <QVector>

#include "dataaccessitem.h"

///
/// \brief Table model for OPC UA data access monitored items.
///
class DataAccessModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit DataAccessModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    QStringList subscriptionNames() const;

    Qt::Alignment columnAlignment(int column) const;
    void setColumnAlignment(int column, Qt::Alignment alignment);

    enum Column {
        ColNumber       = 0,
        ColNodeId       = 1,
        ColDisplayName  = 2,
        ColValue        = 3,
        ColDataType     = 4,
        ColTimestamp    = 5,
        ColStatus       = 6,
        ColSubscription = 7,
        ColCount        = 8
    };

private:
    QVector<DataAccessItem> _items;
    QStringList _subscriptionNames;
    QHash<int, Qt::Alignment> _columnAlignments;
};
