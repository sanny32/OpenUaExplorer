// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dataaccessmonitoringstate.h
/// \brief Declares monitoring state tracked by the data-access coordinator.
///

#pragma once

#include <QSet>
#include <QString>

///
/// \brief Tracks subscribed and in-flight monitored nodes.
///
class DataAccessMonitoringState
{
public:
    bool isSubscribed(const QString &nodeId) const;
    bool isPending(const QString &nodeId) const;
    void beginRequest(const QString &nodeId);
    void finishRequest(const QString &nodeId, bool subscribed, bool success);
    void clear();

private:
    QSet<QString> _subscribedNodeIds;
    QSet<QString> _pendingNodeIds;
};
