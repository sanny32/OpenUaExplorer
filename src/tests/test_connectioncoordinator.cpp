// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_connectioncoordinator.cpp
/// \brief Unit tests for ConnectionCoordinator using fake connection dependencies.
///

#include <algorithm>

#include <QAction>
#include <QHash>
#include <QMenu>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <QToolButton>

#include "application.h"
#include "connectioncoordinator.h"
#include "favoritescoordinator.h"
#include "opcua/connectioncontroller.h"
#include "opcua/connectionprofilestore.h"
#include "opcua/opcuabackend.h"
#include "opcua/recentconnectionstore.h"

///
/// \brief Backend double that records connection calls and drives discovery manually.
///
class ConnectionFakeBackend : public OpcUaBackend
{
    Q_OBJECT

public:
    using OpcUaBackend::OpcUaBackend;

    bool isAvailable() const override { return true; }
    QStringList availableBackends() const override { return {QStringLiteral("fake")}; }
    OpcUaConnectionState state() const override { return currentState; }
    QString lastError() const override { return {}; }
    void setCertificateTrustDecider(CertificateTrustDecider *) override {}
    void discoverEndpoints(const QString &, const QString &, int) override { ++discoveryCalls; }
    void connectToEndpoint(const ConnectionProfile &, const QString &,
                           const QString &) override { ++connectCalls; }
    void disconnectFromEndpoint() override { ++disconnectCalls; }
    void browse(const QString &) override {}
    void browseReferences(const QString &) override {}
    void readNode(const QString &) override {}
    void readValues(const QStringList &) override {}
    void writeValue(const QString &, const QVariant &, int) override {}

    void completeDiscovery() { emit endpointsDiscovered({}, QString()); }

    void setState(OpcUaConnectionState state)
    {
        currentState = state;
        emit stateChanged(state);
    }

    OpcUaConnectionState currentState = OpcUaConnectionState::Disconnected;
    int discoveryCalls = 0;
    int connectCalls = 0;
    int disconnectCalls = 0;
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
        emit readFinished(profileId, secret, values.value(key(profileId, secret)), {});
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
        return profileId + QLatin1Char('/') + QString::number(static_cast<int>(secret));
    }

    QHash<QString, QString> values;
};

///
/// \brief Profile store double that keeps saved profiles in memory.
///
class FakeProfileStore : public ConnectionProfileStore
{
public:
    QList<ConnectionProfile> profiles() const override { return storedProfiles; }

    bool save(const ConnectionProfile &profile) override
    {
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

    QList<ConnectionProfile> storedProfiles;
};

///
/// \brief Recent-connection store double that keeps the history in memory.
///
class FakeRecentStore : public RecentConnectionStore
{
public:
    QList<ConnectionProfile> connections() const override { return recent; }

    void record(const ConnectionProfile &profile) override { recent.prepend(profile); }

    QList<ConnectionProfile> recent;
};

///
/// \brief Bundles the coordinator with its widgets, controller, and fake backend.
///
struct ConnectionHarness
{
    ConnectionHarness()
        : controller(&backend, &secrets, &profiles, &recents)
        , recentMenu(&window)
        , favoritesButton(&window)
    {
        actions.connect = newAction();
        actions.newConnection = newAction();
        actions.disconnect = newAction();
        actions.browse = newAction();
        actions.browseAddressSpace = newAction();
        actions.refresh = newAction();
        actions.endpointSettings = newAction();

        coordinator = new ConnectionCoordinator(&controller, &backend, &recentMenu,
                                                &favoritesButton, actions, &window);
    }

    QAction *newAction() { return new QAction(&window); }

    QWidget window;
    ConnectionFakeBackend backend;
    FakeSecretStore secrets;
    FakeProfileStore profiles;
    FakeRecentStore recents;
    ConnectionController controller;
    QMenu recentMenu;
    QToolButton favoritesButton;
    ConnectionActions actions;
    ConnectionCoordinator *coordinator = nullptr;
};

///
/// \brief Verifies the coordinator's action enabling, recents menu, and favourites flow.
///
class TestConnectionCoordinator : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void actionsFollowClientState();
    void emptyHistoryShowsDisabledPlaceholder();
    void recentMenuListsHistoryByNameOrUrl();
    void recentActionConnectsItsProfile();
    void addFavoriteRequestSavesActiveProfile();

private:
    QTemporaryDir _settingsDirectory;
};

