// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file subscriptionitem.h
/// \brief Declares subscription, event and history data types.
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

///
/// \brief Describes an OPC UA history read request entry.
///
struct HistoryItem
{
    /// \brief Node display text.
    QString node;
    /// \brief History time range display text.
    QString range;
};
