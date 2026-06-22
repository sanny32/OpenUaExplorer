// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file publishingintervaldelegate.h
/// \brief Declares the publishing-interval spin-box item delegate.
///

#pragma once

#include <QStyledItemDelegate>

///
/// \brief Spin-box delegate used to edit a subscription's publishing interval in milliseconds.
///
class PublishingIntervalDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs the delegate.
    /// \param parent Owning QObject.
    ///
    explicit PublishingIntervalDelegate(QObject *parent = nullptr);

    ///
    /// \brief Creates a spin-box editor bounded to the valid publishing-interval range.
    /// \param parent Parent for the editor widget.
    /// \return The spin-box editor.
    ///
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;

    ///
    /// \brief Seeds the spin box with the item's current interval.
    /// \param editor Spin-box editor.
    /// \param index Model index being edited.
    ///
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;

    ///
    /// \brief Writes the spin box's value back to the model.
    /// \param editor Spin-box editor.
    /// \param model Model to update.
    /// \param index Model index being edited.
    ///
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;
};
