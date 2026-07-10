// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_connectioncontroller.cpp
/// \brief Unit tests for ConnectionController and OpcUaClientService using fake dependencies.
///

#include <algorithm>

#include <QHash>
#include <QSignalSpy>
#include <QTest>

#include "opcua/connectioncontroller.h"
#include "opcua/connectionprofilestore.h"
#include "opcua/opcuabackend.h"
#include "opcua/opcuaclientservice.h"
#include "opcua/recentconnectionstore.h"

///
/// \brief In-memory OPC UA backend double that records calls and drives discovery manually.
///
class FakeOpcUaBackend : public OpcUaBackend
{
    Q_OBJECT

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
    void browse(const QString &, int timeoutMs) override
    {
        browseTimeout = timeoutMs;
    }
    void browseReferences(const QString &, int timeoutMs) override
    {
        referencesBrowseTimeout = timeoutMs;
    }
    void readNode(const QString &, int) override {}
    void readValues(const QStringList &, int) override {}
    void writeValue(const QString &, const QVariant &, int, int) override {}
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
/// \brief Secret store double backed by an in-memory map, resolving reads synchronously.
///
class FakeSecretStore : public SecretStore
{
    Q_OBJECT

public:
    using SecretStore::SecretStore;

    bool isAvailable() const override { return true; }

    void read(const QString &profileId, Secret secret) override
    {
        const QString value = values.value(key(profileId, secret));
        emit readFinished(profileId, secret, value, errors.value(key(profileId, secret)));
    }

    void write(const QString &profileId, Secret secret, const QString &value) override
    {
        values.insert(key(profileId, secret), value);
        emit writeFinished(profileId, secret, {});
    }

    void remove(const QString &profileId, Secret secret) override
    {
        values.remove(key(profileId, secret));
        emit writeFinished(profileId, secret, {});
    }

    static QString key(const QString &profileId, Secret secret)
    {
        return profileId + QLatin1Char('/')
            + QString::number(static_cast<int>(secret));
    }

