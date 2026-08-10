// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_sessioncoordinator_startup.cpp
/// \brief Tests SessionCoordinator startup, opening, and restoration priority.
///

#include <QTest>

#include "test_sessioncoordinator_support.h"

///
/// \brief Tests the related behavior in an isolated Qt Test executable.
///
class TestSessionCoordinatorStartup : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void cleanup();
    void openSessionUsesWaitCursorUntilConnectionFails();
    void usernameSessionConnectsWithoutTheConnectionDialog();
    void autosavedSessionIsStagedButNotConnected();
    void autosavedSessionIsSkippedWhenDisabled();
    void savedSessionTakesPriorityOverAutosaveAtStartup();
    void missingSavedSessionFallsBackToAutosave();
    void autosavedSessionIsAppliedOnMatchingEndpoint();
    void autosavedSessionIsKeptForDifferentEndpoint();
    void stagedSessionReconnectsAtStartup();
    void commandLineSessionKeepsPriorityOverAutosave();

private:
    QString writeAutosavedSession();
    QTemporaryDir _settingsDirectory;
};

///
/// \brief Writes a minimal workspace to the autosave path the coordinator reads from.
/// \return The autosave file path.
///
QString TestSessionCoordinatorStartup::writeAutosavedSession()
{
    // Restoring is opt-in, so every staging test has to turn it on.
    AppSettings().setRestoreLastSessionOnStartup(true);

    const QString directory =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(directory);
    const QString path = directory + QStringLiteral("/lastsession.ouas");

    SessionData session;
    session.profile.id = QStringLiteral("autosaved-profile");
    session.profile.name = QStringLiteral("Autosaved");
    session.profile.endpointUrl = QStringLiteral("opc.tcp://autosaved.invalid:4840");
    session.profile.authentication = ConnectionProfile::Authentication::Anonymous;

    // A name that does not collide with a built-in subscription, which would be skipped.
    SubscriptionItem subscription;
    subscription.name = QStringLiteral("Telemetry");
    subscription.publishingInterval = 250.0;
    session.subscriptions.append(subscription);
    session.dataAccessNodes.append({QStringLiteral("ns=2;s=Temp"), QStringLiteral("Telemetry")});
    return SessionStore::save(path, session) ? path : QString();
}

