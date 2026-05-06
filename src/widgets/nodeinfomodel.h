// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file nodeinfomodel.h
/// \brief Declares the selected node information model.
///

#pragma once

#include <QAbstractTableModel>
#include <QVector>

#include "nodeitem.h"

///
/// \brief Table model for selected OPC UA node information.
///
class NodeInfoModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit NodeInfoModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void setItems(const QVector<NodeInfoItem> &items);
    void clear();

    enum Column {
        ColLabel = 0,
        ColValue = 1,
        ColCount = 2
    };

private:
    QVector<NodeInfoItem> _items;
};
