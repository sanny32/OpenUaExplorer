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
    explicit SubscriptionDelegate(QStringList subscriptionNames, QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;

private:
    QStringList _subscriptionNames;
};
