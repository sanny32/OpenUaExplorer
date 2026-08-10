// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file subscriptionswidget.h
/// \brief Declares the OPC UA subscriptions management widget.
///

#pragma once

#include <QVector>
#include <QWidget>

#include "appsettings.h"
#include "models/subscriptionitem.h"

class QPoint;

namespace Ui {
class SubscriptionsWidget;
}

class SubscriptionsModel;

///
/// \brief Widget that manages the configured OPC UA subscriptions.
///
class SubscriptionsWidget : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the subscriptions widget and its table view.
    /// \param parent Parent widget.
    ///
    explicit SubscriptionsWidget(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the widget and its generated UI.
    ///
    ~SubscriptionsWidget() override;

    ///
    /// \brief Resets the list to the built-in Default, Fast and Slow subscriptions.
    ///
    void reset();

    ///
    /// \brief Returns the translated factory name of a built-in subscription.
    /// \param id Built-in subscription identifier.
    /// \return Factory name, or an empty string when the id is not built in.
    ///
    static QString factoryName(int id);

    ///
    /// \brief Maps a stored subscription name onto the name of the current interface language.
    /// \param name Subscription name read from settings or a saved session.
    /// \return Current factory name when the stored one names a built-in subscription in any
    ///         shipped language, otherwise the name unchanged.
    ///
    static QString canonicalName(const QString &name);

    ///
    /// \brief Returns the current subscriptions as a snapshot.
    /// \return Subscriptions in row order.
    ///
    QVector<SubscriptionItem> subscriptions() const;

    ///
    /// \brief Persists the subscriptions table header state.
    /// \param settings Settings store to write to.
    ///
    void saveViewState(AppSettings &settings) const;

    ///
    /// \brief Restores the subscriptions table header state.
    /// \param settings Settings store to read from.
    ///
    void restoreViewState(AppSettings &settings);

    ///
    /// \brief Persists the user-created subscriptions and any edits to the built-in ones.
    /// \param settings Settings store to write to.
    ///
    void saveSubscriptions(AppSettings &settings) const;

    ///
    /// \brief Restores the subscriptions saved from the last run.
    /// \param settings Settings store to read from.
    ///
    void loadSubscriptions(AppSettings &settings);

public slots:
    ///
    /// \brief Adds a subscription with the given name and publishing interval.
    /// \param name Subscription name; ignored when empty or already in use.
    /// \param interval Publishing interval in milliseconds.
    ///
    void createSubscription(const QString &name, double interval);

    ///
    /// \brief Restores the factory name and publishing interval of every built-in subscription.
    ///
    void restoreBuiltinDefaults();

signals:
    ///
    /// \brief Emitted whenever the set of subscriptions changes.
    /// \param subscriptions Current subscriptions in row order.
    ///
    void subscriptionsChanged(QVector<SubscriptionItem> subscriptions);

    ///
    /// \brief Emitted when a subscription is renamed.
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

    ///
    /// \brief Emitted just before a subscription row is removed.
    /// \param name Subscription being removed.
    ///
    void subscriptionRemoved(QString name);

protected:
    ///
    /// \brief Re-applies the built-in row styling when the palette changes.
    /// \param event Change event being handled.
    ///
    void changeEvent(QEvent *event) override;

private:
    void setupSubscriptionsView();
    void applyBuiltinDecoration();
    void showSubscriptionsContextMenu(const QPoint &pos);
    void addSubscription();
    void removeSelectedSubscriptions();
    void removeAllSubscriptions();
    void removeSubscriptionRow(int row);
    bool hasRemovableSubscriptions() const;
    int  nextSubscriptionId() const;
    void emitSubscriptionsChanged();
    void retranslateBuiltinNames();

    Ui::SubscriptionsWidget *ui;
    SubscriptionsModel      *_subscriptionsModel;
};
