// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_sessioncoordinator_autosave.cpp
/// \brief Tests SessionCoordinator autosave lifecycle.
///

#include <QTest>

#include "test_sessioncoordinator_support.h"

///
/// \brief Tests the related behavior in an isolated Qt Test executable.
///
class TestSessionCoordinatorAutosave : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void cleanup();
    void manualDisconnectDiscardsRestorableSession();
    void autosaveIsSkippedWhenRestoreDisabled();
    void failedConnectionDoesNotAutosave();
    void emptyWorkspaceIsAutosavedAfterDisconnect();
    void emptyWorkspaceIsAutosavedWhileConnected();
    void disconnectCapturesWorkspaceBeforeItIsCleared();
    void connectedShutdownWritesAutosave();
    void emptyStartupWorkspaceDoesNotLogAutosave();

private:
    QString writeAutosavedSession();
    QTemporaryDir _settingsDirectory;
};

///
/// \brief Writes a minimal workspace to the autosave path the coordinator reads from.
/// \return The autosave file path.
///
QString TestSessionCoordinatorAutosave::writeAutosavedSession()
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

void TestSessionCoordinatorAutosave::initTestCase()
{
    QVERIFY(_settingsDirectory.isValid());
    QCoreApplication::setOrganizationName(QStringLiteral("TestSessionCoordinatorAutosave"));
    QCoreApplication::setApplicationName(QStringLiteral("TestSessionCoordinatorAutosave"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       _settingsDirectory.path());
    // Keeps the autosave file out of the real application data directory.
    QStandardPaths::setTestModeEnabled(true);
}

void TestSessionCoordinatorAutosave::cleanupTestCase()
{
    QStandardPaths::setTestModeEnabled(false);
}

void TestSessionCoordinatorAutosave::cleanup()
{
    while (QGuiApplication::overrideCursor())
        QGuiApplication::restoreOverrideCursor();
    SettingsStore settings;
    settings.clear();
    QFile::remove(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                  + QStringLiteral("/lastsession.ouas"));
}

///
/// \brief A manual disconnect clears restore state and suppresses autosave until reconnecting.
///
void TestSessionCoordinatorAutosave::manualDisconnectDiscardsRestorableSession()
{
    const QString autosave = writeAutosavedSession();
    QVERIFY(!autosave.isEmpty());

    QTemporaryDir sessionsDirectory;
    QVERIFY(sessionsDirectory.isValid());
    const QString savedPath = sessionsDirectory.filePath(QStringLiteral("saved.ouas"));
    SessionData savedData;
    savedData.profile.endpointUrl = QStringLiteral("opc.tcp://saved.invalid:4840");
    QVERIFY(SessionStore::save(savedPath, savedData));
    AppSettings().setLastSavedSessionPath(savedPath);

    WorkspaceHarness harness;
    harness.connectTo(QStringLiteral("opc.tcp://manual.invalid:4840"));
    harness.coordinator->discardLastSession();
    harness.backend.setState(OpcUaConnectionState::Disconnected);
    harness.coordinator->saveAutosavedSession();

    QVERIFY(!QFileInfo::exists(autosave));
    QVERIFY(QFileInfo::exists(savedPath));
    QVERIFY(AppSettings().lastSavedSessionPath().isEmpty());

    harness.connectTo(QStringLiteral("opc.tcp://next.invalid:4840"));
    harness.coordinator->saveAutosavedSession();
    QVERIFY(QFileInfo::exists(autosave));
}

///
/// \brief A successful connection is not autosaved when startup restoration is disabled.
///
void TestSessionCoordinatorAutosave::autosaveIsSkippedWhenRestoreDisabled()
{
    QVERIFY(!AppSettings().restoreLastSessionOnStartup());
    const QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + QStringLiteral("/lastsession.ouas");

    WorkspaceHarness harness;
    harness.connectTo(QStringLiteral("opc.tcp://connected.invalid:4840"));
    harness.coordinator->saveAutosavedSession();

    QVERIFY(!QFileInfo::exists(path));
}

///
/// \brief A connection attempt that never reached Connected does not create an autosave.
///
void TestSessionCoordinatorAutosave::failedConnectionDoesNotAutosave()
{
    AppSettings().setRestoreLastSessionOnStartup(true);
    const QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + QStringLiteral("/lastsession.ouas");

    WorkspaceHarness harness;
    ConnectionProfile profile;
    profile.id = QStringLiteral("failed-profile");
    profile.endpointUrl = QStringLiteral("opc.tcp://failed.invalid:4840");
    profile.authentication = ConnectionProfile::Authentication::Anonymous;
    harness.controller.connectNewProfile(profile, QString(), QString());
    harness.backend.setState(OpcUaConnectionState::Connecting);
    harness.backend.setState(OpcUaConnectionState::Disconnected);

    harness.coordinator->saveAutosavedSession();

    QVERIFY(!QFileInfo::exists(path));
}

///
/// \brief Disconnecting records the endpoint even when the workspace has no data-access nodes.
///
void TestSessionCoordinatorAutosave::emptyWorkspaceIsAutosavedAfterDisconnect()
{
    AppSettings().setRestoreLastSessionOnStartup(true);
    const QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + QStringLiteral("/lastsession.ouas");
    QVERIFY(!QFileInfo::exists(path));

    WorkspaceHarness harness;
    harness.connectTo(QStringLiteral("opc.tcp://empty.invalid:4840"));
    harness.backend.setState(OpcUaConnectionState::Disconnected);

    harness.coordinator->saveAutosavedSession();

    QVERIFY(QFileInfo::exists(path));
    SessionData reloaded;
    QString error;
    QVERIFY2(SessionStore::load(path, reloaded, &error), qPrintable(error));
    QCOMPARE(reloaded.profile.endpointUrl, QStringLiteral("opc.tcp://empty.invalid:4840"));
    QVERIFY(reloaded.dataAccessNodes.isEmpty());
}

///
/// \brief A connected session is autosaved even when no data-access nodes are present.
///
void TestSessionCoordinatorAutosave::emptyWorkspaceIsAutosavedWhileConnected()
{
    AppSettings().setRestoreLastSessionOnStartup(true);
    const QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + QStringLiteral("/lastsession.ouas");
    QVERIFY(!QFileInfo::exists(path));

    WorkspaceHarness harness;
    harness.connectTo(QStringLiteral("opc.tcp://empty.invalid:4840"));

    harness.coordinator->saveAutosavedSession();

    QVERIFY(QFileInfo::exists(path));
    SessionData reloaded;
    QString error;
    QVERIFY2(SessionStore::load(path, reloaded, &error), qPrintable(error));
    QCOMPARE(reloaded.profile.endpointUrl, QStringLiteral("opc.tcp://empty.invalid:4840"));
    QVERIFY(reloaded.dataAccessNodes.isEmpty());
}

///
/// \brief The workspace is recorded on disconnect, before the data view is cleared.
///
/// This is the flow that matters in practice: disconnect first, quit afterwards.
///
void TestSessionCoordinatorAutosave::disconnectCapturesWorkspaceBeforeItIsCleared()
{
    AppSettings().setRestoreLastSessionOnStartup(true);
    const QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + QStringLiteral("/lastsession.ouas");
    QVERIFY(!QFileInfo::exists(path));

    WorkspaceHarness harness;
    harness.connectTo(QStringLiteral("opc.tcp://autosaved.invalid:4840"));
    harness.addMonitoredNode(QStringLiteral("ns=2;s=Temp"), QStringLiteral("Telemetry"));

    // What MainWindow does on the transition to idle: capture, then clear.
    harness.backend.setState(OpcUaConnectionState::Disconnected);
    harness.coordinator->saveAutosavedSession();
    harness.dataCoordinator->clearRuntimeState();

    QVERIFY(QFileInfo::exists(path));
    SessionData reloaded;
    QVERIFY(SessionStore::load(path, reloaded));
    QCOMPARE(reloaded.dataAccessNodes.size(), 1);
    QCOMPARE(reloaded.dataAccessNodes.first().nodeId, QStringLiteral("ns=2;s=Temp"));
}

///
/// \brief Quitting while connected writes a workspace the next launch can load.
///
void TestSessionCoordinatorAutosave::connectedShutdownWritesAutosave()
{
    AppSettings().setRestoreLastSessionOnStartup(true);
    const QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + QStringLiteral("/lastsession.ouas");
    QVERIFY(!QFileInfo::exists(path));

    WorkspaceHarness harness;
    harness.connectTo(QStringLiteral("opc.tcp://autosaved.invalid:4840"));
    harness.addMonitoredNode(QStringLiteral("ns=2;s=Temp"), QStringLiteral("Telemetry"));

    harness.coordinator->saveAutosavedSession();

    QVERIFY(QFileInfo::exists(path));

    // The file must be loadable, and carry the endpoint plus the monitored workspace.
    SessionData reloaded;
    QString error;
    QVERIFY2(SessionStore::load(path, reloaded, &error), qPrintable(error));
    QCOMPARE(reloaded.profile.endpointUrl, QStringLiteral("opc.tcp://autosaved.invalid:4840"));
    QCOMPARE(reloaded.dataAccessNodes.size(), 1);
    QCOMPARE(reloaded.dataAccessNodes.first().nodeId, QStringLiteral("ns=2;s=Temp"));
    QCOMPARE(reloaded.dataAccessNodes.first().subscriptionName, QStringLiteral("Telemetry"));

    // The autosave is not a named session: it must not touch the title or the recent list.
    QVERIFY(!harness.window.windowTitle().contains(QStringLiteral("lastsession")));
    harness.coordinator->rebuildRecentSessionsMenu();
    QCOMPARE(harness.recentSessionsMenu.actions().size(), 1);
    QVERIFY(!harness.recentSessionsMenu.actions().first()->isEnabled());
}

///
/// \brief An empty startup workspace does not produce an autosave diagnostic.
///
void TestSessionCoordinatorAutosave::emptyStartupWorkspaceDoesNotLogAutosave()
{
    WorkspaceHarness harness;

    capturedSessionLogMessages.clear();
    const QtMessageHandler previousHandler = qInstallMessageHandler(captureSessionLogMessage);
    harness.coordinator->saveAutosavedSession();
    qInstallMessageHandler(previousHandler);

    QVERIFY(capturedSessionLogMessages.isEmpty());
}


int main(int argc, char *argv[])
{
    Application app(argc, argv);
    TestSessionCoordinatorAutosave test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_sessioncoordinator_autosave.moc"
