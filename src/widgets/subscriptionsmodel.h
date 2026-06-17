// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file subscriptionsmodel.h
/// \brief Declares the subscriptions table model.
///

#pragma once

#include <QAbstractTableModel>
#include <QVector>

#include "columnalignmentstore.h"
#include "subscriptionitem.h"

///
/// \brief Table model for configured OPC UA subscriptions.
///
class SubscriptionsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs an empty subscriptions model.
    /// \param parent Owning QObject.
    ///
    explicit SubscriptionsModel(QObject *parent = nullptr);

    ///
    /// \brief Returns the number of subscription rows.
    /// \param parent Parent index; non-root parents have no rows.
    /// \return Subscription count, or 0 for non-root parents.
    ///
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    ///
    /// \brief Returns the fixed column count.
    /// \param parent Parent index; non-root parents have no columns.
    /// \return Column count, or 0 for non-root parents.
    ///
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    ///
    /// \brief Returns the name/interval text and column alignment for a subscription row.
    /// \param index Cell to query.
    /// \param role Requested data role.
    /// \return Value for the role, or an invalid variant.
    ///
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    ///
    /// \brief Returns the Name/Publishing Interval column titles.
    /// \param section Column index.
    /// \param orientation Header orientation.
    /// \param role Display role.
    /// \return Column title, or the base implementation otherwise.
    ///
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    ///
    /// \brief Replaces all subscription rows with row-change notifications.
    /// \param items New subscriptions to display.
    ///
    void setItems(const QVector<SubscriptionItem> &items);

    ///
    /// \brief Removes all subscriptions.
    ///
    void clear();

    ///
    /// \brief Returns the names of all subscriptions.
    /// \return Subscription names in row order.
    ///
    QStringList names() const;

    ///
    /// \brief Sets the text alignment for a column.
    /// \param column Column index.
    /// \param alignment Alignment to apply.
    ///
    void setColumnAlignment(int column, Qt::Alignment alignment);

    ///
    /// \brief Columns exposed by the subscriptions table.
    ///
    enum Column {
        ColName              = 0,
        ColPublishingInterval = 1,
        ColCount             = 2
    };

private:
    QVector<SubscriptionItem>  _items;
    ColumnAlignmentStore _columnAlignments;
};
