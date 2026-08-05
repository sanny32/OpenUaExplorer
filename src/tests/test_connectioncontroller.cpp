// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_connectioncontroller.cpp
/// \brief Unit tests for ConnectionController and OpcUaBackend using fake dependencies.
///

#include <algorithm>

#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

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

///
/// \brief Tests connect/save flows and timeout propagation through the controller and backend.
///
class TestConnectionController : public QObject
{
    Q_OBJECT

private slots:
    void backendReportsFindServersUnsupported();
    void connectingAppliesTheProfileRequestTimeout();
    void connectingAddsStableInstanceSuffixToSessionNames();
    void savedProfileWithoutSecretsDiscoversThenConnects();
    void savedProfileLoadsOnlyTheUserPassword();
    void storedPasswordConnectsWithoutPrompting();
    void missingPasswordIsAskedForBeforeConnecting();
    void decliningTheCredentialsPromptAbortsTheConnection();
    void rememberedCredentialsAreStoredForTheNextConnection();
    void rememberedCredentialsNeverStoreTheKeyPassword();
    void missingCertificateFileIsAskedForAgain();
    void rejectedCredentialsAreAskedForAgain();
    void decliningAfterARejectionStopsRetrying();
    void discoveryFailureDoesNotConnect();
    void savePersistsProfileAndPassword();
    void rememberPasswordStoresTheSecretWithoutTheProfile();
    void savingSameEndpointReplacesFavorite();
    void savingSameEndpointDifferentSecurityKeepsBoth();
    void savingSameEndpointDifferentAuthenticationKeepsBoth();
    void removeFavoriteDeletesProfileAndSecrets();
    void reorderFavoritesPersistsOrderAndNotifies();
    void reorderFavoritesFailureReportsError();
    void connectingRecordsRecentConnection();
};

///
/// \brief A backend that does not implement FindServers reports the operation as unsupported.
///
void TestConnectionController::backendReportsFindServersUnsupported()
{
    FakeOpcUaBackend backend;
    QSignalSpy serversSpy(&backend, &OpcUaBackend::serversDiscovered);

    backend.findServers(QStringLiteral("opc.tcp://localhost:4840"),
                        QStringLiteral("fake"), 5000);

    QCOMPARE(serversSpy.size(), 1);
    const QList<QVariant> arguments = serversSpy.takeFirst();
    QVERIFY(arguments.at(0).value<QList<ServerInfo>>().isEmpty());
    QVERIFY(!arguments.at(1).toString().isEmpty());
}

///
/// \brief Connecting a profile applies its request timeout to the backend's session requests.
///
void TestConnectionController::connectingAppliesTheProfileRequestTimeout()
{
    FakeOpcUaBackend backend;
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    ConnectionController controller(&backend, &secrets, &profiles, &recents);

    ConnectionProfile profile;
    profile.requestTimeoutMs = 2345;
    controller.connectNewProfile(profile, QString(), QString());

    backend.browse(QStringLiteral("ns=0;i=85"));
    backend.browseReferences(QStringLiteral("ns=0;i=85"));

    QCOMPARE(backend.requestTimeout(), profile.requestTimeoutMs);
    QCOMPARE(backend.browseTimeout, profile.requestTimeoutMs);
    QCOMPARE(backend.referencesBrowseTimeout, profile.requestTimeoutMs);
}

