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
    QString name;
    QString publishingInterval;
};

///
/// \brief Describes an OPC UA event entry shown in the event table.
///
struct EventItem
{
    QString time;
    QString message;
};

///
/// \brief Describes an OPC UA history read request entry.
///
struct HistoryItem
{
    QString node;
    QString range;
};
