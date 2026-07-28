// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_dataaccesscoordinator.cpp
/// \brief Unit tests for DataAccessCoordinator using a fake OPC UA backend.
///

#include <QAbstractItemModel>
#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QPushButton>
#include <QSettings>
#include <QTableView>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QWidget>

#include "addressspacemodule.h"
#include "appsettings.h"
#include "application.h"
#include "attributemodule.h"
#include "dataaccesscoordinator.h"
#include "dataaccessmodule.h"
#include "eventsmodule.h"
#include "features/selectioncontext.h"
#include "opcua/opcuabackend.h"
#include "models/dataaccessmodel.h"
#include "servicecontext.h"
#include "settingsstore.h"
#include "widgets/dataaccesswidget.h"
#include "widgets/dialogbuttonbox.h"
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
    void browse(const QString &nodeId) override { browsedNodeIds.append(nodeId); }
    void browseReferences(const QString &) override {}
    void readNode(const QString &nodeId) override { readNodeIds.append(nodeId); }
    void readValues(const QStringList &) override {}
    void writeValue(const QString &, const QVariant &, int) override {}
    void subscribe(const QString &nodeId, double) override { subscribedNodeIds.append(nodeId); }
    void unsubscribe(const QString &nodeId) override { unsubscribedNodeIds.append(nodeId); }

    void setState(OpcUaConnectionState state)
    {
        currentState = state;
        emit stateChanged(state);
    }

    OpcUaConnectionState currentState = OpcUaConnectionState::Disconnected;
    QStringList browsedNodeIds;
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
        : context(&backend, nullptr)
    {
        dataAccess.initialize(context);
        events.initialize(context);
        attributes.initialize(context);
        addressSpace.initialize(context);

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
                                                &events, &attributes, &addressSpace,
                                                &selection, &backend, actions, &window);
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
    ServiceContext context;
    DataAccessModule dataAccess;
    EventsModule events;
    AttributeModule attributes;
    AddressSpaceModule addressSpace;
    DataView dataView;
    TrendPanelWidget trendPanel;
    SelectionContext selection;
    DataAccessActions actions;
    DataAccessCoordinator *coordinator = nullptr;
};

namespace {

///
/// \brief NodeId of the folder used by the folder-drop tests.
///
const QString kFolderNodeId = QStringLiteral("ns=2;s=MyDevice");

///
/// \brief Builds a browse result of variables plus one nested folder that must be skipped.
/// \param variableCount Number of variable children.
/// \return Browse result.
///
QVector<OpcUaNodeInfo> makeFolderChildren(int variableCount)
{
    QVector<OpcUaNodeInfo> children;
    for (int i = 0; i < variableCount; ++i) {
        OpcUaNodeInfo variable;
        variable.nodeId = QStringLiteral("ns=2;s=Var%1").arg(i);
        variable.displayName = QStringLiteral("Var%1").arg(i);
        variable.nodeClass = OpcUa::Variable;
        variable.hasChildren = false;
        children.append(variable);
    }
    OpcUaNodeInfo nested;
    nested.nodeId = QStringLiteral("ns=2;s=Nested");
    nested.displayName = QStringLiteral("Nested");
    nested.nodeClass = OpcUa::Object;
    children.append(nested);
    return children;
}

///
/// \brief Returns the model behind the Data Access table.
/// \param harness Coordinator harness to inspect.
/// \return Table model, or nullptr when the view is missing.
///
QAbstractItemModel *dataAccessModel(CoordinatorHarness &harness)
{
    auto *view = harness.dataView.dataAccess()
                     ->findChild<QTableView *>(QStringLiteral("dataView"));
    return view ? view->model() : nullptr;
}

///
/// \brief Drops a folder onto Data Access and delivers its browse result.
/// \param harness Coordinator harness to drive.
/// \param children Browse result to deliver.
///
void dropFolder(CoordinatorHarness &harness, const QVector<OpcUaNodeInfo> &children)
{
    emit harness.dataView.dataAccess()->folderDropRequested(kFolderNodeId);
    emit harness.backend.browseFinished(kFolderNodeId, children, QString());
}

///
/// \brief Answers the next modal dialog, waiting for it to appear.
/// \param answer Standard button to click once the dialog is up.
///
void answerNextDialog(DialogButtonBox::StandardButton answer)
{
    QTimer::singleShot(0, qApp, [answer]() {
        auto *modal = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!modal) {
            answerNextDialog(answer);
            return;
        }
        auto *buttons = modal->findChild<DialogButtonBox *>();
        QVERIFY(buttons);
        QPushButton *button = buttons->button(answer);
        QVERIFY(button);
        QTest::mouseClick(button, Qt::LeftButton);
    });
}

} // namespace

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
    void folderDropBrowsesDroppedFolder();
    void folderDropAddsDirectVariablesWithoutPrompt();
    void folderDropIgnoresBrowseResultsOfOtherNodes();
    void folderDropAsksBeforeAddingManyVariables();
    void folderDropDeclinedLeavesTableEmpty();
    void folderDropCapsVariablesAtHardLimit();
    void folderDropRowSettlesAfterSubscription();
    void folderDropRowSettlesAfterFailedRead();

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
    SettingsStore settings;
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
/// \brief A folder dropped onto Data Access is browsed for its children.
///
void TestDataAccessCoordinator::folderDropBrowsesDroppedFolder()
{
    CoordinatorHarness harness;
    harness.backend.setState(OpcUaConnectionState::Connected);

    emit harness.dataView.dataAccess()->folderDropRequested(kFolderNodeId);

    QCOMPARE(harness.backend.browsedNodeIds, QStringList{kFolderNodeId});
}