///
/// \brief Connections use a stable process suffix without changing the stored profile name.
///
void TestConnectionController::connectingAddsStableInstanceSuffixToSessionNames()
{
    FakeOpcUaBackend backend;
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    ConnectionController controller(&backend, &secrets, &profiles, &recents);

    ConnectionProfile automatic;
    controller.connectNewProfile(automatic, QString(), QString());
    const QString automaticName = backend.connectedProfile.sessionName;
    const QRegularExpression pattern(
        QStringLiteral("^.+/[0-9]+-[0-9a-f]{8}$"));
    QVERIFY(pattern.match(automaticName).hasMatch());
    QCOMPARE(controller.activeSessionName(), automaticName);
    QVERIFY(controller.activeProfile().sessionName.isEmpty());
    QVERIFY(recents.recent.constFirst().sessionName.isEmpty());

    const QString suffix = automaticName.mid(automaticName.lastIndexOf(QLatin1Char('/')));
    ConnectionProfile configured;
    configured.sessionName = QStringLiteral("Operator Session");
    controller.connectNewProfile(configured, QString(), QString());

    QCOMPARE(backend.connectedProfile.sessionName,
             configured.sessionName + suffix);
    QCOMPARE(controller.activeSessionName(), configured.sessionName + suffix);
    QCOMPARE(controller.activeProfile().sessionName, configured.sessionName);
}