void TestSessionCoordinatorStartup::initTestCase()
{
    QVERIFY(_settingsDirectory.isValid());
    QCoreApplication::setOrganizationName(QStringLiteral("TestSessionCoordinatorStartup"));
    QCoreApplication::setApplicationName(QStringLiteral("TestSessionCoordinatorStartup"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       _settingsDirectory.path());
    // Keeps the autosave file out of the real application data directory.
    QStandardPaths::setTestModeEnabled(true);
}

void TestSessionCoordinatorStartup::cleanupTestCase()
{
    QStandardPaths::setTestModeEnabled(false);
}

void TestSessionCoordinatorStartup::cleanup()
{
    while (QGuiApplication::overrideCursor())
        QGuiApplication::restoreOverrideCursor();
    SettingsStore settings;
    settings.clear();
    QFile::remove(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                  + QStringLiteral("/lastsession.ouas"));
}


void TestSessionCoordinatorStartup::openSessionUsesWaitCursorUntilConnectionFails()
{
    QTemporaryDir sessionsDirectory;
    QVERIFY(sessionsDirectory.isValid());
    const QString path = sessionsDirectory.filePath(QStringLiteral("waiting.ouas"));

    SessionData session;
    session.profile.id = QStringLiteral("waiting-profile");
    session.profile.name = QStringLiteral("Waiting");
    session.profile.endpointUrl = QStringLiteral("opc.tcp://waiting.invalid:4840");
    session.profile.authentication = ConnectionProfile::Authentication::Anonymous;
    QVERIFY(SessionStore::save(path, session));

    SessionFakeBackend backend;
    FakeSecretStore secrets;
    ConnectionProfileStore profiles;
    RecentConnectionStore recents;
    ConnectionController controller(&backend, &secrets, &profiles, &recents);
    QWidget window;
    QMenu recentSessionsMenu(&window);

    SessionCoordinatorContext context;
    context.window = &window;
    context.recentSessionsMenu = &recentSessionsMenu;
    context.connectionController = &controller;
    context.backend = &backend;
    SessionCoordinator coordinator(context);

    QVERIFY(!QGuiApplication::overrideCursor());
    coordinator.openSessionFromFile(path);

    QVERIFY(QGuiApplication::overrideCursor());
    QCOMPARE(QGuiApplication::overrideCursor()->shape(), Qt::WaitCursor);

    backend.setState(OpcUaConnectionState::Disconnected);
    QVERIFY(!QGuiApplication::overrideCursor());
}

///
/// \brief A session authenticating with a username reconnects on its own.
///
/// The credentials come from the credential store, so opening the file starts the connection
/// straight away instead of handing the user the connection dialog.
///
void TestSessionCoordinatorStartup::usernameSessionConnectsWithoutTheConnectionDialog()
{
    QTemporaryDir sessionsDirectory;
    QVERIFY(sessionsDirectory.isValid());
    const QString path = sessionsDirectory.filePath(QStringLiteral("credentials.ouas"));
    const QString endpoint = QStringLiteral("opc.tcp://credentials.invalid:4840");

    SessionData session;
    session.profile.id = QStringLiteral("credentials-profile");
    session.profile.endpointUrl = endpoint;
    session.profile.authentication = ConnectionProfile::Authentication::Username;
    session.profile.username = QStringLiteral("operator");
    QVERIFY(SessionStore::save(path, session));

    WorkspaceHarness harness;
    harness.coordinator->openSessionFromFile(path);

    QVERIFY(harness.coordinator->hasPendingSession());
    QTRY_COMPARE(harness.backend.state(), OpcUaConnectionState::Discovering);
    QCOMPARE(harness.backend.lastDiscoveryUrl, endpoint);
}

///
/// \brief The autosaved workspace is staged at startup without driving a connection.
///
void TestSessionCoordinatorStartup::autosavedSessionIsStagedButNotConnected()
{
    QVERIFY(!writeAutosavedSession().isEmpty());

    SessionFakeBackend backend;
    FakeSecretStore secrets;
    ConnectionProfileStore profiles;
    RecentConnectionStore recents;
    ConnectionController controller(&backend, &secrets, &profiles, &recents);
    QWidget window;
    QMenu recentSessionsMenu(&window);

    SessionCoordinatorContext context;
    context.window = &window;
    context.recentSessionsMenu = &recentSessionsMenu;
    context.connectionController = &controller;
    context.backend = &backend;
    SessionCoordinator coordinator(context);

    coordinator.stageLastSession();

    QVERIFY(coordinator.hasPendingSession());
    // Staging must not touch the connection, the wait cursor or the recent-sessions list.
    QCOMPARE(backend.state(), OpcUaConnectionState::Disconnected);
    QVERIFY(!QGuiApplication::overrideCursor());
    coordinator.rebuildRecentSessionsMenu();
    const QList<QAction *> actions = recentSessionsMenu.actions();
    QCOMPARE(actions.size(), 1);
    QVERIFY(!actions.first()->isEnabled());
    // The autosave is not a named session file.
    QVERIFY(window.windowTitle().isEmpty() || !window.windowTitle().contains(
                QStringLiteral("lastsession")));
}

///
/// \brief Nothing is staged when the user opted out of restoring the last session.
///
void TestSessionCoordinatorStartup::autosavedSessionIsSkippedWhenDisabled()
{
    QVERIFY(!writeAutosavedSession().isEmpty());
    AppSettings().setRestoreLastSessionOnStartup(false);

    SessionFakeBackend backend;
    FakeSecretStore secrets;
    ConnectionProfileStore profiles;
    RecentConnectionStore recents;
    ConnectionController controller(&backend, &secrets, &profiles, &recents);
    QWidget window;
    QMenu recentSessionsMenu(&window);

    SessionCoordinatorContext context;
    context.window = &window;
    context.recentSessionsMenu = &recentSessionsMenu;
    context.connectionController = &controller;
    context.backend = &backend;
    SessionCoordinator coordinator(context);

    coordinator.stageLastSession();
    QVERIFY(!coordinator.hasPendingSession());
}

///
/// \brief A named session remains authoritative even when an autosave file also exists.
///
void TestSessionCoordinatorStartup::savedSessionTakesPriorityOverAutosaveAtStartup()
{
    const QString autosave = writeAutosavedSession();
    QVERIFY(!autosave.isEmpty());
    SessionData autosavedData;
    QVERIFY(SessionStore::load(autosave, autosavedData));

    QTemporaryDir sessionsDirectory;
    QVERIFY(sessionsDirectory.isValid());
    const QString savedPath = sessionsDirectory.filePath(QStringLiteral("preferred.ouas"));
    const QString savedEndpoint = QStringLiteral("opc.tcp://saved.invalid:4840");

    SessionData savedData;
    savedData.profile.id = QStringLiteral("saved-profile");
    savedData.profile.name = QStringLiteral("Saved");
    savedData.profile.endpointUrl = savedEndpoint;
    savedData.profile.authentication = ConnectionProfile::Authentication::Anonymous;
    QVERIFY(SessionStore::save(savedPath, savedData));

    {
        WorkspaceHarness harness;
        harness.coordinator->openSessionFromFile(savedPath);
        harness.connectTo(savedEndpoint);
        harness.coordinator->applyPendingSession();

        QCOMPARE(AppSettings().lastSavedSessionPath(), savedPath);
        QVERIFY(!QFileInfo::exists(autosave));
        harness.coordinator->saveAutosavedSession();
        QVERIFY(!QFileInfo::exists(autosave));
    }

    QVERIFY(SessionStore::save(autosave, autosavedData));
    QVERIFY(AppSettings().restoreLastSessionOnStartup());
    QCOMPARE(AppSettings().lastSavedSessionPath(), savedPath);

    WorkspaceHarness restoredHarness;
    restoredHarness.coordinator->stageLastSession();
    restoredHarness.coordinator->connectStagedSession();

    QCOMPARE(restoredHarness.backend.lastDiscoveryUrl, savedEndpoint);
    restoredHarness.connectTo(savedEndpoint);
    restoredHarness.coordinator->applyPendingSession();
    QVERIFY(restoredHarness.window.windowTitle().contains(QStringLiteral("preferred")));
}

///
/// \brief An unavailable named session is forgotten before autosave is staged.
///
void TestSessionCoordinatorStartup::missingSavedSessionFallsBackToAutosave()
{
    QVERIFY(!writeAutosavedSession().isEmpty());
    AppSettings().setLastSavedSessionPath(
        QStringLiteral("/missing/saved-session.ouas"));

    WorkspaceHarness harness;
    harness.coordinator->stageLastSession();
    harness.coordinator->connectStagedSession();

    QCOMPARE(harness.backend.lastDiscoveryUrl,
             QStringLiteral("opc.tcp://autosaved.invalid:4840"));
    QVERIFY(AppSettings().lastSavedSessionPath().isEmpty());
}

///
/// \brief A staged workspace is applied once the same endpoint connects.
///
void TestSessionCoordinatorStartup::autosavedSessionIsAppliedOnMatchingEndpoint()
{
    QVERIFY(!writeAutosavedSession().isEmpty());

    WorkspaceHarness harness;
    harness.coordinator->stageLastSession();
    QVERIFY(harness.coordinator->hasPendingSession());

    harness.connectTo(QStringLiteral("opc.tcp://autosaved.invalid:4840"));
    harness.coordinator->applyPendingSession();

    QVERIFY(!harness.coordinator->hasPendingSession());

    // The saved subscription came back alongside the three built-in ones.
    const QVector<SubscriptionItem> subscriptions = harness.dataView.subscriptions()->subscriptions();
    QCOMPARE(subscriptions.size(), 4);
    QCOMPARE(subscriptions.at(3).name, QStringLiteral("Telemetry"));
}

///
/// \brief Connecting somewhere else neither restores nor discards the staged workspace.
///
void TestSessionCoordinatorStartup::autosavedSessionIsKeptForDifferentEndpoint()
{
    QVERIFY(!writeAutosavedSession().isEmpty());

    WorkspaceHarness harness;
    harness.coordinator->stageLastSession();
    QVERIFY(harness.coordinator->hasPendingSession());

    harness.connectTo(QStringLiteral("opc.tcp://other.invalid:4840"));
    harness.coordinator->applyPendingSession();

    // Still pending, and nothing from the saved workspace leaked into this session.
    QVERIFY(harness.coordinator->hasPendingSession());
    QCOMPARE(harness.dataView.subscriptions()->subscriptions().size(), 3);
}

///
/// \brief Startup reconnects to the endpoint the autosaved workspace belongs to.
///
void TestSessionCoordinatorStartup::stagedSessionReconnectsAtStartup()
{
    QVERIFY(!writeAutosavedSession().isEmpty());

    WorkspaceHarness harness;
    harness.coordinator->stageLastSession();
    QVERIFY(harness.coordinator->hasPendingSession());
    QCOMPARE(harness.backend.state(), OpcUaConnectionState::Disconnected);

    harness.coordinator->connectStagedSession();

    // The anonymous profile starts connecting without asking, so the workspace can come back.
    QCOMPARE(harness.backend.state(), OpcUaConnectionState::Discovering);

    // Once that connection reports success the workspace is applied.
    harness.connectTo(QStringLiteral("opc.tcp://autosaved.invalid:4840"));
    harness.coordinator->applyPendingSession();
    QVERIFY(!harness.coordinator->hasPendingSession());
    QCOMPARE(harness.dataView.subscriptions()->subscriptions().size(), 4);
}

///
/// \brief A session opened explicitly wins over the staged autosave.
///
void TestSessionCoordinatorStartup::commandLineSessionKeepsPriorityOverAutosave()
{
    QVERIFY(!writeAutosavedSession().isEmpty());

    QTemporaryDir sessionsDirectory;
    QVERIFY(sessionsDirectory.isValid());
    const QString path = sessionsDirectory.filePath(QStringLiteral("explicit.ouas"));
    SessionData explicitSession;
    explicitSession.profile.id = QStringLiteral("explicit");
    explicitSession.profile.endpointUrl = QStringLiteral("opc.tcp://explicit.invalid:4840");
    explicitSession.profile.authentication = ConnectionProfile::Authentication::Anonymous;
    QVERIFY(SessionStore::save(path, explicitSession));

    WorkspaceHarness harness;
    harness.coordinator->stageLastSession();
    harness.coordinator->openSessionFromFile(path);

    // Opening the file started its own connection flow...
    QCOMPARE(harness.backend.state(), OpcUaConnectionState::Discovering);

    // ...and replaced the staged workspace, so the autosave must not start a second one.
    harness.backend.setState(OpcUaConnectionState::Disconnected);
    harness.coordinator->connectStagedSession();
    QCOMPARE(harness.backend.state(), OpcUaConnectionState::Disconnected);
}


int main(int argc, char *argv[])
{
    Application app(argc, argv);
    TestSessionCoordinatorStartup test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_sessioncoordinator_startup.moc"
