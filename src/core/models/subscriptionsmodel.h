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
    /// \brief Reports that the Name and Publishing Interval cells are editable.
    /// \param index Cell to query.
    /// \return Item flags including Qt::ItemIsEditable for both columns.
    ///
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    ///
    /// \brief Stores an edited subscription name or publishing interval.
    /// \param index Cell being edited.
    /// \param value New value.
    /// \param role Edit role; other roles are ignored.
    /// \return True when the value was accepted and stored.
    ///
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

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
    /// \brief Appends a subscription row with row-change notifications.
    /// \param item Subscription to add.
    /// \return Row index of the inserted subscription.
    ///
    int addSubscription(const SubscriptionItem &item);

    ///
    /// \brief Removes a single subscription row.
    /// \param row Row to remove.
    ///
    void removeRow(int row);

    ///
    /// \brief Returns the subscription at a row.
    /// \param row Row to read.
    /// \return Subscription, or a default-constructed item for invalid rows.
    ///
    SubscriptionItem itemAt(int row) const;

    ///
    /// \brief Reports whether a subscription name already exists.
    /// \param name Name to look for.
    /// \param exceptRow Row to ignore in the search, or -1 to check all rows.
    /// \return True when another row carries the name.
    ///
    bool containsName(const QString &name, int exceptRow = -1) const;

    ///
    /// \brief Returns the publishing interval of a named subscription.
    /// \param name Subscription name.
    /// \return Publishing interval in milliseconds, or 1000 when not found.
    ///
    double intervalFor(const QString &name) const;

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

signals:
    ///
    /// \brief Emitted when a subscription is renamed via inline editing.
    /// \param oldName Previous subscription name.
    /// \param newName New subscription name.
    ///
    void subscriptionRenamed(QString oldName, QString newName);

    ///
    /// \brief Emitted when a subscription's publishing interval changes.
    /// \param name Subscription name.
    /// \param interval New publishing interval in milliseconds.
    ///
    void subscriptionIntervalChanged(QString name, double interval);

private:
    QVector<SubscriptionItem>  _items;
    ColumnAlignmentStore _columnAlignments;
};
