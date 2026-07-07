// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file sessiondata.h
/// \brief Declares the serializable working-session payload.
///

#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

#include "models/subscriptionitem.h"
#include "opcua/connectionprofile.h"

///
/// \brief One monitored data-access node restored with a working session.
///
struct SessionNode
{
    /// \brief NodeId string.
    QString nodeId;

    /// \brief Assigned subscription name, empty when the node is not monitored.
    QString subscriptionName;
};

///
/// \brief Snapshot of a working session: connection plus its monitoring workspace.
///
/// Secrets are never part of this payload; the connection profile references its
/// credentials by profile id through the keychain, or the user is prompted on load.
///
struct SessionData
{
    /// \brief Active connection profile, excluding any secrets.
    ConnectionProfile profile;

    /// \brief User-created subscriptions to recreate on load.
    QVector<SubscriptionItem> subscriptions;

    /// \brief Monitored data-access nodes with their subscription assignment.
    QVector<SessionNode> dataAccessNodes;

    /// \brief Charted trend node ids.
    QStringList trendNodes;
};
