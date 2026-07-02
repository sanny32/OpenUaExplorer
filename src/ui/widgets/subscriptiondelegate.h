// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file subscriptiondelegate.h
/// \brief Declares the subscription selection item delegate.
///

#pragma once

#include <QPersistentModelIndex>
#include <QVector>
#include <QStyledItemDelegate>

#include "models/subscriptionitem.h"

///
/// \brief Combo-box delegate used to assign subscriptions to monitored items.
///
class SubscriptionDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs the delegate with the set of selectable subscriptions.
    /// \param subscriptions Subscriptions offered in the editor combo box.
    /// \param parent Owning QObject.
    ///
    explicit SubscriptionDelegate(QVector<SubscriptionItem> subscriptions, QObject *parent = nullptr);

    ///
    /// \brief Replaces the selectable subscriptions offered by future editors.
    /// \param subscriptions Subscriptions to offer.
    ///
    void setSubscriptions(QVector<SubscriptionItem> subscriptions);

    ///
    /// \brief Returns the label of the entry that opens the new-subscription dialog.
    /// \return Create-new entry label, also used as the cell's pending placeholder.
    ///
    static QString createNewLabel();

    ///
    /// \brief Renders a cell's stored subscription name with its publishing interval.
    /// \param value Stored subscription name.
    /// \param locale Active locale.
    /// \return Label formatted as "name (interval ms)", or the raw value when unmatched.
    ///
    QString displayText(const QVariant &value, const QLocale &locale) const override;

    ///
    /// \brief Creates a combo-box editor listing the subscription names plus an empty choice.
    /// \param parent Parent for the editor widget.
    /// \return The combo-box editor.
    ///
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;

    ///
    /// \brief Selects the combo entry matching the item's current value.
    /// \param editor Combo-box editor.
    /// \param index Model index being edited.
    ///
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;

    ///
    /// \brief Writes the combo's selected subscription back to the model.
    /// \param editor Combo-box editor.
    /// \param model Model to update.
    /// \param index Model index being edited.
    ///
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;

signals:
    ///
    /// \brief Emitted when the user picks a different subscription for an item.
    /// \param index Model index that was edited.
    /// \param subscriptionName Chosen subscription name, or empty to unsubscribe.
    ///
    void subscriptionChanged(const QModelIndex &index, const QString &subscriptionName) const;

    ///
    /// \brief Emitted when the user chooses the "create new subscription" entry.
    /// \param index Model index being edited.
    ///
    void newSubscriptionRequested(const QModelIndex &index) const;

private slots:
    void commitAndCloseEditor();

private:
    QVector<SubscriptionItem>     _subscriptions;
    mutable QPersistentModelIndex _editingIndex;
};
