// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file subscriptiondelegate.cpp
/// \brief Implements the subscription selection item delegate.
///

#include <QAbstractItemView>
#include <QComboBox>
#include <QFontMetrics>
#include <QStyle>

#include "subscriptiondelegate.h"

///
/// \brief Constructs the delegate with the set of selectable subscription names.
/// \param subscriptionNames Names offered in the editor combo box.
/// \param parent Owning QObject.
///
SubscriptionDelegate::SubscriptionDelegate(QStringList subscriptionNames, QObject *parent)
    : QStyledItemDelegate(parent)
    , _subscriptionNames(std::move(subscriptionNames))
{
}

///
/// \brief Creates a combo-box editor listing the subscription names plus an empty choice.
/// \param parent Parent for the editor widget.
/// \return The combo-box editor.
///
QWidget *SubscriptionDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &,
                                            const QModelIndex &) const
{
    QComboBox *combo = new QComboBox(parent);
    combo->addItem(QStringLiteral("—"), QString());
    for (const QString &name : _subscriptionNames)
        combo->addItem(name, name);

    const QFontMetrics metrics(combo->font());
    int contentWidth = 0;
    for (int i = 0; i < combo->count(); ++i)
        contentWidth = qMax(contentWidth, metrics.horizontalAdvance(combo->itemText(i)));

    const QMargins margins = combo->view()->contentsMargins();
    const int popupWidth = contentWidth + margins.left() + margins.right()
                           + combo->style()->pixelMetric(QStyle::PM_ScrollBarExtent)
                           + 24;
    combo->view()->setMinimumWidth(popupWidth);

    return combo;
}

///
/// \brief Selects the combo entry matching the item's current value.
/// \param editor Combo-box editor.
/// \param index Model index being edited.
///
void SubscriptionDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QComboBox *combo = static_cast<QComboBox *>(editor);
    const QString current = index.data(Qt::EditRole).toString();
    const int i = combo->findData(current);
    combo->setCurrentIndex(i >= 0 ? i : 0);
}

///
/// \brief Writes the combo's selected subscription back to the model.
/// \param editor Combo-box editor.
/// \param model Model to update.
/// \param index Model index being edited.
///
void SubscriptionDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                        const QModelIndex &index) const
{
    QComboBox *combo = static_cast<QComboBox *>(editor);
    model->setData(index, combo->currentData(), Qt::EditRole);
}
