// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_sessioncoordinator_support.h
/// \brief Shared harness for SessionCoordinator tests.
///

#pragma once

#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QLabel>
#include <QMenu>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QWidget>

#include "addressspacemodule.h"
#include "application.h"
#include "appsettings.h"
#include "attributemodule.h"
#include "dataaccesscoordinator.h"
#include "dataaccessmodule.h"
#include "eventsmodule.h"
#include "fakesecretstore.h"
#include "features/featuremanager.h"
#include "features/selectioncontext.h"
#include "servicecontext.h"
#include "widgets/dataaccesswidget.h"
#include "widgets/dataview.h"
#include "widgets/subscriptionswidget.h"
#include "widgets/trendpanelwidget.h"
#include "opcua/connectioncontroller.h"
#include "opcua/connectioncredentialsprovider.h"
#include "opcua/connectionprofilestore.h"
#include "opcua/opcuabackend.h"
#include "opcua/recentconnectionstore.h"
#include "session/sessionstore.h"
#include "sessioncoordinator.h"
#include "settingsstore.h"

namespace {

QStringList capturedSessionLogMessages;
QString dismissedDialogText;

///
/// \brief Closes the next modal dialog and records the message it displayed.
/// \param attemptsLeft Event-loop turns still spent waiting for the dialog to appear.
///
void dismissNextDialog(int attemptsLeft = 100)
{
    QTimer::singleShot(0, qApp, [attemptsLeft]() {
        auto *modal = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!modal) {
            if (attemptsLeft > 0)
                dismissNextDialog(attemptsLeft - 1);
            return;
        }
        const QList<QLabel *> labels = modal->findChildren<QLabel *>();
        QStringList texts;
        for (const QLabel *label : labels) {
            if (!label->text().isEmpty())
                texts.append(label->text());
        }
        dismissedDialogText = texts.join(QLatin1Char('\n'));
        modal->accept();
    });
}

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
public:
    using OpcUaBackend::OpcUaBackend;

    bool isAvailable() const override { return true; }
    QStringList availableBackends() const override { return {QStringLiteral("fake")}; }
    OpcUaConnectionState state() const override { return currentState; }
    QString lastError() const override { return {}; }
    void setCertificateTrustDecider(CertificateTrustDecider *) override {}
    void discoverEndpoints(const QString &url, const QString &, int) override
    {
        lastDiscoveryUrl = url;
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

    void completeDiscovery()
    {
        emit endpointsDiscovered({}, QString());
    }

    OpcUaConnectionState currentState = OpcUaConnectionState::Disconnected;
    QString lastDiscoveryUrl;
};

///
/// \brief Credentials provider double that answers every prompt with a typed password.
///
class SessionCredentialsProvider : public ConnectionCredentialsProvider
{
public:
    ConnectionCredentials requestCredentials(const ConnectionProfile &profile,
                                             const QString &) override
    {
        ++requests;
        ConnectionCredentials credentials;
        credentials.accepted = true;
        credentials.profile = profile;
        credentials.password = QStringLiteral("typed-secret");
        return credentials;
    }

    int requests = 0;
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
    FakeSecretStore secrets;
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