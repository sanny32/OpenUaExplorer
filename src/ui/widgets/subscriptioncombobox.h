// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file subscriptioncombobox.h
/// \brief Declares a combo box for selecting and creating OPC UA subscriptions.
///

#pragma once

#include <QComboBox>
#include <QString>
#include <QVector>

#include "models/subscriptionitem.h"

///
/// \brief Combo box that lists the configured subscriptions and can create a new one.
///
/// Populated from the application's shared subscription list, it presents each
/// subscription as "name (interval ms)" and optionally a leading empty choice and a
/// trailing "New subscription" entry. Choosing the create entry opens the
/// new-subscription dialog and, on acceptance, emits subscriptionCreationRequested so
/// the host can add it to the shared list; the created subscription is selected
/// immediately. This is the single subscription selector used across the project.
///
class SubscriptionComboBox : public QComboBox
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs the combo box with the create entry enabled.
    /// \param parent Parent widget.
    ///
    explicit SubscriptionComboBox(QWidget *parent = nullptr);

    ///
    /// \brief Replaces the listed subscriptions, preserving the current selection by name.
    /// \param subscriptions Subscriptions in display order.
    ///
    void setSubscriptions(const QVector<SubscriptionItem> &subscriptions);

    ///
    /// \brief Returns the currently listed subscriptions.
    /// \return Subscriptions in display order.
    ///
    QVector<SubscriptionItem> subscriptions() const;

    ///
    /// \brief Returns the selected subscription name, or empty for the none entry.
    /// \return Selected subscription name.
    ///
    QString currentSubscription() const;

    ///
    /// \brief Selects the entry for a subscription name, defaulting to the first entry.
    /// \param name Subscription name to select.
    ///
    void setCurrentSubscription(const QString &name);

    ///
    /// \brief Returns the publishing interval of the selected subscription.
    /// \return Publishing interval in milliseconds, or the default when none is selected.
    ///
    double currentInterval() const;

    ///
    /// \brief Shows or hides the leading empty ("none") choice.
    /// \param visible True to offer an empty selection.
    ///
    void setNoneEntryVisible(bool visible);

    ///
    /// \brief Shows or hides the trailing "New subscription" entry.
    /// \param visible True to offer creating a new subscription.
    ///
    void setCreateEntryVisible(bool visible);

signals:
    ///
    /// \brief Emitted when the user selects a subscription (or the empty choice).
    /// \param name Selected subscription name, empty for the none entry.
    ///
    void subscriptionSelected(const QString &name);

    ///
    /// \brief Emitted after the user creates a new subscription.
    /// \param name New subscription name.
    /// \param publishingInterval Publishing interval in milliseconds.
    ///
    void subscriptionCreationRequested(const QString &name, double publishingInterval);

private slots:
    void handleActivated(int index);

private:
    void rebuild();
    void promptNewSubscription();
    double intervalForSubscription(const QString &name) const;

    QVector<SubscriptionItem> _subscriptions;
    QString _selectedName;
    int _previousIndex = 0;
    bool _noneEntry = false;
    bool _createEntry = true;
};