///
/// \brief Routes QSettings to a throwaway directory so tests never touch real configuration.
///
void TestConnectionCoordinator::initTestCase()
{
    QVERIFY(_settingsDirectory.isValid());
    QCoreApplication::setOrganizationName(QStringLiteral("OpenUaExplorerTests"));
    QCoreApplication::setApplicationName(QStringLiteral("OpenUaExplorerTests"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       _settingsDirectory.path());
}

///
/// \brief Clears stored settings between tests to keep them independent.
///
void TestConnectionCoordinator::cleanup()
{
    QSettings settings;
    settings.clear();
}

///
/// \brief The connect actions follow idle and the server actions follow connected.
///
void TestConnectionCoordinator::actionsFollowClientState()
{
    ConnectionHarness harness;

    QVERIFY(harness.actions.connect->isEnabled());
    QVERIFY(harness.actions.newConnection->isEnabled());
    QVERIFY(!harness.actions.disconnect->isEnabled());
    QVERIFY(!harness.actions.browse->isEnabled());
    QVERIFY(!harness.actions.browseAddressSpace->isEnabled());
    QVERIFY(!harness.actions.refresh->isEnabled());
    QVERIFY(!harness.actions.endpointSettings->isEnabled());

    harness.backend.setState(OpcUaConnectionState::Connected);

    QVERIFY(!harness.actions.connect->isEnabled());
    QVERIFY(!harness.actions.newConnection->isEnabled());
    QVERIFY(harness.actions.disconnect->isEnabled());
    QVERIFY(harness.actions.browse->isEnabled());
    QVERIFY(harness.actions.browseAddressSpace->isEnabled());
    QVERIFY(harness.actions.refresh->isEnabled());
    QVERIFY(harness.actions.endpointSettings->isEnabled());
}

///
/// \brief An empty history leaves a single disabled placeholder entry in the menu.
///
void TestConnectionCoordinator::emptyHistoryShowsDisabledPlaceholder()
{
    ConnectionHarness harness;

    const QList<QAction *> entries = harness.recentMenu.actions();
    QCOMPARE(entries.size(), 1);
    QVERIFY(!entries.constFirst()->isEnabled());
}

///
/// \brief Recent profiles appear as menu entries labelled by name, or URL when unnamed.
///
void TestConnectionCoordinator::recentMenuListsHistoryByNameOrUrl()
{
    ConnectionHarness harness;

    ConnectionProfile named;
    named.id = QStringLiteral("named");
    named.name = QStringLiteral("Lab Server");
    named.endpointUrl = QStringLiteral("opc.tcp://lab:4840");
    ConnectionProfile unnamed;
    unnamed.id = QStringLiteral("unnamed");
    unnamed.endpointUrl = QStringLiteral("opc.tcp://plant:4840");
    harness.recents.recent = {named, unnamed};
    emit harness.controller.recentsChanged();

    const QList<QAction *> entries = harness.recentMenu.actions();
    QCOMPARE(entries.size(), 2);
    QCOMPARE(entries.at(0)->text(), QStringLiteral("Lab Server"));
    QCOMPARE(entries.at(1)->text(), QStringLiteral("opc.tcp://plant:4840"));
    QVERIFY(entries.at(0)->isEnabled());
}

///
/// \brief Triggering a recent entry starts connecting to exactly that profile.
///
void TestConnectionCoordinator::recentActionConnectsItsProfile()
{
    ConnectionHarness harness;

    ConnectionProfile profile;
    profile.id = QStringLiteral("recent");
    profile.name = QStringLiteral("Recent Server");
    profile.endpointUrl = QStringLiteral("opc.tcp://recent:4840");
    profile.backend = QStringLiteral("fake");
    harness.recents.recent = {profile};
    emit harness.controller.recentsChanged();

    harness.recentMenu.actions().constFirst()->trigger();

    QCOMPARE(harness.backend.discoveryCalls, 1);
}

///
/// \brief A favourites add request saves the active profile as a favourite.
///
void TestConnectionCoordinator::addFavoriteRequestSavesActiveProfile()
{
    ConnectionHarness harness;

    ConnectionProfile profile;
    profile.id = QStringLiteral("active");
    profile.name = QStringLiteral("Active Server");
    profile.endpointUrl = QStringLiteral("opc.tcp://active:4840");
    profile.backend = QStringLiteral("fake");
    harness.controller.connectSavedProfile(profile);
    harness.backend.completeDiscovery();

    auto *favorites = harness.window.findChild<FavoritesCoordinator *>();
    QVERIFY(favorites);
    emit favorites->addFavoriteRequested();

    const QList<ConnectionProfile> saved = harness.controller.profiles();
    QCOMPARE(saved.size(), 1);
    QCOMPARE(saved.constFirst().endpointUrl, profile.endpointUrl);
    QVERIFY(saved.constFirst().saveProfile);
}

///
/// \brief Runs the suite under a real Application so theApp() is available.
/// \param argc Argument count.
/// \param argv Argument vector.
/// \return Test exit code.
///
int main(int argc, char *argv[])
{
    Application app(argc, argv);
    TestConnectionCoordinator test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_connectioncoordinator.moc"
