#pragma once

#include <QString>

struct SubscriptionItem
{
    QString name;
    QString publishingInterval;
};

struct EventItem
{
    QString time;
    QString message;
};

struct HistoryItem
{
    QString node;
    QString range;
};
