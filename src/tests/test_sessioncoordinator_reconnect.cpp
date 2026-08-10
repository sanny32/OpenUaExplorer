// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_sessioncoordinator_reconnect.cpp
/// \brief Tests SessionCoordinator reconnect workspace handling.
///

#include <QTest>

#include "test_sessioncoordinator_support.h"

///
/// \brief Tests the related behavior in an isolated Qt Test executable.
///
class TestSessionCoordinatorReconnect : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void cleanup();
    void lostConnectionHoldsWorkspaceForReconnect();
    void heldWorkspaceIsDroppedForAnotherEndpoint();
    void openedSessionIsNotReplacedByAHeldWorkspace();

private:
    static void openConnectedSession(WorkspaceHarness &harness, const QString &path);
    QTemporaryDir _settingsDirectory;
};

///
/// \brief Puts the harness into "session file open and connected" state.
/// \param harness Harness to drive.
/// \param path Session file to create and open.
///
void TestSessionCoordinatorReconnect::openConnectedSession(WorkspaceHarness &harness, const QString &path)
{
    const QString endpoint = QStringLiteral("opc.tcp://saving.invalid:4840");

    SessionData session;
    session.profile.id = QStringLiteral("saving-profile");
    session.profile.endpointUrl = endpoint;
    session.profile.authentication = ConnectionProfile::Authentication::Anonymous;
    QVERIFY(SessionStore::save(path, session));

    harness.coordinator->openSessionFromFile(path);
    harness.connectTo(endpoint);
    harness.coordinator->applyPendingSession();
}


void TestSessionCoordinatorReconnect::initTestCase()
{
    QVERIFY(_settingsDirectory.isValid());
    QCoreApplication::setOrganizationName(QStringLiteral("TestSessionCoordinatorReconnect"));
    QCoreApplication::setApplicationName(QStringLiteral("TestSessionCoordinatorReconnect"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       _settingsDirectory.path());
    // Keeps the autosave file out of the real application data directory.
    QStandardPaths::setTestModeEnabled(true);
}

void TestSessionCoordinatorReconnect::cleanupTestCase()
{
    QStandardPaths::setTestModeEnabled(false);
}

void TestSessionCoordinatorReconnect::cleanup()
{
    while (QGuiApplication::overrideCursor())
        QGuiApplication::restoreOverrideCursor();
    SettingsStore settings;
    settings.clear();
    QFile::remove(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                  + QStringLiteral("/lastsession.ouas"));
}

///
/// \brief A dropped connection keeps its workspace, and reconnecting makes it live (issue #7).
///
void TestSessionCoordinatorReconnect::lostConnectionHoldsWorkspaceForReconnect()
{
    QTemporaryDir sessionsDirectory;
    QVERIFY(sessionsDirectory.isValid());
    const QString path = sessionsDirectory.filePath(QStringLiteral("held.ouas"));

    WorkspaceHarness harness;
    openConnectedSession(harness, path);
    const QString endpoint = QStringLiteral("opc.tcp://saving.invalid:4840");
    harness.addMonitoredNode(QStringLiteral("ns=2;s=Temp"), QStringLiteral("Telemetry"));

    // What MainWindow does on the transition to idle: hold the workspace, then grey it out.
    harness.backend.setState(OpcUaConnectionState::Disconnected);
    harness.coordinator->holdWorkspaceForReconnect();
    harness.dataCoordinator->setOffline(true);

    QVERIFY(harness.coordinator->hasPendingSession());
    QCOMPARE(harness.dataView.dataAccess()->monitoredNodes().size(), 1);
    QVERIFY(harness.window.windowTitle().contains(QStringLiteral("held")));

    harness.connectTo(endpoint);
    harness.coordinator->dropHeldWorkspaceIfEndpointChanged();
    harness.dataCoordinator->setOffline(false);
    harness.coordinator->applyPendingSession();

    QVERIFY(!harness.coordinator->hasPendingSession());
    QVERIFY(harness.window.windowTitle().contains(QStringLiteral("held")));
    const QVector<SessionNode> nodes = harness.dataView.dataAccess()->monitoredNodes();
    QCOMPARE(nodes.size(), 1);
    QCOMPARE(nodes.first().nodeId, QStringLiteral("ns=2;s=Temp"));
    QCOMPARE(nodes.first().subscriptionName, QStringLiteral("Telemetry"));
}

///
/// \brief Connecting to another server drops the held workspace instead of mixing them.
///
void TestSessionCoordinatorReconnect::heldWorkspaceIsDroppedForAnotherEndpoint()
{
    QTemporaryDir sessionsDirectory;
    QVERIFY(sessionsDirectory.isValid());
    const QString path = sessionsDirectory.filePath(QStringLiteral("dropped.ouas"));

    WorkspaceHarness harness;
    openConnectedSession(harness, path);
    harness.addMonitoredNode(QStringLiteral("ns=2;s=Temp"), QStringLiteral("Telemetry"));
    QVERIFY(harness.window.windowTitle().contains(QStringLiteral("dropped")));

    harness.backend.setState(OpcUaConnectionState::Disconnected);
    harness.coordinator->holdWorkspaceForReconnect();
    harness.dataCoordinator->setOffline(true);
    QVERIFY(harness.coordinator->hasPendingSession());

    // A lost connection keeps the open session on screen, so the window still names it.
    QVERIFY(harness.window.windowTitle().contains(QStringLiteral("dropped")));

    harness.connectTo(QStringLiteral("opc.tcp://other.invalid:4840"));
    harness.coordinator->dropHeldWorkspaceIfEndpointChanged();

    QVERIFY(!harness.coordinator->hasPendingSession());
    QVERIFY(!harness.dataView.dataAccess()->hasData());
    QVERIFY(!harness.window.windowTitle().contains(QStringLiteral("dropped")));
}

///
/// \brief A workspace loaded from a file is not overwritten when a connection drops.
///
void TestSessionCoordinatorReconnect::openedSessionIsNotReplacedByAHeldWorkspace()
{
    QTemporaryDir sessionsDirectory;
    QVERIFY(sessionsDirectory.isValid());
    const QString path = sessionsDirectory.filePath(QStringLiteral("opened.ouas"));

    SessionData session;
    session.profile.id = QStringLiteral("opened-profile");
    session.profile.endpointUrl = QStringLiteral("opc.tcp://opened.invalid:4840");
    session.profile.authentication = ConnectionProfile::Authentication::Anonymous;
    session.dataAccessNodes.append({QStringLiteral("ns=2;s=FromFile"), QString()});
    QVERIFY(SessionStore::save(path, session));

    WorkspaceHarness harness;
    harness.connectTo(QStringLiteral("opc.tcp://held.invalid:4840"));
    harness.coordinator->openSessionFromFile(path);
    QVERIFY(harness.coordinator->hasPendingSession());

    harness.backend.setState(OpcUaConnectionState::Disconnected);
    harness.coordinator->holdWorkspaceForReconnect();

    harness.connectTo(QStringLiteral("opc.tcp://opened.invalid:4840"));
    harness.coordinator->dropHeldWorkspaceIfEndpointChanged();
    harness.coordinator->applyPendingSession();

    const QVector<SessionNode> nodes = harness.dataView.dataAccess()->monitoredNodes();
    QCOMPARE(nodes.size(), 1);
    QCOMPARE(nodes.first().nodeId, QStringLiteral("ns=2;s=FromFile"));
}

int main(int argc, char *argv[])
{
    Application app(argc, argv);
    TestSessionCoordinatorReconnect test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_sessioncoordinator_reconnect.moc"
