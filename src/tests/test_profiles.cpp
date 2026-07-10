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
#include "opcua/recentconnectionstore.h"

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

    void recentStoreIsEmptyInitially();
    void recentRecordsMostRecentFirst();
    void recentDeduplicatesByEndpointUrl();
    void recentIgnoresEmptyEndpointUrl();
    void recentCapsAtMaximumSize();
    void recentRoundTripsFields();

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

///
/// \brief A fresh store reports no recent connections.
///
void TestProfiles::recentStoreIsEmptyInitially()
{
    RecentConnectionStore store;
    QVERIFY(store.connections().isEmpty());
}

///
/// \brief record() prepends, so the newest connection is listed first.
///
void TestProfiles::recentRecordsMostRecentFirst()
{
    RecentConnectionStore store;
    ConnectionProfile first = makeProfile(QStringLiteral("a"), QStringLiteral("A"));
    first.endpointUrl = QStringLiteral("opc.tcp://host-a:4840");
    ConnectionProfile second = makeProfile(QStringLiteral("b"), QStringLiteral("B"));
    second.endpointUrl = QStringLiteral("opc.tcp://host-b:4840");

    store.record(first);
    store.record(second);

    const QList<ConnectionProfile> recent = store.connections();
    QCOMPARE(recent.size(), 2);
    QCOMPARE(recent.at(0).endpointUrl, second.endpointUrl);
    QCOMPARE(recent.at(1).endpointUrl, first.endpointUrl);
}

///
/// \brief Recording the same endpoint again moves it to the front without duplicating.
///
void TestProfiles::recentDeduplicatesByEndpointUrl()
{
    RecentConnectionStore store;
    ConnectionProfile a = makeProfile(QStringLiteral("a"), QStringLiteral("A"));
    a.endpointUrl = QStringLiteral("opc.tcp://host-a:4840");
    ConnectionProfile b = makeProfile(QStringLiteral("b"), QStringLiteral("B"));
    b.endpointUrl = QStringLiteral("opc.tcp://host-b:4840");

    store.record(a);
    store.record(b);
    store.record(a); // same endpoint URL as the first entry

    const QList<ConnectionProfile> recent = store.connections();
    QCOMPARE(recent.size(), 2);
    QCOMPARE(recent.at(0).endpointUrl, a.endpointUrl);
    QCOMPARE(recent.at(1).endpointUrl, b.endpointUrl);
}

///
/// \brief A profile without an endpoint URL is not recorded.
///
void TestProfiles::recentIgnoresEmptyEndpointUrl()
{
    RecentConnectionStore store;
    ConnectionProfile profile = makeProfile(QStringLiteral("a"), QStringLiteral("A"));
    profile.endpointUrl.clear();

    store.record(profile);

    QVERIFY(store.connections().isEmpty());
}

///
/// \brief The recent list is trimmed to RecentConnectionStore::maximumSize.
///
void TestProfiles::recentCapsAtMaximumSize()
{
    RecentConnectionStore store;
    for (int index = 0; index < RecentConnectionStore::maximumSize + 5; ++index) {
        ConnectionProfile profile =
            makeProfile(QStringLiteral("id%1").arg(index), QStringLiteral("Name%1").arg(index));
        profile.endpointUrl = QStringLiteral("opc.tcp://host-%1:4840").arg(index);
        store.record(profile);
    }

    const QList<ConnectionProfile> recent = store.connections();
    QCOMPARE(recent.size(), RecentConnectionStore::maximumSize);
    // The most recently recorded endpoint survives at the front.
    QCOMPARE(recent.first().endpointUrl,
             QStringLiteral("opc.tcp://host-%1:4840")
                 .arg(RecentConnectionStore::maximumSize + 4));
}

///
/// \brief All persisted profile fields survive a record/read round-trip.
///
void TestProfiles::recentRoundTripsFields()
{
    RecentConnectionStore store;
    ConnectionProfile profile = makeProfile(QStringLiteral("id"), QStringLiteral("Name"));
    profile.endpointUrl = QStringLiteral("opc.tcp://host:4840");
    profile.sessionName = QStringLiteral("session");
    profile.backend = QStringLiteral("open62541");
    profile.securityMode = 3;
    profile.authentication = ConnectionProfile::Authentication::Username;
    profile.username = QStringLiteral("user");
    profile.sessionTimeoutMs = 123456;
    profile.requestTimeoutMs = 4321;
    profile.lastUsed = QDateTime::fromMSecsSinceEpoch(1700000000000LL);

    store.record(profile);

    const QList<ConnectionProfile> recent = store.connections();
    QCOMPARE(recent.size(), 1);
    const ConnectionProfile &read = recent.first();
    QCOMPARE(read.id, profile.id);
    QCOMPARE(read.name, profile.name);
    QCOMPARE(read.sessionName, profile.sessionName);
    QCOMPARE(read.securityMode, profile.securityMode);
    QCOMPARE(read.authentication, profile.authentication);
    QCOMPARE(read.username, profile.username);
    QCOMPARE(read.sessionTimeoutMs, profile.sessionTimeoutMs);
    QCOMPARE(read.requestTimeoutMs, profile.requestTimeoutMs);
    QCOMPARE(read.lastUsed, profile.lastUsed);
}

QTEST_MAIN(TestProfiles)

#include "test_profiles.moc"