    QHash<QString, QString> values;
    QHash<QString, QString> errors;
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
/// \brief Tests connect/save flows and timeout propagation through the controller and service.
///
class TestConnectionController : public QObject
{
    Q_OBJECT

private slots:
    void serviceForwardsBackendState();
    void serviceReportsFindServersUnsupported();
    void serviceUsesProfileRequestTimeout();
    void serviceForwardsMonitoringRequestsAndResults();
    void savedProfileWithoutSecretsDiscoversThenConnects();
    void savedProfileLoadsBothSecrets();
    void discoveryFailureDoesNotConnect();
    void savePersistsProfileAndSecrets();
    void savingSameEndpointReplacesFavorite();
    void savingSameEndpointDifferentSecurityKeepsBoth();
    void savingSameEndpointDifferentAuthenticationKeepsBoth();
    void removeFavoriteDeletesProfileAndSecrets();
    void reorderFavoritesPersistsOrderAndNotifies();
    void reorderFavoritesFailureReportsError();
    void connectingRecordsRecentConnection();
};

void TestConnectionController::serviceForwardsBackendState()
{
    FakeOpcUaBackend backend;
    OpcUaClientService service(&backend);
    QSignalSpy stateSpy(&service, &OpcUaClientService::stateChanged);

    backend.currentState = OpcUaConnectionState::Connected;
    emit backend.stateChanged(backend.currentState);

    QCOMPARE(service.state(), OpcUaConnectionState::Connected);
    QCOMPARE(stateSpy.size(), 1);
}

void TestConnectionController::serviceReportsFindServersUnsupported()
{
    FakeOpcUaBackend backend;
    OpcUaClientService service(&backend);
    QSignalSpy serversSpy(&service, &OpcUaClientService::serversDiscovered);

    service.findServers(QStringLiteral("opc.tcp://localhost:4840"));

    QCOMPARE(serversSpy.size(), 1);
    const QList<QVariant> arguments = serversSpy.takeFirst();
    QVERIFY(arguments.at(0).value<QList<ServerInfo>>().isEmpty());
    QVERIFY(!arguments.at(1).toString().isEmpty());
}

void TestConnectionController::serviceUsesProfileRequestTimeout()
{
    FakeOpcUaBackend backend;
    OpcUaClientService service(&backend);
    ConnectionProfile profile;
    profile.requestTimeoutMs = 2345;

    service.connectToEndpoint(profile);
    service.browse(QStringLiteral("ns=0;i=85"));
    service.browseReferences(QStringLiteral("ns=0;i=85"));

    QCOMPARE(backend.browseTimeout, profile.requestTimeoutMs);
    QCOMPARE(backend.referencesBrowseTimeout, profile.requestTimeoutMs);
}

///
/// \brief Monitoring calls reach the backend and their completion is forwarded.
///
void TestConnectionController::serviceForwardsMonitoringRequestsAndResults()
{
    FakeOpcUaBackend backend;
    OpcUaClientService service(&backend);
    QSignalSpy finishedSpy(&service, &OpcUaClientService::monitoringFinished);
    const QString nodeId = QStringLiteral("ns=2;s=Temperature");

    service.subscribe(nodeId);
    QCOMPARE(backend.subscribedNodeId, nodeId);
    QCOMPARE(backend.subscriptionPublishingInterval, 1000.0);

    emit backend.monitoringFinished(nodeId, true, true, QString());
    QCOMPARE(finishedSpy.size(), 1);
    QCOMPARE(finishedSpy.constFirst().at(0).toString(), nodeId);
    QCOMPARE(finishedSpy.constFirst().at(1).toBool(), true);
    QCOMPARE(finishedSpy.constFirst().at(2).toBool(), true);

    service.unsubscribe(nodeId);
    QCOMPARE(backend.unsubscribedNodeId, nodeId);
}

void TestConnectionController::savedProfileWithoutSecretsDiscoversThenConnects()
{
    FakeOpcUaBackend backend;
    OpcUaClientService service(&backend);
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    ConnectionController controller(&service, &secrets, &profiles, &recents);

    ConnectionProfile profile;
    profile.id = QStringLiteral("anonymous");
    profile.endpointUrl = QStringLiteral("opc.tcp://localhost:4840");
    profile.backend = QStringLiteral("fake");
    profile.endpointTimeoutMs = 4321;
    controller.connectSavedProfile(profile);

    QCOMPARE(backend.discoveryCalls, 1);
    QCOMPARE(backend.discoveredUrl, profile.endpointUrl);
    QCOMPARE(backend.discoveryTimeout, profile.endpointTimeoutMs);
    backend.completeDiscovery();
    QCOMPARE(backend.connectCalls, 1);
    QCOMPARE(backend.connectedProfile.id, profile.id);
}

void TestConnectionController::savedProfileLoadsBothSecrets()
{
    FakeOpcUaBackend backend;
    OpcUaClientService service(&backend);
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    ConnectionController controller(&service, &secrets, &profiles, &recents);

    ConnectionProfile profile;
    profile.id = QStringLiteral("secured");
    profile.endpointUrl = QStringLiteral("opc.tcp://localhost:4840");
    profile.authentication = ConnectionProfile::Authentication::Username;
    profile.privateKeyFile = QStringLiteral("client.pem");
    secrets.values.insert(
        FakeSecretStore::key(profile.id, SecretStore::Secret::Password),
        QStringLiteral("login-secret"));
    secrets.values.insert(
        FakeSecretStore::key(profile.id, SecretStore::Secret::PrivateKeyPassword),
        QStringLiteral("key-secret"));

    controller.connectSavedProfile(profile);
    QCOMPARE(backend.discoveryCalls, 1);
    backend.completeDiscovery();
    QCOMPARE(backend.connectedPassword, QStringLiteral("login-secret"));
    QCOMPARE(backend.connectedPrivateKeyPassword, QStringLiteral("key-secret"));
}

void TestConnectionController::discoveryFailureDoesNotConnect()
{
    FakeOpcUaBackend backend;
    OpcUaClientService service(&backend);
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    ConnectionController controller(&service, &secrets, &profiles, &recents);
    QSignalSpy errorSpy(&controller, &ConnectionController::errorOccurred);

    ConnectionProfile profile;
    profile.id = QStringLiteral("failed");
    profile.endpointUrl = QStringLiteral("opc.tcp://localhost:4840");
    controller.connectSavedProfile(profile);
    backend.completeDiscovery(QStringLiteral("discovery failed"));

    QCOMPARE(backend.connectCalls, 0);
    QCOMPARE(errorSpy.size(), 1);
}

void TestConnectionController::savePersistsProfileAndSecrets()
{
    FakeOpcUaBackend backend;
    OpcUaClientService service(&backend);
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    ConnectionController controller(&service, &secrets, &profiles, &recents);
    QSignalSpy changedSpy(&controller, &ConnectionController::profilesChanged);

    ConnectionProfile profile;
    profile.id = QStringLiteral("saved");
    controller.saveProfile(profile, QStringLiteral("password"),
                           QStringLiteral("key-password"));

    QCOMPARE(profiles.storedProfiles.size(), 1);
    QCOMPARE(secrets.values.value(FakeSecretStore::key(
                 profile.id, SecretStore::Secret::Password)),
             QStringLiteral("password"));
    QCOMPARE(secrets.values.value(FakeSecretStore::key(
                 profile.id, SecretStore::Secret::PrivateKeyPassword)),
             QStringLiteral("key-password"));
    QCOMPARE(changedSpy.size(), 1);
}

void TestConnectionController::savingSameEndpointReplacesFavorite()
{
    FakeOpcUaBackend backend;
    OpcUaClientService service(&backend);
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    ConnectionController controller(&service, &secrets, &profiles, &recents);

    ConnectionProfile first;
    first.id = QStringLiteral("first");
    first.endpointUrl = QStringLiteral("opc.tcp://host:4840");
    first.securityPolicy = QStringLiteral("Basic256Sha256");
    first.securityMode = 3;
    controller.saveProfile(first, QStringLiteral("pw1"), QString());

    ConnectionProfile second;
    second.id = QStringLiteral("second");
    second.endpointUrl = QStringLiteral("opc.tcp://host:4840");
    second.securityPolicy = QStringLiteral("Basic256Sha256");
    second.securityMode = 3;
    controller.saveProfile(second, QString(), QString());

    QCOMPARE(profiles.storedProfiles.size(), 1);
    QCOMPARE(profiles.storedProfiles.first().id, QStringLiteral("second"));
    QVERIFY(!secrets.values.contains(
        FakeSecretStore::key(QStringLiteral("first"), SecretStore::Secret::Password)));
}

void TestConnectionController::savingSameEndpointDifferentSecurityKeepsBoth()
{
    FakeOpcUaBackend backend;
    OpcUaClientService service(&backend);
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    ConnectionController controller(&service, &secrets, &profiles, &recents);

    ConnectionProfile signEncrypt;
    signEncrypt.id = QStringLiteral("sign-encrypt");
    signEncrypt.endpointUrl = QStringLiteral("opc.tcp://host:4840");
    signEncrypt.securityPolicy = QStringLiteral("Basic256Sha256");
    signEncrypt.securityMode = 3;
    controller.saveProfile(signEncrypt, QString(), QString());

    ConnectionProfile sign;
    sign.id = QStringLiteral("sign");
    sign.endpointUrl = QStringLiteral("opc.tcp://host:4840");
    sign.securityPolicy = QStringLiteral("Aes128_Sha256_RsaOaep");
    sign.securityMode = 2;
    controller.saveProfile(sign, QString(), QString());

    QCOMPARE(profiles.storedProfiles.size(), 2);
}

void TestConnectionController::savingSameEndpointDifferentAuthenticationKeepsBoth()
{
    FakeOpcUaBackend backend;
    OpcUaClientService service(&backend);
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    ConnectionController controller(&service, &secrets, &profiles, &recents);

    ConnectionProfile anonymous;
    anonymous.id = QStringLiteral("anonymous");
    anonymous.endpointUrl = QStringLiteral("opc.tcp://host:4840");
    anonymous.securityPolicy = QStringLiteral("Basic256Sha256");
    anonymous.securityMode = 3;
    anonymous.authentication = ConnectionProfile::Authentication::Anonymous;
    controller.saveProfile(anonymous, QString(), QString());

    ConnectionProfile username;
    username.id = QStringLiteral("username");
    username.endpointUrl = QStringLiteral("opc.tcp://host:4840");
    username.securityPolicy = QStringLiteral("Basic256Sha256");
    username.securityMode = 3;
    username.authentication = ConnectionProfile::Authentication::Username;
    controller.saveProfile(username, QStringLiteral("pw"), QString());

    QCOMPARE(profiles.storedProfiles.size(), 2);
}

void TestConnectionController::removeFavoriteDeletesProfileAndSecrets()
{
    FakeOpcUaBackend backend;
    OpcUaClientService service(&backend);
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    ConnectionController controller(&service, &secrets, &profiles, &recents);

    ConnectionProfile profile;
    profile.id = QStringLiteral("fav");
    profile.endpointUrl = QStringLiteral("opc.tcp://host:4840");
    controller.saveProfile(profile, QStringLiteral("pw"), QString());
    QCOMPARE(profiles.storedProfiles.size(), 1);

    QSignalSpy changedSpy(&controller, &ConnectionController::profilesChanged);
    controller.removeFavorite(QStringLiteral("fav"));

    QVERIFY(profiles.storedProfiles.isEmpty());
    QVERIFY(!secrets.values.contains(
        FakeSecretStore::key(QStringLiteral("fav"), SecretStore::Secret::Password)));
    QCOMPARE(changedSpy.size(), 1);
}

void TestConnectionController::reorderFavoritesPersistsOrderAndNotifies()
{
    FakeOpcUaBackend backend;
    OpcUaClientService service(&backend);
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    ConnectionController controller(&service, &secrets, &profiles, &recents);
    QSignalSpy changedSpy(&controller, &ConnectionController::profilesChanged);

    controller.reorderFavorites({QStringLiteral("b"), QStringLiteral("a")});

    QCOMPARE(profiles.order, (QStringList{QStringLiteral("b"), QStringLiteral("a")}));
    QCOMPARE(changedSpy.size(), 1);
}

void TestConnectionController::reorderFavoritesFailureReportsError()
{
    FakeOpcUaBackend backend;
    OpcUaClientService service(&backend);
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    profiles.setOrderSucceeds = false;
    ConnectionController controller(&service, &secrets, &profiles, &recents);
    QSignalSpy changedSpy(&controller, &ConnectionController::profilesChanged);
    QSignalSpy errorSpy(&controller, &ConnectionController::errorOccurred);

    controller.reorderFavorites({QStringLiteral("a")});

    QCOMPARE(changedSpy.size(), 0);
    QCOMPARE(errorSpy.size(), 1);
}

void TestConnectionController::connectingRecordsRecentConnection()
{
    FakeOpcUaBackend backend;
    OpcUaClientService service(&backend);
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    ConnectionController controller(&service, &secrets, &profiles, &recents);
    QSignalSpy recentsSpy(&controller, &ConnectionController::recentsChanged);

    ConnectionProfile first;
    first.id = QStringLiteral("a");
    first.endpointUrl = QStringLiteral("opc.tcp://a:4840");
    controller.connectNewProfile(first, QString(), QString());

    ConnectionProfile second;
    second.id = QStringLiteral("b");
    second.endpointUrl = QStringLiteral("opc.tcp://b:4840");
    controller.connectNewProfile(second, QString(), QString());

    const QList<ConnectionProfile> recent = controller.recentConnections();
    QCOMPARE(recent.size(), 2);
    QCOMPARE(recent.first().endpointUrl, second.endpointUrl);
    QCOMPARE(recent.last().endpointUrl, first.endpointUrl);
    QCOMPARE(recentsSpy.size(), 2);

    controller.connectNewProfile(first, QString(), QString());
    const QList<ConnectionProfile> reordered = controller.recentConnections();
    QCOMPARE(reordered.size(), 2);
    QCOMPARE(reordered.first().endpointUrl, first.endpointUrl);
}

QTEST_GUILESS_MAIN(TestConnectionController)

#include "test_connectioncontroller.moc"
