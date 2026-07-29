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

#include "addressspacemodule.h"
#include "application.h"
#include "appsettings.h"
#include "attributemodule.h"
#include "dataaccesscoordinator.h"
#include "dataaccessmodule.h"
#include "eventsmodule.h"
#include "features/featuremanager.h"
#include "features/selectioncontext.h"
#include "servicecontext.h"
#include "widgets/dataaccesswidget.h"
#include "widgets/dataview.h"
#include "widgets/subscriptionswidget.h"
#include "widgets/trendpanelwidget.h"
#include "opcua/connectioncontroller.h"
#include "opcua/connectionprofilestore.h"
#include "opcua/opcuabackend.h"
#include "opcua/recentconnectionstore.h"
#include "session/sessionstore.h"
#include "sessioncoordinator.h"
#include "settingsstore.h"

namespace {

QStringList capturedSessionLogMessages;

///
/// \brief Captures messages emitted through the session logging category.
///
void captureSessionLogMessage(QtMsgType, const QMessageLogContext &context,
                              const QString &message)
{
    if (context.category && qstrcmp(context.category, "ouaexp.Session") == 0)
        capturedSessionLogMessages.append(message);
}

} // namespace

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

    // Accept monitoring so seeding a workspace does not raise a modal error dialog.
    void subscribe(const QString &nodeId, double) override
    {
        emit monitoringFinished(nodeId, true, true, QString());
    }
    void unsubscribe(const QString &nodeId) override
    {
        emit monitoringFinished(nodeId, false, true, QString());
    }

    void setState(OpcUaConnectionState state)
    {
        currentState = state;
        emit stateChanged(state);
    }

    OpcUaConnectionState currentState = OpcUaConnectionState::Disconnected;
};


///
/// \brief Assembles the collaborators SessionCoordinator needs to snapshot a workspace.
///
/// The fake backend never talks to a server, so the workspace is seeded directly through
/// the data-access widget instead of through monitoring round-trips.
///
struct WorkspaceHarness
{
    WorkspaceHarness()
        : controller(&backend, &secrets, &profiles, &recents)
        , serviceContext(&backend, nullptr)
        , recentSessionsMenu(&window)
    {
        dataAccess.initialize(serviceContext);
        events.initialize(serviceContext);
        attributes.initialize(serviceContext);
        addressSpace.initialize(serviceContext);

        for (QAction **action : {&actions.read, &actions.readSelected, &actions.write,
                                 &actions.writeValue, &actions.subscribe, &actions.unsubscribe,
                                 &actions.addToDataAccess, &actions.removeFromDataAccess,
                                 &actions.clearDataAccess, &actions.setSubscriptionNone,
                                 &actions.setSubscriptionDefault, &actions.setSubscriptionFast,
                                 &actions.setSubscriptionCustom, &actions.readDataHistory,
                                 &actions.readEventsHistory}) {
            *action = new QAction(&window);
        }

        dataCoordinator = new DataAccessCoordinator(&dataView, &trendPanel, &dataAccess, &events,
                                                    &attributes, &addressSpace, &selection,
                                                    &backend, actions, &window);

        context.window = &window;
        context.recentSessionsMenu = &recentSessionsMenu;
        context.connectionController = &controller;
        context.dataAccessCoordinator = dataCoordinator;
        context.featureManager = &features;
        context.backend = &backend;
        context.captureNodeMonitors = [] { return QVector<SessionNodeMonitor>(); };
        context.restoreNodeMonitor = [](const SessionNodeMonitor &) {};

        coordinator = new SessionCoordinator(context, &window);
    }

    ///
    /// \brief Marks the harness as connected to the given endpoint.
    ///
    void connectTo(const QString &endpointUrl)
    {
        ConnectionProfile profile;
        profile.id = QStringLiteral("probe");
        profile.name = QStringLiteral("Probe");
        profile.endpointUrl = endpointUrl;
        profile.authentication = ConnectionProfile::Authentication::Anonymous;
        controller.connectNewProfile(profile, QString(), QString());
        backend.setState(OpcUaConnectionState::Connected);
    }

    ///
    /// \brief Puts one monitored node into the data-access workspace.
    ///
    void addMonitoredNode(const QString &nodeId, const QString &subscriptionName)
    {
        SubscriptionItem subscription;
        subscription.name = subscriptionName;
        subscription.publishingInterval = 250.0;
        dataView.subscriptions()->createSubscription(subscriptionName, 250.0);

        OpcUaNodeDetails details;
        details.nodeId = nodeId;
        details.displayName = nodeId;
        details.nodeClass = OpcUa::Variable;
        dataView.dataAccess()->addNodeWithDefaultSubscription(details, subscription);
    }

