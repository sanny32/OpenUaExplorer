// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file subscriptionitem.h
/// \brief Declares subscription and event data types.
///

#pragma once

#include <QString>

/// \brief Smallest publishing interval accepted from the user, in milliseconds.
constexpr int minPublishingIntervalMs = 50;

/// \brief Largest publishing interval accepted from the user, in milliseconds.
constexpr int maxPublishingIntervalMs = 600000;

///
/// \brief Stable identifiers of the built-in subscriptions.
///
enum BuiltinSubscriptionId {
    DefaultSubscriptionId = 0,
    FastSubscriptionId = 1,
    SlowSubscriptionId = 2
};

///
/// \brief Describes a configured OPC UA subscription.
///
struct SubscriptionItem
{
    /// \brief Subscription name.
    QString name;

    /// \brief Publishing interval in milliseconds.
    double publishingInterval = 1000.0;

    /// \brief Stable subscription identifier; 0 is the built-in Default subscription.
    int id = 0;

    /// \brief Whether this subscription is built in and cannot be removed.
    bool builtin = false;

    /// \brief Whether the user gave a built-in subscription a name of their own.
    bool renamed = false;

    ///
    /// \brief Reports whether this is the built-in Default subscription.
    /// \return True for the built-in Default subscription.
    ///
    bool isDefault() const { return id == 0; }

    ///
    /// \brief Reports whether this subscription is protected from removal.
    /// \return True for built-in subscriptions.
    ///
    bool isBuiltin() const { return builtin; }

    ///
    /// \brief Returns a display label combining the name and publishing interval.
    /// \return Text formatted as "name (interval ms)".
    ///
    QString label() const
    {
        return QStringLiteral("%1 (%2 ms)").arg(name, QString::number(publishingInterval));
    }
};

///
/// \brief Describes an OPC UA event entry shown in the event table.
///
struct EventItem
{
    /// \brief Event time display text.
    QString time;

    /// \brief Event severity display text.
    QString severity;

    /// \brief Event source name.
    QString source;

    /// \brief Event message.
    QString message;

    /// \brief Event type display text.
    QString eventType;
};
