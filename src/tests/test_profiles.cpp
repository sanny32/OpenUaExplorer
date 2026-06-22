// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_profiles.cpp
/// \brief Tests CRUD behaviour and edge cases of ConnectionProfileStore.
///

#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

#include "opcua/connectionprofilestore.h"

///
/// \brief Unit tests for persistent connection profile storage.
///
class TestProfiles : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void saveRejectsEmptyId();
    void savedProfileReportsSaveProfileTrue();
    void saveThenUpdateSameIdReplacesProfile();
    void removeDeletesOnlyTargetProfile();
    void removeUnknownIdIsNoOp();
    void allFieldsRoundTrip();
    void removingProfileClearsSettingsKeys();
    void setOrderControlsProfileOrder();
    void unorderedProfilesFollowOrderedOnes();
    void removingProfilePrunesStoredOrder();

private:
    static ConnectionProfile makeProfile(const QString &id, const QString &name);

    QTemporaryDir _settingsDirectory;
};

///
/// \brief Routes QSettings to a throwaway directory so tests never touch the
///        real user configuration.
///
void TestProfiles::initTestCase()
{
    QVERIFY(_settingsDirectory.isValid());
    QCoreApplication::setOrganizationName(QStringLiteral("OpenUaExplorerTests"));
    QCoreApplication::setApplicationName(QStringLiteral("OpenUaExplorerTests"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       _settingsDirectory.path());
}

///
/// \brief Clears all stored settings between tests to keep them independent.
///
void TestProfiles::cleanup()
{
    QSettings settings;
    settings.clear();
    settings.sync();
}

///
/// \brief Builds a profile with a few representative non-default fields.
///
ConnectionProfile TestProfiles::makeProfile(const QString &id, const QString &name)
{
    ConnectionProfile profile;
    profile.id = id;
    profile.name = name;
    profile.endpointUrl = QStringLiteral("opc.tcp://localhost:4840");
    profile.securityPolicy =
        QStringLiteral("http://opcfoundation.org/UA/SecurityPolicy#None");
    return profile;
}

///
/// \brief save() refuses a profile without an identifier.
///
void TestProfiles::saveRejectsEmptyId()
{
    ConnectionProfileStore store;
    ConnectionProfile profile = makeProfile(QString(), QStringLiteral("No id"));
    QVERIFY(!store.save(profile));
    QVERIFY(store.profiles().isEmpty());
}

///
/// \brief Profiles read back from the store always advertise saveProfile = true.
///
void TestProfiles::savedProfileReportsSaveProfileTrue()
{
    ConnectionProfileStore store;
    ConnectionProfile profile = makeProfile(QStringLiteral("p1"), QStringLiteral("One"));
    profile.saveProfile = false; // the store persists regardless of the flag
    QVERIFY(store.save(profile));

    const QList<ConnectionProfile> profiles = store.profiles();
    QCOMPARE(profiles.size(), 1);
    QVERIFY(profiles.first().saveProfile);
}

///
/// \brief Saving twice with the same id replaces rather than duplicates.
///
void TestProfiles::saveThenUpdateSameIdReplacesProfile()
{
    ConnectionProfileStore store;
    QVERIFY(store.save(makeProfile(QStringLiteral("p1"), QStringLiteral("Original"))));

    ConnectionProfile updated = makeProfile(QStringLiteral("p1"), QStringLiteral("Renamed"));
    updated.endpointUrl = QStringLiteral("opc.tcp://server:4841");
    QVERIFY(store.save(updated));

    const QList<ConnectionProfile> profiles = store.profiles();
    QCOMPARE(profiles.size(), 1);
    QCOMPARE(profiles.first().name, QStringLiteral("Renamed"));
    QCOMPARE(profiles.first().endpointUrl, QStringLiteral("opc.tcp://server:4841"));
}

///
/// \brief remove() deletes only the targeted profile and leaves the rest intact.
///
void TestProfiles::removeDeletesOnlyTargetProfile()
{
    ConnectionProfileStore store;
    QVERIFY(store.save(makeProfile(QStringLiteral("p1"), QStringLiteral("One"))));
    QVERIFY(store.save(makeProfile(QStringLiteral("p2"), QStringLiteral("Two"))));
    QVERIFY(store.save(makeProfile(QStringLiteral("p3"), QStringLiteral("Three"))));

    QVERIFY(store.remove(QStringLiteral("p2")));

    QStringList ids;
    for (const ConnectionProfile &profile : store.profiles())
        ids.append(profile.id);
    ids.sort();
    QCOMPARE(ids, (QStringList{QStringLiteral("p1"), QStringLiteral("p3")}));
}

///
/// \brief Removing an unknown id succeeds without disturbing existing profiles.
///
void TestProfiles::removeUnknownIdIsNoOp()
{
    ConnectionProfileStore store;
    QVERIFY(store.save(makeProfile(QStringLiteral("p1"), QStringLiteral("One"))));

    QVERIFY(store.remove(QStringLiteral("does-not-exist")));
    QCOMPARE(store.profiles().size(), 1);
    QCOMPARE(store.profiles().first().id, QStringLiteral("p1"));
}

///
/// \brief Every persisted field survives a save/read round trip.
///
void TestProfiles::allFieldsRoundTrip()
{
    ConnectionProfile profile = makeProfile(QStringLiteral("full"), QStringLiteral("Full"));
    profile.sessionName = QStringLiteral("Operator Session");
    profile.backend = QStringLiteral("uacpp");
    profile.securityPolicy =
        QStringLiteral("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");
    profile.securityMode = 3;
    profile.authentication = ConnectionProfile::Authentication::Certificate;
    profile.username = QStringLiteral("operator");
    profile.clientCertificateFile = QStringLiteral("/pki/own/certs/client.der");
    profile.privateKeyFile = QStringLiteral("/pki/own/private/client.pem");
    profile.sessionTimeoutMs = 120000;
    profile.connectTimeoutMs = 5000;
    profile.secureChannelLifetimeMs = 300000;
    profile.endpointTimeoutMs = 8000;
    profile.requestTimeoutMs = 20000;
    profile.maxMessageSizeBytes = 2097152;

    ConnectionProfileStore store;
    QVERIFY(store.save(profile));

    const QList<ConnectionProfile> profiles = store.profiles();
    QCOMPARE(profiles.size(), 1);
    const ConnectionProfile &read = profiles.first();
    QCOMPARE(read.id, profile.id);
    QCOMPARE(read.name, profile.name);
    QCOMPARE(read.sessionName, profile.sessionName);
    QCOMPARE(read.backend, profile.backend);
    QCOMPARE(read.endpointUrl, profile.endpointUrl);
    QCOMPARE(read.securityPolicy, profile.securityPolicy);
    QCOMPARE(read.securityMode, profile.securityMode);
    QCOMPARE(read.authentication, profile.authentication);
    QCOMPARE(read.username, profile.username);
    QCOMPARE(read.clientCertificateFile, profile.clientCertificateFile);
    QCOMPARE(read.privateKeyFile, profile.privateKeyFile);
    QCOMPARE(read.sessionTimeoutMs, profile.sessionTimeoutMs);
    QCOMPARE(read.connectTimeoutMs, profile.connectTimeoutMs);
    QCOMPARE(read.secureChannelLifetimeMs, profile.secureChannelLifetimeMs);
    QCOMPARE(read.endpointTimeoutMs, profile.endpointTimeoutMs);
    QCOMPARE(read.requestTimeoutMs, profile.requestTimeoutMs);
    QCOMPARE(read.maxMessageSizeBytes, profile.maxMessageSizeBytes);
}

///
/// \brief After removal no settings keys for the profile linger behind.
///
void TestProfiles::removingProfileClearsSettingsKeys()
{
    ConnectionProfileStore store;
    QVERIFY(store.save(makeProfile(QStringLiteral("p1"), QStringLiteral("One"))));

    {
        QSettings settings;
        QVERIFY(!settings.allKeys().filter(QStringLiteral("p1")).isEmpty());
    }

    QVERIFY(store.remove(QStringLiteral("p1")));

    QSettings settings;
    QVERIFY(settings.allKeys().filter(QStringLiteral("p1")).isEmpty());
}

///
/// \brief setOrder() defines the sequence profiles() returns its entries in.
///
void TestProfiles::setOrderControlsProfileOrder()
{
    ConnectionProfileStore store;
    QVERIFY(store.save(makeProfile(QStringLiteral("p1"), QStringLiteral("One"))));
    QVERIFY(store.save(makeProfile(QStringLiteral("p2"), QStringLiteral("Two"))));
    QVERIFY(store.save(makeProfile(QStringLiteral("p3"), QStringLiteral("Three"))));

    QVERIFY(store.setOrder(
        {QStringLiteral("p3"), QStringLiteral("p1"), QStringLiteral("p2")}));

    QStringList ids;
    for (const ConnectionProfile &profile : store.profiles())
        ids.append(profile.id);
    QCOMPARE(ids, (QStringList{QStringLiteral("p3"), QStringLiteral("p1"),
                               QStringLiteral("p2")}));
}

///
/// \brief Profiles missing from the order list keep their position and sort last.
///
void TestProfiles::unorderedProfilesFollowOrderedOnes()
{
    ConnectionProfileStore store;
    QVERIFY(store.save(makeProfile(QStringLiteral("p1"), QStringLiteral("One"))));
    QVERIFY(store.save(makeProfile(QStringLiteral("p2"), QStringLiteral("Two"))));
    QVERIFY(store.save(makeProfile(QStringLiteral("p3"), QStringLiteral("Three"))));

    // Only p3 and p1 are ordered; p2 represents a freshly added favourite.
    QVERIFY(store.setOrder({QStringLiteral("p3"), QStringLiteral("p1")}));

    const QList<ConnectionProfile> profiles = store.profiles();
    QCOMPARE(profiles.size(), 3);
    QCOMPARE(profiles.at(0).id, QStringLiteral("p3"));
    QCOMPARE(profiles.at(1).id, QStringLiteral("p1"));
    QCOMPARE(profiles.at(2).id, QStringLiteral("p2"));
}

///
/// \brief Removing a profile drops its id from the stored order list.
///
void TestProfiles::removingProfilePrunesStoredOrder()
{
    ConnectionProfileStore store;
    QVERIFY(store.save(makeProfile(QStringLiteral("p1"), QStringLiteral("One"))));
    QVERIFY(store.save(makeProfile(QStringLiteral("p2"), QStringLiteral("Two"))));
    QVERIFY(store.setOrder({QStringLiteral("p1"), QStringLiteral("p2")}));

    QVERIFY(store.remove(QStringLiteral("p1")));

    QSettings settings;
    const QStringList order =
        settings.value(QStringLiteral("opcua/favoritesOrder")).toStringList();
    QCOMPARE(order, QStringList{QStringLiteral("p2")});
}

QTEST_MAIN(TestProfiles)

#include "test_profiles.moc"
