// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file subscriptioncombobox.cpp
/// \brief Implements the subscription selection combo box.
///

#include <QSignalBlocker>

#include "dialogs/newsubscriptiondialog.h"
#include "subscriptioncombobox.h"

namespace {

/// \brief Item-data role flagging the entry that opens the new-subscription dialog.
constexpr int kCreateNewRole = Qt::UserRole + 1;

/// \brief Default publishing interval used when no subscription is resolved.
constexpr double kDefaultIntervalMs = 1000.0;

///
/// \brief Returns the label of the entry that opens the new-subscription dialog.
/// \return Create-new entry label.
///
QString createNewLabel()
{
    return SubscriptionComboBox::tr("<New subscription>");
}

} // namespace

///
/// \brief Constructs the combo box with the create entry enabled.
/// \param parent Parent widget.
///
SubscriptionComboBox::SubscriptionComboBox(QWidget *parent)
    : QComboBox(parent)
{
    connect(this, &QComboBox::activated, this, &SubscriptionComboBox::handleActivated);
    rebuild();
}

///
/// \brief Replaces the listed subscriptions, preserving the current selection by name.
/// \param subscriptions Subscriptions in display order.
///
void SubscriptionComboBox::setSubscriptions(const QVector<SubscriptionItem> &subscriptions)
{
    _subscriptions = subscriptions;
    rebuild();
}

///
/// \brief Returns the currently listed subscriptions.
/// \return Subscriptions in display order.
///
QVector<SubscriptionItem> SubscriptionComboBox::subscriptions() const
{
    return _subscriptions;
}

///
/// \brief Returns the selected subscription name, or empty for the none entry.
/// \return Selected subscription name.
///
QString SubscriptionComboBox::currentSubscription() const
{
    if (itemData(currentIndex(), kCreateNewRole).toBool())
        return _selectedName;
    return currentData().toString();
}

///
/// \brief Selects the entry for a subscription name, defaulting to the first entry.
/// \param name Subscription name to select.
///
void SubscriptionComboBox::setCurrentSubscription(const QString &name)
{
    _selectedName = name;
    const int index = findData(name);
    setCurrentIndex(index >= 0 ? index : 0);
    _selectedName = currentData().toString();
    _previousIndex = currentIndex();
}

///
/// \brief Returns the publishing interval of the selected subscription.
/// \return Publishing interval in milliseconds, or the default when none is selected.
///
double SubscriptionComboBox::currentInterval() const
{
    return intervalForSubscription(currentSubscription());
}

///
/// \brief Shows or hides the leading empty ("none") choice.
/// \param visible True to offer an empty selection.
///
void SubscriptionComboBox::setNoneEntryVisible(bool visible)
{
    if (_noneEntry == visible)
        return;
    _noneEntry = visible;
    rebuild();
}

///
/// \brief Shows or hides the trailing "New subscription" entry.
/// \param visible True to offer creating a new subscription.
///
void SubscriptionComboBox::setCreateEntryVisible(bool visible)
{
    if (_createEntry == visible)
        return;
    _createEntry = visible;
    rebuild();
}

///
/// \brief Rebuilds the entries and restores the selection by name.
///
void SubscriptionComboBox::rebuild()
{
    QSignalBlocker blocker(this);
    clear();

    if (_noneEntry)
        addItem(QStringLiteral("—"), QString());
    for (const SubscriptionItem &item : _subscriptions)
        addItem(item.label(), item.name);
    if (_createEntry) {
        insertSeparator(count());
        addItem(createNewLabel());
        setItemData(count() - 1, true, kCreateNewRole);
    }

    int index = findData(_selectedName);
    if (index < 0)
        index = 0;
    setCurrentIndex(index);
    _selectedName = currentData().toString();
    _previousIndex = currentIndex();
}

///
/// \brief Opens the new-subscription dialog for the create entry, else records the choice.
/// \param index Activated entry index.
///
void SubscriptionComboBox::handleActivated(int index)
{
    if (itemData(index, kCreateNewRole).toBool()) {
        promptNewSubscription();
        return;
    }
    _previousIndex = index;
    _selectedName = itemData(index).toString();
    emit subscriptionSelected(_selectedName);
}

///
/// \brief Prompts for a new subscription, selects it and requests its creation.
///
void SubscriptionComboBox::promptNewSubscription()
{
    QStringList existing;
    existing.reserve(_subscriptions.size());
    for (const SubscriptionItem &item : _subscriptions)
        existing.append(item.name);

    NewSubscriptionDialog dialog(existing, window());
    if (dialog.exec() != QDialog::Accepted) {
        setCurrentIndex(_previousIndex);
        return;
    }

    SubscriptionItem item;
    item.name = dialog.subscriptionName();
    item.publishingInterval = dialog.publishingInterval();
    _subscriptions.append(item);
    _selectedName = item.name;
    rebuild();

    emit subscriptionCreationRequested(item.name, item.publishingInterval);
    emit subscriptionSelected(item.name);
}

///
/// \brief Resolves the publishing interval of a named subscription.
/// \param name Subscription name.
/// \return Publishing interval in milliseconds, or the default when unresolved.
///
double SubscriptionComboBox::intervalForSubscription(const QString &name) const
{
    for (const SubscriptionItem &item : _subscriptions) {
        if (item.name == name)
            return item.publishingInterval;
    }
    return kDefaultIntervalMs;
}
