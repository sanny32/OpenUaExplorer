// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_connectioncontroller.cpp
/// \brief Unit tests for ConnectionController and OpcUaClientService using fake dependencies.
///

#include <QHash>
#include <QSignalSpy>
#include <QTest>

#include "opcua/connectioncontroller.h"
#include "opcua/connectionprofilestore.h"
#include "opcua/opcuabackend.h"
#include "opcua/opcuaclientservice.h"

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
        storedProfiles = {profile};
        return true;
    }

    bool remove(const QString &) override { return true; }

    bool saveSucceeds = true;
    QList<ConnectionProfile> storedProfiles;
};

///
/// \brief Tests connect/save flows and timeout propagation through the controller and service.
///
class TestConnectionController : public QObject
{
    Q_OBJECT

private slots:
    void serviceForwardsBackendState();
    void serviceUsesProfileRequestTimeout();
    void savedProfileWithoutSecretsDiscoversThenConnects();
    void savedProfileLoadsBothSecrets();
    void discoveryFailureDoesNotConnect();
    void savePersistsProfileAndSecrets();
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

void TestConnectionController::savedProfileWithoutSecretsDiscoversThenConnects()
{
    FakeOpcUaBackend backend;
    OpcUaClientService service(&backend);
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    ConnectionController controller(&service, &secrets, &profiles);

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
    ConnectionController controller(&service, &secrets, &profiles);

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
    ConnectionController controller(&service, &secrets, &profiles);
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
    ConnectionController controller(&service, &secrets, &profiles);
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

QTEST_GUILESS_MAIN(TestConnectionController)

#include "test_connectioncontroller.moc"
