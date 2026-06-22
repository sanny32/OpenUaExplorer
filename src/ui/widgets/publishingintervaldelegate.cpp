// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file publishingintervaldelegate.cpp
/// \brief Implements the publishing-interval spin-box item delegate.
///

#include <QSpinBox>

#include "publishingintervaldelegate.h"

namespace {

/// \brief Smallest publishing interval offered by the editor, in milliseconds.
constexpr int minIntervalMs = 50;
/// \brief Largest publishing interval offered by the editor, in milliseconds.
constexpr int maxIntervalMs = 600000;
/// \brief Spin-box step between offered intervals, in milliseconds.
constexpr int intervalStepMs = 100;

}

///
/// \brief Constructs the delegate.
/// \param parent Owning QObject.
///
PublishingIntervalDelegate::PublishingIntervalDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

///
/// \brief Creates a spin-box editor bounded to the valid publishing-interval range.
/// \param parent Parent for the editor widget.
/// \return The spin-box editor.
///
QWidget *PublishingIntervalDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &,
                                                  const QModelIndex &) const
{
    QSpinBox *spin = new QSpinBox(parent);
    spin->setRange(minIntervalMs, maxIntervalMs);
    spin->setSingleStep(intervalStepMs);
    spin->setSuffix(QStringLiteral(" ms"));
    return spin;
}

///
/// \brief Seeds the spin box with the item's current interval.
/// \param editor Spin-box editor.
/// \param index Model index being edited.
///
void PublishingIntervalDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QSpinBox *spin = static_cast<QSpinBox *>(editor);
    spin->setValue(qRound(index.data(Qt::EditRole).toDouble()));
}

///
/// \brief Writes the spin box's value back to the model.
/// \param editor Spin-box editor.
/// \param model Model to update.
/// \param index Model index being edited.
///
void PublishingIntervalDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                              const QModelIndex &index) const
{
    QSpinBox *spin = static_cast<QSpinBox *>(editor);
    spin->interpretText();
    model->setData(index, static_cast<double>(spin->value()), Qt::EditRole);
}
