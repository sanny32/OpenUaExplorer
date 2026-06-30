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

namespace {

/// \brief Item-data role flagging the combo entry that opens the new-subscription dialog.
constexpr int createNewRole = Qt::UserRole + 1;

}

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
/// \brief Replaces the selectable subscription names offered by future editors.
/// \param subscriptionNames Names to offer.
///
void SubscriptionDelegate::setSubscriptionNames(QStringList subscriptionNames)
{
    _subscriptionNames = std::move(subscriptionNames);
}

///
/// \brief Returns the label of the entry that opens the new-subscription dialog.
/// \return Create-new entry label, also used as the cell's pending placeholder.
///
QString SubscriptionDelegate::createNewLabel()
{
    return tr("<New subscription>");
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

    combo->insertSeparator(combo->count());
    combo->addItem(createNewLabel());
    combo->setItemData(combo->count() - 1, true, createNewRole);

    connect(combo, QOverload<int>::of(&QComboBox::activated),
            this, &SubscriptionDelegate::commitAndCloseEditor);

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
    _editingIndex = index;
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
    if (combo->itemData(combo->currentIndex(), createNewRole).toBool())
        return;
    const QString newName = combo->currentData().toString();
    if (newName == index.data(Qt::EditRole).toString())
        return;
    model->setData(index, newName, Qt::EditRole);
    emit subscriptionChanged(index, newName);
}

///
/// \brief Commits and closes the editor as soon as the user picks a combo entry.
///
/// The "create new subscription" entry closes the editor without committing its value, so
/// the receiver can drive the cell while no editor is open to overwrite it.
///
void SubscriptionDelegate::commitAndCloseEditor()
{
    QComboBox *combo = qobject_cast<QComboBox *>(sender());
    if (!combo)
        return;
    if (combo->itemData(combo->currentIndex(), createNewRole).toBool()) {
        emit newSubscriptionRequested(_editingIndex);
        emit closeEditor(combo);
        return;
    }
    emit commitData(combo);
    emit closeEditor(combo);
}
