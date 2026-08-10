// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_connectioncontroller_support.h
/// \brief Shared doubles for ConnectionController tests.
///

#pragma once

#include <algorithm>

#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "fakesecretstore.h"
#include "opcua/connectioncontroller.h"
#include "opcua/connectioncredentialsprovider.h"
#include "opcua/connectionprofilestore.h"
#include "opcua/opcuabackend.h"
#include "opcua/recentconnectionstore.h"

///
/// \brief In-memory OPC UA backend double that records calls and drives discovery manually.
///
class FakeOpcUaBackend : public OpcUaBackend
{
public:
    explicit FakeOpcUaBackend(QObject *parent = nullptr)
        : OpcUaBackend(parent)
    {
    }

    bool isAvailable() const override { return true; }
    QStringList availableBackends() const override { return {QStringLiteral("fake")}; }
    OpcUaConnectionState state() const override { return currentState; }
    QString lastError() const override { return error; }
    void setCertificateTrustDecider(CertificateTrustDecider *) override {}

    void discoverEndpoints(const QString &url, const QString &backend,
                           int timeoutMs) override
    {
        discoveredUrl = url;
        discoveredBackend = backend;
        discoveryTimeout = timeoutMs;
        ++discoveryCalls;
    }

    void connectToEndpoint(const ConnectionProfile &profile,
                           const QString &password,
                           const QString &privateKeyPassword) override
    {
        connectedProfile = profile;
        connectedPassword = password;
        connectedPrivateKeyPassword = privateKeyPassword;
        ++connectCalls;
    }

    void disconnectFromEndpoint() override { ++disconnectCalls; }
    void browse(const QString &) override
    {
        browseTimeout = requestTimeout();
    }
    void browseReferences(const QString &) override
    {
        referencesBrowseTimeout = requestTimeout();
    }
    void readNode(const QString &) override {}
    void readValues(const QStringList &) override {}
    void writeValue(const QString &, const QVariant &, int) override {}
    void subscribe(const QString &nodeId, double publishingInterval) override
    {
        subscribedNodeId = nodeId;
        subscriptionPublishingInterval = publishingInterval;
    }
    void unsubscribe(const QString &nodeId) override
    {
        unsubscribedNodeId = nodeId;
    }

    void completeDiscovery(const QString &message = {})
    {
        emit endpointsDiscovered({}, message);
    }

    void setState(OpcUaConnectionState state)
    {
        currentState = state;
        emit stateChanged(state);
    }

    OpcUaConnectionState currentState = OpcUaConnectionState::Disconnected;
    QString error;
    QString discoveredUrl;
    QString discoveredBackend;
    int discoveryTimeout = 0;
    int discoveryCalls = 0;
    int connectCalls = 0;
    int disconnectCalls = 0;
    int browseTimeout = 0;
    int referencesBrowseTimeout = 0;
    ConnectionProfile connectedProfile;
    QString connectedPassword;
    QString connectedPrivateKeyPassword;
    QString subscribedNodeId;
    QString unsubscribedNodeId;
    double subscriptionPublishingInterval = 0.0;
};

///
/// \brief Profile store double that keeps a single saved profile in memory.
///
class FakeProfileStore : public ConnectionProfileStore
{
public:
    QList<ConnectionProfile> profiles() const override { return storedProfiles; }

    bool save(const ConnectionProfile &profile) override
    {
        if (!saveSucceeds)
            return false;
        remove(profile.id);
        storedProfiles.append(profile);
        return true;
    }

    bool remove(const QString &id) override
    {
        storedProfiles.erase(std::remove_if(storedProfiles.begin(), storedProfiles.end(),
                                            [&id](const ConnectionProfile &existing) {
                                                return existing.id == id;
                                            }),
                             storedProfiles.end());
        return true;
    }

    bool setOrder(const QStringList &orderedIds) override
    {
        if (!setOrderSucceeds)
            return false;
        order = orderedIds;
        return true;
    }

    bool saveSucceeds = true;
    bool setOrderSucceeds = true;
    QStringList order;
    QList<ConnectionProfile> storedProfiles;
};

///
/// \brief Recent-connection store double that keeps the history in memory.
///
class FakeRecentStore : public RecentConnectionStore
{
public:
    QList<ConnectionProfile> connections() const override { return recent; }

    void record(const ConnectionProfile &profile) override
    {
        recent.erase(std::remove_if(recent.begin(), recent.end(),
                                    [&profile](const ConnectionProfile &existing) {
                                        return existing.endpointUrl == profile.endpointUrl;
                                    }),
                     recent.end());
        recent.prepend(profile);
        while (recent.size() > RecentConnectionStore::maximumSize)
            recent.removeLast();
    }

    QList<ConnectionProfile> recent;
};

///
/// \brief Credentials provider double answering with a prepared reply.
///
class FakeCredentialsProvider : public ConnectionCredentialsProvider
{
public:
    ConnectionCredentials requestCredentials(const ConnectionProfile &profile,
                                             const QString &reason) override
    {
        ++requests;
        requestedProfile = profile;
        requestedReason = reason;
        ConnectionCredentials credentials = reply;
        if (credentials.profile.endpointUrl.isEmpty())
            credentials.profile = profile;
        return credentials;
    }

    int requests = 0;
    ConnectionProfile requestedProfile;
    QString requestedReason;
    ConnectionCredentials reply;
};