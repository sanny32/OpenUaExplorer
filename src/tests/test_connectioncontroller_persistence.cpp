// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_connectioncontroller_persistence.cpp
/// \brief Tests profile, favorite, and recent-connection persistence.
///

#include <QTest>

#include "test_connectioncontroller_support.h"

///
/// \brief Tests the related behavior in an isolated Qt Test executable.
///
class TestConnectionControllerPersistence : public QObject
{
    Q_OBJECT

private slots:
    void savePersistsProfileAndPassword();
    void savingSameEndpointReplacesFavorite();
    void savingSameEndpointDifferentSecurityKeepsBoth();
    void savingSameEndpointDifferentAuthenticationKeepsBoth();
    void removeFavoriteDeletesProfileAndSecrets();
    void reorderFavoritesPersistsOrderAndNotifies();
    void reorderFavoritesFailureReportsError();
    void connectingRecordsRecentConnection();
};

void TestConnectionControllerPersistence::savePersistsProfileAndPassword()
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
                 profile.secretScope(), SecretStore::Secret::Password)),
             QStringLiteral("password"));
    QCOMPARE(changedSpy.size(), 1);
}


void TestConnectionControllerPersistence::savingSameEndpointReplacesFavorite()
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
        FakeSecretStore::key(first.secretScope(), SecretStore::Secret::Password)));
}

void TestConnectionControllerPersistence::savingSameEndpointDifferentSecurityKeepsBoth()
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

void TestConnectionControllerPersistence::savingSameEndpointDifferentAuthenticationKeepsBoth()
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

void TestConnectionControllerPersistence::removeFavoriteDeletesProfileAndSecrets()
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
        FakeSecretStore::key(profile.secretScope(), SecretStore::Secret::Password)));
    QCOMPARE(changedSpy.size(), 1);
}

void TestConnectionControllerPersistence::reorderFavoritesPersistsOrderAndNotifies()
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

void TestConnectionControllerPersistence::reorderFavoritesFailureReportsError()
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

void TestConnectionControllerPersistence::connectingRecordsRecentConnection()
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


QTEST_GUILESS_MAIN(TestConnectionControllerPersistence)

#include "test_connectioncontroller_persistence.moc"
