// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file subscriptionitem.h
/// \brief Declares subscription and event data types.
///

#pragma once

#include <QString>

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

    ///
    /// \brief Reports whether this is the built-in Default subscription.
    /// \return True for the built-in Default subscription.
    ///
    bool isDefault() const { return id == 0; }
};

///
/// \brief Describes an OPC UA event entry shown in the event table.
///
struct EventItem
{
    /// \brief Event time display text.
    QString time;

    /// \brief Event message.
    QString message;
};
