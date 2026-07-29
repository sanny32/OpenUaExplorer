// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_sessioncoordinator.cpp
/// \brief Tests session restoration cursor handling.
///

#include <QAction>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMenu>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QWidget>

#include "application.h"
#include "appsettings.h"
#include "opcua/connectioncontroller.h"
#include "opcua/connectionprofilestore.h"
#include "opcua/opcuabackend.h"
#include "opcua/recentconnectionstore.h"
#include "session/sessionstore.h"
#include "sessioncoordinator.h"
#include "settingsstore.h"

class SessionFakeBackend : public OpcUaBackend
{
    Q_OBJECT

public:
    using OpcUaBackend::OpcUaBackend;

    bool isAvailable() const override { return true; }
    QStringList availableBackends() const override { return {QStringLiteral("fake")}; }
    OpcUaConnectionState state() const override { return currentState; }
    QString lastError() const override { return {}; }
    void setCertificateTrustDecider(CertificateTrustDecider *) override {}
    void discoverEndpoints(const QString &, const QString &, int) override
    {
        setState(OpcUaConnectionState::Discovering);
    }
    void connectToEndpoint(const ConnectionProfile &, const QString &,
                           const QString &) override {}
    void disconnectFromEndpoint() override {}
    void browse(const QString &) override {}
    void browseReferences(const QString &) override {}
    void readNode(const QString &) override {}
    void readValues(const QStringList &) override {}
    void writeValue(const QString &, const QVariant &, int) override {}

    void setState(OpcUaConnectionState state)
    {
        currentState = state;
        emit stateChanged(state);
    }

    OpcUaConnectionState currentState = OpcUaConnectionState::Disconnected;
};

class TestSessionCoordinator : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void cleanup();
    void openSessionUsesWaitCursorUntilConnectionFails();
    void autosavedSessionIsStagedButNotConnected();
    void autosavedSessionIsSkippedWhenDisabled();
    void disconnectedShutdownRemovesStaleAutosave();

private:
    QString writeAutosavedSession();

    QTemporaryDir _settingsDirectory;
};

void TestSessionCoordinator::initTestCase()
{
    QVERIFY(_settingsDirectory.isValid());
    QCoreApplication::setOrganizationName(QStringLiteral("OpenUaExplorerSessionCoordinatorTests"));
    QCoreApplication::setApplicationName(QStringLiteral("OpenUaExplorerSessionCoordinatorTests"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       _settingsDirectory.path());
    // Keeps the autosave file out of the real application data directory.
    QStandardPaths::setTestModeEnabled(true);
}

void TestSessionCoordinator::cleanupTestCase()
{
    QStandardPaths::setTestModeEnabled(false);
}

void TestSessionCoordinator::cleanup()
{
    while (QGuiApplication::overrideCursor())
        QGuiApplication::restoreOverrideCursor();
    SettingsStore settings;
    settings.clear();
    QFile::remove(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                  + QStringLiteral("/lastsession.ouas"));
}

///
/// \brief Writes a minimal workspace to the autosave path the coordinator reads from.
/// \return The autosave file path.
///
QString TestSessionCoordinator::writeAutosavedSession()
{
    const QString directory =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(directory);
    const QString path = directory + QStringLiteral("/lastsession.ouas");

    SessionData session;
    session.profile.id = QStringLiteral("autosaved-profile");
    session.profile.name = QStringLiteral("Autosaved");
    session.profile.endpointUrl = QStringLiteral("opc.tcp://autosaved.invalid:4840");
    session.profile.authentication = ConnectionProfile::Authentication::Anonymous;
    session.dataAccessNodes.append({QStringLiteral("ns=2;s=Temp"), QStringLiteral("Fast")});
    return SessionStore::save(path, session) ? path : QString();
}

void TestSessionCoordinator::openSessionUsesWaitCursorUntilConnectionFails()
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
    SecretStore secrets;
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
/// \brief The autosaved workspace is staged at startup without driving a connection.
///
void TestSessionCoordinator::autosavedSessionIsStagedButNotConnected()
{
    QVERIFY(!writeAutosavedSession().isEmpty());

    SessionFakeBackend backend;
    SecretStore secrets;
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

    coordinator.stageAutosavedSession();

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
void TestSessionCoordinator::autosavedSessionIsSkippedWhenDisabled()
{
    QVERIFY(!writeAutosavedSession().isEmpty());
    AppSettings().setRestoreLastSessionOnStartup(false);

    SessionFakeBackend backend;
    SecretStore secrets;
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

    coordinator.stageAutosavedSession();
    QVERIFY(!coordinator.hasPendingSession());
}

///
/// \brief Shutting down without a connection clears a stale autosave instead of keeping it.
///
void TestSessionCoordinator::disconnectedShutdownRemovesStaleAutosave()
{
    const QString path = writeAutosavedSession();
    QVERIFY(!path.isEmpty());
    QVERIFY(QFileInfo::exists(path));

    SessionFakeBackend backend;
    SecretStore secrets;
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

    coordinator.saveAutosavedSession();
    QVERIFY(!QFileInfo::exists(path));
}

int main(int argc, char *argv[])
{
    Application app(argc, argv);
    TestSessionCoordinator test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_sessioncoordinator.moc"
