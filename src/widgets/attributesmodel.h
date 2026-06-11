// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file attributesmodel.h
/// \brief Declares the selected node attributes tree model.
///

#pragma once

#include <memory>
#include <vector>

#include <QAbstractItemModel>
#include <QHash>
#include <QPair>
#include <QString>
#include <QVector>

#include "opcua/opcuatypes.h"

///
/// \brief Tree model for selected OPC UA node attributes.
///
class AttributesModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit AttributesModel(QObject *parent = nullptr);
    ~AttributesModel() override;

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    void setAttributes(const QVector<OpcUaNodeAttribute> &attributes);
    void setItems(const QVector<QPair<QString, QString>> &items);
    void clear();

    void setColumnAlignment(int column, Qt::Alignment alignment);

    enum Column {
        ColAttribute = 0,
        ColValue     = 1,
        ColCount     = 2
    };

private:
    struct Item;

    void appendAttribute(Item *parent, const OpcUaNodeAttribute &attribute);
    Item *itemForIndex(const QModelIndex &index) const;

    std::unique_ptr<Item> _root;
    QHash<int, Qt::Alignment> _columnAlignments;
};
