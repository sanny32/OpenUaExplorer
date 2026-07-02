// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_dataaccesscoordinator.cpp
/// \brief Unit tests for DataAccessCoordinator using a fake OPC UA backend.
///

#include <QAction>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <QWidget>

#include "appsettings.h"
#include "application.h"
#include "attributemodule.h"
#include "dataaccesscoordinator.h"
#include "dataaccessmodule.h"
#include "eventsmodule.h"
#include "features/selectioncontext.h"
#include "opcua/opcuabackend.h"
#include "opcua/opcuaclientservice.h"
#include "servicecontext.h"
#include "widgets/dataview.h"
#include "widgets/trendpanelwidget.h"

///
/// \brief Backend double that records monitoring calls without emitting results.
///
class CoordinatorFakeBackend : public OpcUaBackend
{
    Q_OBJECT

public:
    using OpcUaBackend::OpcUaBackend;

    bool isAvailable() const override { return true; }
    QStringList availableBackends() const override { return {QStringLiteral("fake")}; }
    OpcUaConnectionState state() const override { return currentState; }
    QString lastError() const override { return {}; }
    void setCertificateTrustDecider(CertificateTrustDecider *) override {}
    void discoverEndpoints(const QString &, const QString &, int) override {}
    void connectToEndpoint(const ConnectionProfile &, const QString &,
                           const QString &) override {}
    void disconnectFromEndpoint() override {}
    void browse(const QString &, int) override {}
    void browseReferences(const QString &, int) override {}
    void readNode(const QString &nodeId, int) override { readNodeIds.append(nodeId); }
    void readValues(const QStringList &, int) override {}
    void writeValue(const QString &, const QVariant &, int, int) override {}
    void subscribe(const QString &nodeId, double) override { subscribedNodeIds.append(nodeId); }
    void unsubscribe(const QString &nodeId) override { unsubscribedNodeIds.append(nodeId); }

    void setState(OpcUaConnectionState state)
    {
        currentState = state;
        emit stateChanged(state);
    }

    OpcUaConnectionState currentState = OpcUaConnectionState::Disconnected;
    QStringList readNodeIds;
    QStringList subscribedNodeIds;
    QStringList unsubscribedNodeIds;
};

///
/// \brief Bundles the coordinator with its widgets, modules, and fake backend.
///
struct CoordinatorHarness
{
    CoordinatorHarness()
        : service(&backend)
        , context(&service, nullptr)
    {
        dataAccess.initialize(context);
        events.initialize(context);
        attributes.initialize(context);

        actions.read = newAction();
        actions.readSelected = newAction();
        actions.write = newAction();
        actions.writeValue = newAction();
        actions.subscribe = newAction();
        actions.unsubscribe = newAction();
        actions.addToDataAccess = newAction();
        actions.removeFromDataAccess = newAction();
        actions.clearDataAccess = newAction();
        actions.setSubscriptionNone = newAction();
        actions.setSubscriptionDefault = newAction();
        actions.setSubscriptionFast = newAction();
        actions.setSubscriptionCustom = newAction();
        actions.readDataHistory = newAction();
        actions.readEventsHistory = newAction();

        coordinator = new DataAccessCoordinator(&dataView, &trendPanel, &dataAccess,
                                                &events, &attributes, &selection,
                                                &service, actions, &window);
    }

    QAction *newAction()
    {
        auto *action = new QAction(&window);
        action->setEnabled(false);
        return action;
    }

    void publishWritableVariable(const QString &nodeId)
    {
        OpcUaNodeInfo node;
        node.nodeId = nodeId;
        node.displayName = QStringLiteral("Node");
        node.nodeClass = OpcUa::Variable;
        selection.selectNode(node);

        OpcUaNodeDetails details;
        details.nodeId = nodeId;
        details.displayName = QStringLiteral("Node");
        details.nodeClass = OpcUa::Variable;
        details.userAccessLevel = OpcUa::CurrentRead | OpcUa::CurrentWrite;
        details.historizing = true;
        selection.setDetails(details, QString());
    }

    QWidget window;
    CoordinatorFakeBackend backend;
    OpcUaClientService service;
    ServiceContext context;
    DataAccessModule dataAccess;
    EventsModule events;
    AttributeModule attributes;
    DataView dataView;
    TrendPanelWidget trendPanel;
    SelectionContext selection;
    DataAccessActions actions;
    DataAccessCoordinator *coordinator = nullptr;
};

///
/// \brief Verifies the coordinator's action enabling, monitoring state, and persistence.
///
class TestDataAccessCoordinator : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void detailsReadyEnablesSelectionActions();
    void clearedSelectionDisablesActions();
    void disconnectedClientKeepsMonitoringDisabled();
    void subscribeSelectedMarksNodePending();
    void monitoringResultTogglesActions();
    void clearRuntimeStateResetsMonitoring();
    void pageStateSurvivesSaveRestoreRoundTrip();

private:
    QTemporaryDir _settingsDirectory;
};

///
/// \brief Routes QSettings to a throwaway directory so tests never touch real configuration.
///
void TestDataAccessCoordinator::initTestCase()
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
void TestDataAccessCoordinator::cleanup()
{
    QSettings settings;
    settings.clear();
}

