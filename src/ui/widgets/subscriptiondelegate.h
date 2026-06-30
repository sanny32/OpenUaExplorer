// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file subscriptiondelegate.h
/// \brief Declares the subscription selection item delegate.
///

#pragma once

#include <QStyledItemDelegate>
#include <QStringList>

///
/// \brief Combo-box delegate used to assign subscriptions to monitored items.
///
class SubscriptionDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs the delegate with the set of selectable subscription names.
    /// \param subscriptionNames Names offered in the editor combo box.
    /// \param parent Owning QObject.
    ///
    explicit SubscriptionDelegate(QStringList subscriptionNames, QObject *parent = nullptr);

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

private:
    QStringList _subscriptionNames;
};
