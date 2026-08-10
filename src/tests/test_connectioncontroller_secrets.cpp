// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_connectioncontroller_secrets.cpp
/// \brief Tests credential storage and secret scoping.
///

#include <QTest>

#include "test_connectioncontroller_support.h"

///
/// \brief Tests the related behavior in an isolated Qt Test executable.
///
class TestConnectionControllerSecrets : public QObject
{
    Q_OBJECT

private slots:
    void rememberedCredentialsAreStoredForTheNextConnection();
    void rememberedCredentialsNeverStoreTheKeyPassword();
    void rememberPasswordStoresTheSecretWithoutTheProfile();
    void secretScopeFollowsTheEndpointIdentity();
    void aProfilePointedAtAnotherServerCannotReadTheStoredPassword();
    void editingAFavoriteEndpointLeavesItsPasswordBehind();
};

///
/// \brief Credentials the user asked to remember reconnect the same profile unattended.
///
void TestConnectionControllerSecrets::rememberedCredentialsAreStoredForTheNextConnection()
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
void TestConnectionControllerSecrets::rememberedCredentialsNeverStoreTheKeyPassword()
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
        FakeSecretStore::key(profile.secretScope(), SecretStore::Secret::Password)));
    QVERIFY(!secrets.values.contains(
        FakeSecretStore::key(profile.secretScope(), SecretStore::Secret::PrivateKeyPassword)));
}

///
/// \brief Remembering a password of a plain connection stores the secret and nothing else.
///
void TestConnectionControllerSecrets::rememberPasswordStoresTheSecretWithoutTheProfile()
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
                 profile.secretScope(), SecretStore::Secret::Password)),
             QStringLiteral("kept"));
    QVERIFY(profiles.storedProfiles.isEmpty());

    // An unchecked box arrives as an empty password and must not clear what is stored.
    controller.rememberPassword(profile, QString());
    QCOMPARE(secrets.values.value(FakeSecretStore::key(
                 profile.secretScope(), SecretStore::Secret::Password)),
             QStringLiteral("kept"));
}

///
/// \brief A secret is named after the endpoint it may be replayed at, not the profile alone.
///
void TestConnectionControllerSecrets::secretScopeFollowsTheEndpointIdentity()
{
    ConnectionProfile profile;
    profile.id = QStringLiteral("scoped");
    profile.endpointUrl = QStringLiteral("opc.tcp://host:4840");
    profile.securityPolicy = QStringLiteral("Basic256Sha256");
    profile.securityMode = 3;
    profile.authentication = ConnectionProfile::Authentication::Username;

    // Two profiles that count as the same favourite must resolve the same secret.
    ConnectionProfile same = profile;
    same.name = QStringLiteral("A different display name");
    same.username = QStringLiteral("operator");
    QVERIFY(profile.isSameEndpoint(same));
    QCOMPARE(same.secretScope(), profile.secretScope());

    ConnectionProfile otherUrl = profile;
    otherUrl.endpointUrl = QStringLiteral("opc.tcp://attacker:4840");
    QVERIFY(otherUrl.secretScope() != profile.secretScope());

    ConnectionProfile otherPolicy = profile;
    otherPolicy.securityPolicy = QStringLiteral("None");
    QVERIFY(otherPolicy.secretScope() != profile.secretScope());

    ConnectionProfile otherMode = profile;
    otherMode.securityMode = 1;
    QVERIFY(otherMode.secretScope() != profile.secretScope());

    ConnectionProfile otherAuthentication = profile;
    otherAuthentication.authentication = ConnectionProfile::Authentication::Anonymous;
    QVERIFY(otherAuthentication.secretScope() != profile.secretScope());

    ConnectionProfile withoutId = profile;
    withoutId.id.clear();
    QVERIFY(withoutId.secretScope().isEmpty());
}

///
/// \brief A session file naming a known profile but a foreign server gets no stored password.
///
void TestConnectionControllerSecrets::aProfilePointedAtAnotherServerCannotReadTheStoredPassword()
{
    FakeOpcUaBackend backend;
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    FakeCredentialsProvider credentials;
    ConnectionController controller(&backend, &secrets, &profiles, &recents);
    controller.setCredentialsProvider(&credentials);

    ConnectionProfile saved;
    saved.id = QStringLiteral("shared-id");
    saved.endpointUrl = QStringLiteral("opc.tcp://trusted:4840");
    saved.authentication = ConnectionProfile::Authentication::Username;
    saved.username = QStringLiteral("operator");
    controller.saveProfile(saved, QStringLiteral("top-secret"));

    // What a tampered session file looks like: the identifier the secret was filed under, and
    // somebody else's server to send it to.
    ConnectionProfile tampered = saved;
    tampered.endpointUrl = QStringLiteral("opc.tcp://attacker:4840");
    credentials.reply.accepted = false;

    controller.connectSavedProfile(tampered);

    QCOMPARE(credentials.requests, 1);
    QCOMPARE(backend.discoveryCalls, 0);
    QVERIFY(backend.connectedPassword.isEmpty());
}

///
/// \brief Repointing a favourite at another server does not carry its password along.
///
void TestConnectionControllerSecrets::editingAFavoriteEndpointLeavesItsPasswordBehind()
{
    FakeOpcUaBackend backend;
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    FakeCredentialsProvider credentials;
    ConnectionController controller(&backend, &secrets, &profiles, &recents);
    controller.setCredentialsProvider(&credentials);

    ConnectionProfile favorite;
    favorite.id = QStringLiteral("favorite");
    favorite.endpointUrl = QStringLiteral("opc.tcp://trusted:4840");
    favorite.authentication = ConnectionProfile::Authentication::Username;
    controller.saveProfile(favorite, QStringLiteral("top-secret"));

    ConnectionProfile moved = favorite;
    moved.endpointUrl = QStringLiteral("opc.tcp://elsewhere:4840");
    controller.saveProfile(moved, QString());

    credentials.reply.accepted = false;
    controller.connectSavedProfile(moved);

    QCOMPARE(credentials.requests, 1);
    QVERIFY(backend.connectedPassword.isEmpty());
    // The original secret is untouched: the favourite it belonged to can still be reached by
    // pointing a profile back at that endpoint.
    QCOMPARE(secrets.values.value(
                 FakeSecretStore::key(favorite.secretScope(), SecretStore::Secret::Password)),
             QStringLiteral("top-secret"));
}

QTEST_GUILESS_MAIN(TestConnectionControllerSecrets)

#include "test_connectioncontroller_secrets.moc"