void TestConnectionController::savedProfileWithoutSecretsDiscoversThenConnects()
{
    FakeOpcUaBackend backend;
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    ConnectionController controller(&backend, &secrets, &profiles, &recents);

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

///
/// \brief Only the user password comes from the keychain; a stored key password is left alone.
///
/// The backend refuses encrypted keys outright, so passing a stored key password on would turn
/// every connection of the profile into that error.
///
void TestConnectionController::savedProfileLoadsOnlyTheUserPassword()
{
    FakeOpcUaBackend backend;
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    ConnectionController controller(&backend, &secrets, &profiles, &recents);

    ConnectionProfile profile;
    profile.id = QStringLiteral("secured");
    profile.endpointUrl = QStringLiteral("opc.tcp://localhost:4840");
    profile.authentication = ConnectionProfile::Authentication::Username;
    profile.privateKeyFile = QStringLiteral("client.pem");
    secrets.values.insert(
        FakeSecretStore::key(profile.id, SecretStore::Secret::Password),
        QStringLiteral("login-secret"));
    // Left over from an older version that stored it; it must not reach the backend.
    secrets.values.insert(
        FakeSecretStore::key(profile.id, SecretStore::Secret::PrivateKeyPassword),
        QStringLiteral("key-secret"));

    controller.connectSavedProfile(profile);
    QCOMPARE(backend.discoveryCalls, 1);
    backend.completeDiscovery();
    QCOMPARE(backend.connectedPassword, QStringLiteral("login-secret"));
    QVERIFY(backend.connectedPrivateKeyPassword.isEmpty());
}

///
/// \brief A profile whose password is stored reconnects without interrupting the user.
///
void TestConnectionController::storedPasswordConnectsWithoutPrompting()
{
    FakeOpcUaBackend backend;
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    FakeCredentialsProvider credentials;
    ConnectionController controller(&backend, &secrets, &profiles, &recents);
    controller.setCredentialsProvider(&credentials);

    ConnectionProfile profile;
    profile.id = QStringLiteral("stored");
    profile.endpointUrl = QStringLiteral("opc.tcp://localhost:4840");
    profile.authentication = ConnectionProfile::Authentication::Username;
    profile.username = QStringLiteral("operator");
    secrets.values.insert(FakeSecretStore::key(profile.id, SecretStore::Secret::Password),
                          QStringLiteral("stored-secret"));

    controller.connectSavedProfile(profile);

    QCOMPARE(credentials.requests, 0);
    QCOMPARE(backend.discoveryCalls, 1);
    backend.completeDiscovery();
    QCOMPARE(backend.connectedPassword, QStringLiteral("stored-secret"));
}

///
/// \brief Without a stored password the provider is asked, and its answer is what connects.
///
void TestConnectionController::missingPasswordIsAskedForBeforeConnecting()
{
    FakeOpcUaBackend backend;
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    FakeCredentialsProvider credentials;
    ConnectionController controller(&backend, &secrets, &profiles, &recents);
    controller.setCredentialsProvider(&credentials);

    ConnectionProfile profile;
    profile.id = QStringLiteral("asked");
    profile.endpointUrl = QStringLiteral("opc.tcp://localhost:4840");
    profile.authentication = ConnectionProfile::Authentication::Username;
    profile.username = QStringLiteral("operator");
    credentials.reply.accepted = true;
    credentials.reply.password = QStringLiteral("typed-secret");

    controller.connectSavedProfile(profile);

    QCOMPARE(credentials.requests, 1);
    QCOMPARE(credentials.requestedProfile.id, profile.id);
    QCOMPARE(backend.discoveryCalls, 1);
    backend.completeDiscovery();
    QCOMPARE(backend.connectedPassword, QStringLiteral("typed-secret"));
}

///
/// \brief Cancelling the credentials prompt leaves the connection unstarted and unrecorded.
///
void TestConnectionController::decliningTheCredentialsPromptAbortsTheConnection()
{
    FakeOpcUaBackend backend;
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    FakeCredentialsProvider credentials;
    ConnectionController controller(&backend, &secrets, &profiles, &recents);
    controller.setCredentialsProvider(&credentials);
    QSignalSpy abortedSpy(&controller, &ConnectionController::connectionAborted);

    ConnectionProfile profile;
    profile.id = QStringLiteral("declined");
    profile.endpointUrl = QStringLiteral("opc.tcp://localhost:4840");
    profile.authentication = ConnectionProfile::Authentication::Username;

    controller.connectSavedProfile(profile);

    QCOMPARE(credentials.requests, 1);
    QCOMPARE(abortedSpy.size(), 1);
    QCOMPARE(backend.discoveryCalls, 0);
    QCOMPARE(backend.connectCalls, 0);
    QVERIFY(recents.recent.isEmpty());
}

///
/// \brief Credentials the user asked to remember reconnect the same profile unattended.
///
void TestConnectionController::rememberedCredentialsAreStoredForTheNextConnection()
{
    FakeOpcUaBackend backend;
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    FakeCredentialsProvider credentials;
    ConnectionController controller(&backend, &secrets, &profiles, &recents);
    controller.setCredentialsProvider(&credentials);

    ConnectionProfile profile;
    profile.id = QStringLiteral("remembered");
    profile.endpointUrl = QStringLiteral("opc.tcp://localhost:4840");
    profile.authentication = ConnectionProfile::Authentication::Username;
    credentials.reply.accepted = true;
    credentials.reply.password = QStringLiteral("typed-secret");
    credentials.reply.remember = true;

    controller.connectSavedProfile(profile);
    QCOMPARE(credentials.requests, 1);

    controller.connectSavedProfile(profile);

    QCOMPARE(credentials.requests, 1);
    QCOMPARE(backend.discoveryCalls, 2);
    backend.completeDiscovery();
    QCOMPARE(backend.connectedPassword, QStringLiteral("typed-secret"));
}

///
/// \brief The private-key password is used for the attempt only, never filed in the keychain.
///
void TestConnectionController::rememberedCredentialsNeverStoreTheKeyPassword()
{
    FakeOpcUaBackend backend;
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    FakeCredentialsProvider credentials;
    ConnectionController controller(&backend, &secrets, &profiles, &recents);
    controller.setCredentialsProvider(&credentials);

    ConnectionProfile profile;
    profile.id = QStringLiteral("key-password");
    profile.endpointUrl = QStringLiteral("opc.tcp://localhost:4840");
    profile.authentication = ConnectionProfile::Authentication::Username;
    credentials.reply.accepted = true;
    credentials.reply.password = QStringLiteral("typed-secret");
    credentials.reply.privateKeyPassword = QStringLiteral("key-secret");
    credentials.reply.remember = true;

    controller.connectSavedProfile(profile);
    backend.completeDiscovery();

    QCOMPARE(backend.connectedPrivateKeyPassword, QStringLiteral("key-secret"));
    QVERIFY(secrets.values.contains(
        FakeSecretStore::key(profile.id, SecretStore::Secret::Password)));
    QVERIFY(!secrets.values.contains(
        FakeSecretStore::key(profile.id, SecretStore::Secret::PrivateKeyPassword)));
}

///
/// \brief A certificate the profile names but the machine no longer has is asked for again.
///
void TestConnectionController::missingCertificateFileIsAskedForAgain()
{
    QTemporaryDir pkiDirectory;
    QVERIFY(pkiDirectory.isValid());
    const QString certificatePath = pkiDirectory.filePath(QStringLiteral("client.der"));
    const QString privateKeyPath = pkiDirectory.filePath(QStringLiteral("client.pem"));
    for (const QString &path : {certificatePath, privateKeyPath}) {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("test");
    }

    FakeOpcUaBackend backend;
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    FakeCredentialsProvider credentials;
    ConnectionController controller(&backend, &secrets, &profiles, &recents);
    controller.setCredentialsProvider(&credentials);

    ConnectionProfile profile;
    profile.id = QStringLiteral("certificate");
    profile.endpointUrl = QStringLiteral("opc.tcp://localhost:4840");
    profile.authentication = ConnectionProfile::Authentication::Certificate;
    profile.clientCertificateFile = certificatePath;
    profile.privateKeyFile = privateKeyPath;

    // Both files are there, so the profile connects on its own.
    controller.connectSavedProfile(profile);
    QCOMPARE(credentials.requests, 0);
    QCOMPARE(backend.discoveryCalls, 1);
    backend.completeDiscovery();

    QVERIFY(QFile::remove(certificatePath));
    ConnectionProfile replacement = profile;
    replacement.clientCertificateFile = privateKeyPath;
    credentials.reply.accepted = true;
    credentials.reply.profile = replacement;

    controller.connectSavedProfile(profile);

    QCOMPARE(credentials.requests, 1);
    QVERIFY2(credentials.requestedReason.contains(QFileInfo(certificatePath).fileName()),
             qPrintable(credentials.requestedReason));
    QCOMPARE(backend.discoveryCalls, 2);
    backend.completeDiscovery();
    QCOMPARE(backend.connectedProfile.clientCertificateFile, privateKeyPath);
}

///
/// \brief A password the server turns down is asked for again, with the reason shown.
///
void TestConnectionController::rejectedCredentialsAreAskedForAgain()
{
    FakeOpcUaBackend backend;
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    FakeCredentialsProvider credentials;
    ConnectionController controller(&backend, &secrets, &profiles, &recents);
    controller.setCredentialsProvider(&credentials);

    ConnectionProfile profile;
    profile.id = QStringLiteral("rejected");
    profile.endpointUrl = QStringLiteral("opc.tcp://localhost:4840");
    profile.authentication = ConnectionProfile::Authentication::Username;
    secrets.values.insert(FakeSecretStore::key(profile.id, SecretStore::Secret::Password),
                          QStringLiteral("stale-secret"));

    controller.connectSavedProfile(profile);
    backend.completeDiscovery();
    QCOMPARE(backend.connectedPassword, QStringLiteral("stale-secret"));
    QCOMPARE(credentials.requests, 0);

    credentials.reply.accepted = true;
    credentials.reply.password = QStringLiteral("second-try");
    emit backend.authenticationRejected(QStringLiteral("Access denied."));
    backend.setState(OpcUaConnectionState::Disconnected);

    QTRY_COMPARE(credentials.requests, 1);
    QVERIFY(credentials.requestedReason.contains(QStringLiteral("Access denied.")));
    QCOMPARE(backend.discoveryCalls, 2);
    backend.completeDiscovery();
    QCOMPARE(backend.connectedPassword, QStringLiteral("second-try"));
}

///
/// \brief Giving up at the second prompt ends the attempt instead of retrying forever.
///
void TestConnectionController::decliningAfterARejectionStopsRetrying()
{
    FakeOpcUaBackend backend;
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    FakeCredentialsProvider credentials;
    ConnectionController controller(&backend, &secrets, &profiles, &recents);
    controller.setCredentialsProvider(&credentials);
    QSignalSpy abortedSpy(&controller, &ConnectionController::connectionAborted);

    ConnectionProfile profile;
    profile.id = QStringLiteral("declined-retry");
    profile.endpointUrl = QStringLiteral("opc.tcp://localhost:4840");
    profile.authentication = ConnectionProfile::Authentication::Username;
    credentials.reply.accepted = true;
    credentials.reply.password = QStringLiteral("first-try");

    controller.connectSavedProfile(profile);
    backend.completeDiscovery();
    QCOMPARE(credentials.requests, 1);

    credentials.reply = {};
    emit backend.authenticationRejected(QStringLiteral("Access denied."));
    backend.setState(OpcUaConnectionState::Disconnected);

    QTRY_COMPARE(credentials.requests, 2);
    QCOMPARE(abortedSpy.size(), 1);
    QCOMPARE(backend.discoveryCalls, 1);
}

void TestConnectionController::discoveryFailureDoesNotConnect()
{
    FakeOpcUaBackend backend;
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    ConnectionController controller(&backend, &secrets, &profiles, &recents);
    QSignalSpy errorSpy(&controller, &ConnectionController::errorOccurred);

    ConnectionProfile profile;
    profile.id = QStringLiteral("failed");
    profile.endpointUrl = QStringLiteral("opc.tcp://localhost:4840");
    controller.connectSavedProfile(profile);
    backend.completeDiscovery(QStringLiteral("discovery failed"));

    QCOMPARE(backend.connectCalls, 0);
    QCOMPARE(errorSpy.size(), 1);
}

void TestConnectionController::savePersistsProfileAndPassword()
{
    FakeOpcUaBackend backend;
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    ConnectionController controller(&backend, &secrets, &profiles, &recents);
    QSignalSpy changedSpy(&controller, &ConnectionController::profilesChanged);

    ConnectionProfile profile;
    profile.id = QStringLiteral("saved");
    controller.saveProfile(profile, QStringLiteral("password"));

    QCOMPARE(profiles.storedProfiles.size(), 1);
    QCOMPARE(secrets.values.value(FakeSecretStore::key(
                 profile.id, SecretStore::Secret::Password)),
             QStringLiteral("password"));
    QCOMPARE(changedSpy.size(), 1);
}

///
/// \brief Remembering a password of a plain connection stores the secret and nothing else.
///
void TestConnectionController::rememberPasswordStoresTheSecretWithoutTheProfile()
{
    FakeOpcUaBackend backend;
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    ConnectionController controller(&backend, &secrets, &profiles, &recents);

    ConnectionProfile profile;
    profile.id = QStringLiteral("plain");
    profile.endpointUrl = QStringLiteral("opc.tcp://localhost:4840");
    profile.authentication = ConnectionProfile::Authentication::Username;

    controller.rememberPassword(profile, QStringLiteral("kept"));

    QCOMPARE(secrets.values.value(FakeSecretStore::key(
                 profile.id, SecretStore::Secret::Password)),
             QStringLiteral("kept"));
    QVERIFY(profiles.storedProfiles.isEmpty());

    // An unchecked box arrives as an empty password and must not clear what is stored.
    controller.rememberPassword(profile, QString());
    QCOMPARE(secrets.values.value(FakeSecretStore::key(
                 profile.id, SecretStore::Secret::Password)),
             QStringLiteral("kept"));
}

void TestConnectionController::savingSameEndpointReplacesFavorite()
{
    FakeOpcUaBackend backend;
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    ConnectionController controller(&backend, &secrets, &profiles, &recents);

    ConnectionProfile first;
    first.id = QStringLiteral("first");
    first.endpointUrl = QStringLiteral("opc.tcp://host:4840");
    first.securityPolicy = QStringLiteral("Basic256Sha256");
    first.securityMode = 3;
    controller.saveProfile(first, QStringLiteral("pw1"));

    ConnectionProfile second;
    second.id = QStringLiteral("second");
    second.endpointUrl = QStringLiteral("opc.tcp://host:4840");
    second.securityPolicy = QStringLiteral("Basic256Sha256");
    second.securityMode = 3;
    controller.saveProfile(second, QString());

    QCOMPARE(profiles.storedProfiles.size(), 1);
    QCOMPARE(profiles.storedProfiles.first().id, QStringLiteral("second"));
    QVERIFY(!secrets.values.contains(
        FakeSecretStore::key(QStringLiteral("first"), SecretStore::Secret::Password)));
}

void TestConnectionController::savingSameEndpointDifferentSecurityKeepsBoth()
{
    FakeOpcUaBackend backend;
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    ConnectionController controller(&backend, &secrets, &profiles, &recents);

    ConnectionProfile signEncrypt;
    signEncrypt.id = QStringLiteral("sign-encrypt");
    signEncrypt.endpointUrl = QStringLiteral("opc.tcp://host:4840");
    signEncrypt.securityPolicy = QStringLiteral("Basic256Sha256");
    signEncrypt.securityMode = 3;
    controller.saveProfile(signEncrypt, QString());

    ConnectionProfile sign;
    sign.id = QStringLiteral("sign");
    sign.endpointUrl = QStringLiteral("opc.tcp://host:4840");
    sign.securityPolicy = QStringLiteral("Aes128_Sha256_RsaOaep");
    sign.securityMode = 2;
    controller.saveProfile(sign, QString());

    QCOMPARE(profiles.storedProfiles.size(), 2);
}

void TestConnectionController::savingSameEndpointDifferentAuthenticationKeepsBoth()
{
    FakeOpcUaBackend backend;
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    ConnectionController controller(&backend, &secrets, &profiles, &recents);

    ConnectionProfile anonymous;
    anonymous.id = QStringLiteral("anonymous");
    anonymous.endpointUrl = QStringLiteral("opc.tcp://host:4840");
    anonymous.securityPolicy = QStringLiteral("Basic256Sha256");
    anonymous.securityMode = 3;
    anonymous.authentication = ConnectionProfile::Authentication::Anonymous;
    controller.saveProfile(anonymous, QString());

    ConnectionProfile username;
    username.id = QStringLiteral("username");
    username.endpointUrl = QStringLiteral("opc.tcp://host:4840");
    username.securityPolicy = QStringLiteral("Basic256Sha256");
    username.securityMode = 3;
    username.authentication = ConnectionProfile::Authentication::Username;
    controller.saveProfile(username, QStringLiteral("pw"));

    QCOMPARE(profiles.storedProfiles.size(), 2);
}

void TestConnectionController::removeFavoriteDeletesProfileAndSecrets()
{
    FakeOpcUaBackend backend;
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    ConnectionController controller(&backend, &secrets, &profiles, &recents);

    ConnectionProfile profile;
    profile.id = QStringLiteral("fav");
    profile.endpointUrl = QStringLiteral("opc.tcp://host:4840");
    controller.saveProfile(profile, QStringLiteral("pw"));
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
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    ConnectionController controller(&backend, &secrets, &profiles, &recents);
    QSignalSpy changedSpy(&controller, &ConnectionController::profilesChanged);

    controller.reorderFavorites({QStringLiteral("b"), QStringLiteral("a")});

    QCOMPARE(profiles.order, (QStringList{QStringLiteral("b"), QStringLiteral("a")}));
    QCOMPARE(changedSpy.size(), 1);
}

void TestConnectionController::reorderFavoritesFailureReportsError()
{
    FakeOpcUaBackend backend;
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    profiles.setOrderSucceeds = false;
    ConnectionController controller(&backend, &secrets, &profiles, &recents);
    QSignalSpy changedSpy(&controller, &ConnectionController::profilesChanged);
    QSignalSpy errorSpy(&controller, &ConnectionController::errorOccurred);

    controller.reorderFavorites({QStringLiteral("a")});

    QCOMPARE(changedSpy.size(), 0);
    QCOMPARE(errorSpy.size(), 1);
}

void TestConnectionController::connectingRecordsRecentConnection()
{
    FakeOpcUaBackend backend;
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    ConnectionController controller(&backend, &secrets, &profiles, &recents);
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