///
/// \brief Up to the silent limit the folder's direct variables are added without a prompt.
///
void TestDataAccessCoordinator::folderDropAddsDirectVariablesWithoutPrompt()
{
    CoordinatorHarness harness;
    harness.backend.setState(OpcUaConnectionState::Connected);

    dropFolder(harness, makeFolderChildren(4));

    QAbstractItemModel *model = dataAccessModel(harness);
    QVERIFY(model);
    // The nested folder is skipped and the rows are visible before any read answers.
    QCOMPARE(model->rowCount(), 4);
    QCOMPARE(harness.backend.readNodeIds.size(), 4);
    QCOMPARE(model->data(model->index(0, DataAccessModel::ColDisplayName)).toString(),
             QStringLiteral("Var0"));
    QVERIFY(!(model->flags(model->index(0, DataAccessModel::ColSubscription))
              & Qt::ItemIsEditable));
}

///
/// \brief Browse results for nodes the coordinator did not drop are ignored.
///
void TestDataAccessCoordinator::folderDropIgnoresBrowseResultsOfOtherNodes()
{
    CoordinatorHarness harness;
    harness.backend.setState(OpcUaConnectionState::Connected);

    emit harness.backend.browseFinished(QStringLiteral("ns=2;s=Other"),
                                        makeFolderChildren(3), QString());

    QAbstractItemModel *model = dataAccessModel(harness);
    QVERIFY(model);
    QCOMPARE(model->rowCount(), 0);
    QVERIFY(harness.backend.readNodeIds.isEmpty());
}

///
/// \brief Above the silent limit the user is asked before the variables are added.
///
void TestDataAccessCoordinator::folderDropAsksBeforeAddingManyVariables()
{
    CoordinatorHarness harness;
    harness.backend.setState(OpcUaConnectionState::Connected);

    answerNextDialog(DialogButtonBox::Yes);
    dropFolder(harness, makeFolderChildren(11));

    QAbstractItemModel *model = dataAccessModel(harness);
    QVERIFY(model);
    QCOMPARE(model->rowCount(), 11);
    QCOMPARE(harness.backend.readNodeIds.size(), 11);
}

///
/// \brief Declining the prompt adds nothing at all.
///
void TestDataAccessCoordinator::folderDropDeclinedLeavesTableEmpty()
{
    CoordinatorHarness harness;
    harness.backend.setState(OpcUaConnectionState::Connected);

    answerNextDialog(DialogButtonBox::No);
    dropFolder(harness, makeFolderChildren(11));

    QAbstractItemModel *model = dataAccessModel(harness);
    QVERIFY(model);
    QCOMPARE(model->rowCount(), 0);
    QVERIFY(harness.backend.readNodeIds.isEmpty());
}

///
/// \brief Beyond the hard limit only the first 100 variables are added.
///
void TestDataAccessCoordinator::folderDropCapsVariablesAtHardLimit()
{
    CoordinatorHarness harness;
    harness.backend.setState(OpcUaConnectionState::Connected);

    answerNextDialog(DialogButtonBox::Ok);
    dropFolder(harness, makeFolderChildren(150));

    QAbstractItemModel *model = dataAccessModel(harness);
    QVERIFY(model);
    QCOMPARE(model->rowCount(), 100);
    QCOMPARE(harness.backend.readNodeIds.size(), 100);
    QCOMPARE(model->data(model->index(99, DataAccessModel::ColDisplayName)).toString(),
             QStringLiteral("Var99"));
}

///
/// \brief A dropped row stays inactive until its read and subscription have finished.
///
void TestDataAccessCoordinator::folderDropRowSettlesAfterSubscription()
{
    CoordinatorHarness harness;
    harness.backend.setState(OpcUaConnectionState::Connected);

    dropFolder(harness, makeFolderChildren(1));

    QAbstractItemModel *model = dataAccessModel(harness);
    QVERIFY(model);
    QCOMPARE(model->rowCount(), 1);

    const QString nodeId = QStringLiteral("ns=2;s=Var0");
    OpcUaNodeDetails details;
    details.nodeId = nodeId;
    details.displayName = QStringLiteral("Var0");
    details.nodeClass = OpcUa::Variable;
    details.status = QStringLiteral("Good");
    emit harness.backend.nodeDetailsReady(details, QString());

    // The attribute read alone must not settle the row; the subscription is still open.
    QCOMPARE(harness.backend.subscribedNodeIds, QStringList{nodeId});
    QVERIFY(!(model->flags(model->index(0, DataAccessModel::ColSubscription))
              & Qt::ItemIsEditable));

    emit harness.backend.monitoringFinished(nodeId, true, true, QString());

    QVERIFY(model->flags(model->index(0, DataAccessModel::ColSubscription))
            & Qt::ItemIsEditable);
    QCOMPARE(model->data(model->index(0, DataAccessModel::ColStatus)).toString(),
             QStringLiteral("Good"));
}

///
/// \brief A failed attribute read settles the row and reports the batch failure once.
///
void TestDataAccessCoordinator::folderDropRowSettlesAfterFailedRead()
{
    CoordinatorHarness harness;
    harness.backend.setState(OpcUaConnectionState::Connected);

    dropFolder(harness, makeFolderChildren(1));

    QAbstractItemModel *model = dataAccessModel(harness);
    QVERIFY(model);

    OpcUaNodeDetails failed;
    failed.nodeId = QStringLiteral("ns=2;s=Var0");
    answerNextDialog(DialogButtonBox::Ok);
    emit harness.backend.nodeDetailsReady(failed, QStringLiteral("Node read timed out."));

    QVERIFY(harness.backend.subscribedNodeIds.isEmpty());
    QCOMPARE(model->rowCount(), 1);
    QVERIFY(model->flags(model->index(0, DataAccessModel::ColSubscription))
            & Qt::ItemIsEditable);
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
