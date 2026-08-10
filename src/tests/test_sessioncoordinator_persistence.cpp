// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_sessioncoordinator_persistence.cpp
/// \brief Tests SessionCoordinator saving and modified-state tracking.
///

#include <QTest>

#include "test_sessioncoordinator_support.h"

///
/// \brief Tests the related behavior in an isolated Qt Test executable.
///
class TestSessionCoordinatorPersistence : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void cleanup();
    void savingASessionConfirmsTheDestination();
    void reenteredCredentialsMarkTheSessionModified();
    void restoringASessionWithTypedCredentialsMarksItModified();
    void idleBetweenTypingAndConnectingKeepsTheSessionModified();
    void sessionWithoutAProfileIdIsStillMarkedModified();
    void failedSaveReportsTheError();

private:
    static void openConnectedSession(WorkspaceHarness &harness, const QString &path);
    QTemporaryDir _settingsDirectory;
};

///
/// \brief Puts the harness into "session file open and connected" state.
/// \param harness Harness to drive.
/// \param path Session file to create and open.
///
void TestSessionCoordinatorPersistence::openConnectedSession(WorkspaceHarness &harness, const QString &path)
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


void TestSessionCoordinatorPersistence::initTestCase()
{
    QVERIFY(_settingsDirectory.isValid());
    QCoreApplication::setOrganizationName(QStringLiteral("TestSessionCoordinatorPersistence"));
    QCoreApplication::setApplicationName(QStringLiteral("TestSessionCoordinatorPersistence"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       _settingsDirectory.path());
    // Keeps the autosave file out of the real application data directory.
    QStandardPaths::setTestModeEnabled(true);
}

void TestSessionCoordinatorPersistence::cleanupTestCase()
{
    QStandardPaths::setTestModeEnabled(false);
}

void TestSessionCoordinatorPersistence::cleanup()
{
    while (QGuiApplication::overrideCursor())
        QGuiApplication::restoreOverrideCursor();
    SettingsStore settings;
    settings.clear();
    QFile::remove(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                  + QStringLiteral("/lastsession.ouas"));
}

///
/// \brief A successful save tells the user where the session was written.
///
void TestSessionCoordinatorPersistence::savingASessionConfirmsTheDestination()
{
    QTemporaryDir sessionsDirectory;
    QVERIFY(sessionsDirectory.isValid());
    const QString path = sessionsDirectory.filePath(QStringLiteral("confirmed.ouas"));

    WorkspaceHarness harness;
    openConnectedSession(harness, path);

    dismissedDialogText.clear();
    dismissNextDialog();
    QVERIFY(harness.coordinator->saveCurrentSession());
    QVERIFY(dismissedDialogText.contains(QStringLiteral("confirmed.ouas")));
}

///
/// \brief Credentials typed by hand make the open session count as changed until it is saved.
///
void TestSessionCoordinatorPersistence::reenteredCredentialsMarkTheSessionModified()
{
    QTemporaryDir sessionsDirectory;
    QVERIFY(sessionsDirectory.isValid());
    const QString path = sessionsDirectory.filePath(QStringLiteral("credentials.ouas"));

    WorkspaceHarness harness;
    openConnectedSession(harness, path);

    // Saving first takes the workspace as the baseline, so only the credentials differ later.
    dismissNextDialog();
    QVERIFY(harness.coordinator->saveCurrentSession());
    QVERIFY(!harness.window.isWindowModified());

    emit harness.controller.credentialsEntered();

    QVERIFY(harness.window.isWindowModified());

    dismissNextDialog();
    QVERIFY(harness.coordinator->saveCurrentSession());
    QVERIFY(!harness.window.isWindowModified());
}

///
/// \brief Opening a session that asks for its password leaves it marked as changed.
///
/// Walks the whole path the window drives: the file is opened, the credentials are typed in,
/// the connection reaches the endpoint and the workspace is applied.
///
void TestSessionCoordinatorPersistence::restoringASessionWithTypedCredentialsMarksItModified()
{
    QTemporaryDir sessionsDirectory;
    QVERIFY(sessionsDirectory.isValid());
    const QString path = sessionsDirectory.filePath(QStringLiteral("typed.ouas"));
    const QString endpoint = QStringLiteral("opc.tcp://typed.invalid:4840");

    SessionData session;
    session.profile.id = QStringLiteral("typed-profile");
    session.profile.endpointUrl = endpoint;
    session.profile.authentication = ConnectionProfile::Authentication::Username;
    session.profile.username = QStringLiteral("operator");
    QVERIFY(SessionStore::save(path, session));

    WorkspaceHarness harness;
    SessionCredentialsProvider provider;
    harness.controller.setCredentialsProvider(&provider);

    harness.coordinator->openSessionFromFile(path);

    QTRY_COMPARE(provider.requests, 1);
    harness.backend.completeDiscovery();
    harness.backend.setState(OpcUaConnectionState::Connected);
    harness.coordinator->applyPendingSession();

    QCOMPARE(harness.controller.activeProfile().endpointUrl, endpoint);
    QVERIFY(harness.window.isWindowModified());
}

///
/// \brief A connection that drops to idle before it succeeds keeps the credentials mark.
///
/// The window closes the session on every idle transition, and one happens whenever the first
/// attempt after typing the password fails or the client restarts.
///
void TestSessionCoordinatorPersistence::idleBetweenTypingAndConnectingKeepsTheSessionModified()
{
    QTemporaryDir sessionsDirectory;
    QVERIFY(sessionsDirectory.isValid());
    const QString path = sessionsDirectory.filePath(QStringLiteral("retried.ouas"));
    const QString endpoint = QStringLiteral("opc.tcp://retried.invalid:4840");

    SessionData session;
    session.profile.id = QStringLiteral("retried-profile");
    session.profile.endpointUrl = endpoint;
    session.profile.authentication = ConnectionProfile::Authentication::Username;
    QVERIFY(SessionStore::save(path, session));

    WorkspaceHarness harness;
    SessionCredentialsProvider provider;
    harness.controller.setCredentialsProvider(&provider);

    harness.coordinator->openSessionFromFile(path);
    QTRY_COMPARE(provider.requests, 1);

    // What MainWindow does when an attempt ends without a connection.
    harness.backend.setState(OpcUaConnectionState::Disconnected);
    harness.coordinator->closeCurrentSession();

    harness.backend.completeDiscovery();
    harness.backend.setState(OpcUaConnectionState::Connected);
    harness.coordinator->applyPendingSession();

    QVERIFY(harness.window.isWindowModified());
}

///
/// \brief A session file written without a profile identifier is marked like any other.
///
void TestSessionCoordinatorPersistence::sessionWithoutAProfileIdIsStillMarkedModified()
{
    QTemporaryDir sessionsDirectory;
    QVERIFY(sessionsDirectory.isValid());
    const QString path = sessionsDirectory.filePath(QStringLiteral("anonymous-id.ouas"));
    const QString endpoint = QStringLiteral("opc.tcp://no-id.invalid:4840");

    SessionData session;
    session.profile.endpointUrl = endpoint;
    session.profile.authentication = ConnectionProfile::Authentication::Username;
    QVERIFY(SessionStore::save(path, session));
    QVERIFY(session.profile.id.isEmpty());

    WorkspaceHarness harness;
    SessionCredentialsProvider provider;
    harness.controller.setCredentialsProvider(&provider);

    harness.coordinator->openSessionFromFile(path);
    QTRY_COMPARE(provider.requests, 1);
    harness.backend.completeDiscovery();
    harness.backend.setState(OpcUaConnectionState::Connected);
    harness.coordinator->applyPendingSession();

    QVERIFY(harness.window.isWindowModified());
}

///
/// \brief A save that cannot be written reports the failure instead of passing silently.
///
void TestSessionCoordinatorPersistence::failedSaveReportsTheError()
{
    QTemporaryDir sessionsDirectory;
    QVERIFY(sessionsDirectory.isValid());
    const QString path = sessionsDirectory.filePath(QStringLiteral("unwritable.ouas"));

    WorkspaceHarness harness;
    openConnectedSession(harness, path);
    QVERIFY(sessionsDirectory.remove());

    dismissedDialogText.clear();
    dismissNextDialog();
    QVERIFY(!harness.coordinator->saveCurrentSession());
    QVERIFY(!dismissedDialogText.isEmpty());
}


int main(int argc, char *argv[])
{
    Application app(argc, argv);
    TestSessionCoordinatorPersistence test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_sessioncoordinator_persistence.moc"