    QWidget window;
    SessionFakeBackend backend;
    SecretStore secrets;
    ConnectionProfileStore profiles;
    RecentConnectionStore recents;
    ConnectionController controller;
    ServiceContext serviceContext;
    DataAccessModule dataAccess;
    EventsModule events;
    AttributeModule attributes;
    AddressSpaceModule addressSpace;
    DataView dataView;
    TrendPanelWidget trendPanel;
    SelectionContext selection;
    FeatureManager features;
    DataAccessActions actions;
    QMenu recentSessionsMenu;
    SessionCoordinatorContext context;
    DataAccessCoordinator *dataCoordinator = nullptr;
    SessionCoordinator *coordinator = nullptr;
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
    void disconnectedShutdownKeepsAutosave();
    void emptyWorkspaceDropsAutosaveWhileConnected();
    void disconnectCapturesWorkspaceBeforeItIsCleared();
    void connectedShutdownWritesAutosave();
    void emptyStartupWorkspaceDoesNotLogAutosave();
    void autosavedSessionIsAppliedOnMatchingEndpoint();
    void autosavedSessionIsKeptForDifferentEndpoint();
    void stagedSessionReconnectsAtStartup();
    void commandLineSessionKeepsPriorityOverAutosave();

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
/// \brief Shutting down after a disconnect keeps the workspace captured on disconnect.
///
void TestSessionCoordinator::disconnectedShutdownKeepsAutosave()
{
    const QString path = writeAutosavedSession();
    QVERIFY(!path.isEmpty());

    WorkspaceHarness harness;
    // A profile is known, but the workspace has already been cleared by the disconnect.
    harness.connectTo(QStringLiteral("opc.tcp://autosaved.invalid:4840"));
    harness.backend.setState(OpcUaConnectionState::Disconnected);

    harness.coordinator->saveAutosavedSession();

    QVERIFY(QFileInfo::exists(path));
}

///
/// \brief Emptying the workspace on purpose, while connected, drops the autosave.
///
void TestSessionCoordinator::emptyWorkspaceDropsAutosaveWhileConnected()
{
    const QString path = writeAutosavedSession();
    QVERIFY(!path.isEmpty());

    WorkspaceHarness harness;
    harness.connectTo(QStringLiteral("opc.tcp://autosaved.invalid:4840"));

    harness.coordinator->saveAutosavedSession();

    QVERIFY(!QFileInfo::exists(path));
}

///
/// \brief The workspace is recorded on disconnect, before the data view is cleared.
///
/// This is the flow that matters in practice: disconnect first, quit afterwards.
///
void TestSessionCoordinator::disconnectCapturesWorkspaceBeforeItIsCleared()
{
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

    // Quitting afterwards must not throw the capture away.
    harness.coordinator->saveAutosavedSession();
    QVERIFY(QFileInfo::exists(path));
}

///
/// \brief Quitting while connected writes a workspace the next launch can load.
///
void TestSessionCoordinator::connectedShutdownWritesAutosave()
{
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
void TestSessionCoordinator::emptyStartupWorkspaceDoesNotLogAutosave()
{
    WorkspaceHarness harness;

    capturedSessionLogMessages.clear();
    const QtMessageHandler previousHandler = qInstallMessageHandler(captureSessionLogMessage);
    harness.coordinator->saveAutosavedSession();
    qInstallMessageHandler(previousHandler);

    QVERIFY(capturedSessionLogMessages.isEmpty());
}

///
/// \brief A staged workspace is applied once the same endpoint connects.
///
void TestSessionCoordinator::autosavedSessionIsAppliedOnMatchingEndpoint()
{
    QVERIFY(!writeAutosavedSession().isEmpty());

    WorkspaceHarness harness;
    harness.coordinator->stageAutosavedSession();
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
void TestSessionCoordinator::autosavedSessionIsKeptForDifferentEndpoint()
{
    QVERIFY(!writeAutosavedSession().isEmpty());

    WorkspaceHarness harness;
    harness.coordinator->stageAutosavedSession();
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
void TestSessionCoordinator::stagedSessionReconnectsAtStartup()
{
    QVERIFY(!writeAutosavedSession().isEmpty());

    WorkspaceHarness harness;
    harness.coordinator->stageAutosavedSession();
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
void TestSessionCoordinator::commandLineSessionKeepsPriorityOverAutosave()
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
    harness.coordinator->stageAutosavedSession();
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
    TestSessionCoordinator test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_sessioncoordinator.moc"
