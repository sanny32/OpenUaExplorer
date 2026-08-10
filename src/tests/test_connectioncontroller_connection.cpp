// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_connectioncontroller_connection.cpp
/// \brief Tests connection, discovery, and credential request flows.
///

#include <QTest>

#include "test_connectioncontroller_support.h"

///
/// \brief Tests the related behavior in an isolated Qt Test executable.
///
class TestConnectionControllerConnection : public QObject
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
    void missingCertificateFileIsAskedForAgain();
    void rejectedCredentialsAreAskedForAgain();
    void decliningAfterARejectionStopsRetrying();
    void discoveryFailureDoesNotConnect();
};

///
/// \brief A backend that does not implement FindServers reports the operation as unsupported.
///
void TestConnectionControllerConnection::backendReportsFindServersUnsupported()
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
void TestConnectionControllerConnection::connectingAppliesTheProfileRequestTimeout()
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
void TestConnectionControllerConnection::connectingAddsStableInstanceSuffixToSessionNames()
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

void TestConnectionControllerConnection::savedProfileWithoutSecretsDiscoversThenConnects()
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
void TestConnectionControllerConnection::savedProfileLoadsOnlyTheUserPassword()
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
        FakeSecretStore::key(profile.secretScope(), SecretStore::Secret::Password),
        QStringLiteral("login-secret"));
    // Left over from an older version that stored it; it must not reach the backend.
    secrets.values.insert(
        FakeSecretStore::key(profile.secretScope(), SecretStore::Secret::PrivateKeyPassword),
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
void TestConnectionControllerConnection::storedPasswordConnectsWithoutPrompting()
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
    secrets.values.insert(FakeSecretStore::key(profile.secretScope(), SecretStore::Secret::Password),
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
void TestConnectionControllerConnection::missingPasswordIsAskedForBeforeConnecting()
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
void TestConnectionControllerConnection::decliningTheCredentialsPromptAbortsTheConnection()
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
/// \brief A certificate the profile names but the machine no longer has is asked for again.
///
void TestConnectionControllerConnection::missingCertificateFileIsAskedForAgain()
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
void TestConnectionControllerConnection::rejectedCredentialsAreAskedForAgain()
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
    secrets.values.insert(FakeSecretStore::key(profile.secretScope(), SecretStore::Secret::Password),
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
void TestConnectionControllerConnection::decliningAfterARejectionStopsRetrying()
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

void TestConnectionControllerConnection::discoveryFailureDoesNotConnect()
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

QTEST_GUILESS_MAIN(TestConnectionControllerConnection)

#include "test_connectioncontroller_connection.moc"