///
/// \brief Details for a connected writable variable enable the matching actions.
///
void TestDataAccessCoordinator::detailsReadyEnablesSelectionActions()
{
    CoordinatorHarness harness;
    harness.backend.setState(OpcUaConnectionState::Connected);
    harness.publishWritableVariable(QStringLiteral("ns=2;s=Demo"));

    QVERIFY(harness.actions.read->isEnabled());
    QVERIFY(harness.actions.readSelected->isEnabled());
    QVERIFY(harness.actions.write->isEnabled());
    QVERIFY(harness.actions.writeValue->isEnabled());
    QVERIFY(harness.actions.addToDataAccess->isEnabled());
    QVERIFY(harness.actions.subscribe->isEnabled());
    QVERIFY(!harness.actions.unsubscribe->isEnabled());
    QCOMPARE(harness.actions.readDataHistory->isEnabled(), OpcUa::isHistoryReadSupported());
}

///
/// \brief Clearing the selection disables every selected-node action.
///
void TestDataAccessCoordinator::clearedSelectionDisablesActions()
{
    CoordinatorHarness harness;
    harness.backend.setState(OpcUaConnectionState::Connected);
    harness.publishWritableVariable(QStringLiteral("ns=2;s=Demo"));

    harness.selection.clear();

    QVERIFY(!harness.actions.read->isEnabled());
    QVERIFY(!harness.actions.readSelected->isEnabled());
    QVERIFY(!harness.actions.write->isEnabled());
    QVERIFY(!harness.actions.writeValue->isEnabled());
    QVERIFY(!harness.actions.addToDataAccess->isEnabled());
    QVERIFY(!harness.actions.subscribe->isEnabled());
    QVERIFY(!harness.actions.unsubscribe->isEnabled());
    QVERIFY(!harness.actions.readDataHistory->isEnabled());
    QVERIFY(!harness.actions.readEventsHistory->isEnabled());
}

///
/// \brief Without a connection the monitoring actions stay disabled even with details.
///
void TestDataAccessCoordinator::disconnectedClientKeepsMonitoringDisabled()
{
    CoordinatorHarness harness;
    harness.publishWritableVariable(QStringLiteral("ns=2;s=Demo"));

    QVERIFY(!harness.actions.subscribe->isEnabled());
    QVERIFY(!harness.actions.unsubscribe->isEnabled());
}

///
/// \brief Subscribing marks the node pending and disables both monitoring actions.
///
void TestDataAccessCoordinator::subscribeSelectedMarksNodePending()
{
    CoordinatorHarness harness;
    harness.backend.setState(OpcUaConnectionState::Connected);
    const QString nodeId = QStringLiteral("ns=2;s=Demo");
    harness.publishWritableVariable(nodeId);

    harness.coordinator->subscribeSelected();

    QCOMPARE(harness.backend.subscribedNodeIds, QStringList{nodeId});
    QVERIFY(!harness.actions.subscribe->isEnabled());
    QVERIFY(!harness.actions.unsubscribe->isEnabled());
    QVERIFY(harness.actions.clearDataAccess->isEnabled());
}

///
/// \brief A monitoring result enables the matching subscribe/unsubscribe action.
///
void TestDataAccessCoordinator::monitoringResultTogglesActions()
{
    CoordinatorHarness harness;
    harness.backend.setState(OpcUaConnectionState::Connected);
    const QString nodeId = QStringLiteral("ns=2;s=Demo");
    harness.publishWritableVariable(nodeId);
    harness.coordinator->subscribeSelected();

    emit harness.backend.monitoringFinished(nodeId, true, true, QString());

    QVERIFY(!harness.actions.subscribe->isEnabled());
    QVERIFY(harness.actions.unsubscribe->isEnabled());

    harness.coordinator->unsubscribeSelected();
    QCOMPARE(harness.backend.unsubscribedNodeIds, QStringList{nodeId});
    emit harness.backend.monitoringFinished(nodeId, false, true, QString());

    QVERIFY(harness.actions.subscribe->isEnabled());
    QVERIFY(!harness.actions.unsubscribe->isEnabled());
}

///
/// \brief Disconnecting and clearing the runtime state resets the monitoring actions.
///
void TestDataAccessCoordinator::clearRuntimeStateResetsMonitoring()
{
    CoordinatorHarness harness;
    harness.backend.setState(OpcUaConnectionState::Connected);
    const QString nodeId = QStringLiteral("ns=2;s=Demo");
    harness.publishWritableVariable(nodeId);
    harness.coordinator->subscribeSelected();
    emit harness.backend.monitoringFinished(nodeId, true, true, QString());

    harness.backend.setState(OpcUaConnectionState::Disconnected);
    harness.coordinator->clearRuntimeState();

    QVERIFY(!harness.actions.subscribe->isEnabled());
    QVERIFY(!harness.actions.unsubscribe->isEnabled());
    QVERIFY(!harness.actions.clearDataAccess->isEnabled());
}

///
/// \brief The visible page round-trips through saveState/restoreState.
///
void TestDataAccessCoordinator::pageStateSurvivesSaveRestoreRoundTrip()
{
    CoordinatorHarness harness;
    harness.coordinator->showEventsPage();
    QCOMPARE(harness.dataView.currentPage(), static_cast<int>(DataView::EventsPage));

    AppSettings settings;
    harness.coordinator->saveState(settings);

    harness.coordinator->showDataAccessPage();
    QCOMPARE(harness.dataView.currentPage(), static_cast<int>(DataView::DataAccessPage));

    harness.coordinator->restoreState(settings);
    QCOMPARE(harness.dataView.currentPage(), static_cast<int>(DataView::EventsPage));
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
    TestDataAccessCoordinator test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_dataaccesscoordinator.moc"
