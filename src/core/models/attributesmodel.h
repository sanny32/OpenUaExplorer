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
#include <QPair>
#include <QString>
#include <QVector>

#include "appsettings.h"
#include "columnalignmentstore.h"
#include "opcua/opcuatypes.h"

///
/// \brief Tree model for selected OPC UA node attributes.
///
class AttributesModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs an empty attributes tree model.
    /// \param parent Parent object.
    ///
    explicit AttributesModel(QObject *parent = nullptr);

    ///
    /// \brief Destroys the model and its item tree.
    ///
    ~AttributesModel() override;

    ///
    /// \brief Returns the index for a child item.
    /// \param row Child row.
    /// \param column Model column.
    /// \param parent Parent index.
    /// \return Model index for the requested item.
    ///
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;

    ///
    /// \brief Returns the parent index of an item.
    /// \param child Child index.
    /// \return Parent index or an invalid index for top-level items.
    ///
    QModelIndex parent(const QModelIndex &child) const override;

    ///
    /// \brief Returns the number of child rows.
    /// \param parent Parent index.
    /// \return Number of child rows.
    ///
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    ///
    /// \brief Returns the fixed column count.
    /// \param parent Parent index.
    /// \return Number of columns.
    ///
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    ///
    /// \brief Returns attribute/value text, alignment, and status colour for a cell.
    /// \param index Model index.
    /// \param role Data role.
    /// \return Item data.
    ///
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    ///
    /// \brief Returns the Attribute/Value column titles.
    /// \param section Header section.
    /// \param orientation Header orientation.
    /// \param role Data role.
    /// \return Header value.
    ///
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    ///
    /// \brief Re-emits the header titles after a UI language change.
    ///
    void retranslate();

    ///
    /// \brief Rebuilds the tree from structured OPC UA attributes.
    /// \param attributes Structured OPC UA attributes.
    ///
    void setAttributes(const QVector<OpcUaNodeAttribute> &attributes);

    ///
    /// \brief Builds a flat attribute tree from name/value pairs.
    /// \param items Flat attribute rows used by UI sample data.
    ///
    void setItems(const QVector<QPair<QString, QString>> &items);

    ///
    /// \brief Empties the attribute tree.
    ///
    void clear();

    ///
    /// \brief Sets the text alignment for a column and refreshes it.
    /// \param column Model column.
    /// \param alignment Text alignment.
    ///
    void setColumnAlignment(int column, Qt::Alignment alignment);

public slots:
    ///
    /// \brief Sets the timestamp display mode and reformats timestamp rows in place.
    /// \param mode Local time or UTC.
    ///
    void setTimestampMode(AppSettings::TimestampMode mode);

public:

    ///
    /// \brief Columns exposed by the attributes tree.
    ///
    enum Column {
        ColAttribute = 0,
        ColValue     = 1,
        ColCount     = 2
    };

private:
    struct Item;

    void appendAttribute(Item *parent, const OpcUaNodeAttribute &attribute);
    Item *itemForIndex(const QModelIndex &index) const;
    QString timestampValue(const Item &item) const;
    void refreshTimestamps(const QModelIndex &parentIndex);

    std::unique_ptr<Item> _root;
    ColumnAlignmentStore _columnAlignments;
    AppSettings::TimestampMode _timestampMode;
};
