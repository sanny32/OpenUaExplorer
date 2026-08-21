// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_dataaccesscoordinator_support.h
/// \brief Shared harness for DataAccessCoordinator tests.
///

#pragma once

#include <QAbstractItemModel>
#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QPushButton>
#include <QSettings>
#include <QTreeView>
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
    void collectSubtreeVariables(const QString &rootNodeId) override
    {
        crawledNodeIds.append(rootNodeId);
    }

    void setState(OpcUaConnectionState state)
    {
        currentState = state;
        emit stateChanged(state);
    }

    OpcUaConnectionState currentState = OpcUaConnectionState::Disconnected;
    QStringList browsedNodeIds;
    QStringList crawledNodeIds;
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
                     ->findChild<QTreeView *>(QStringLiteral("dataView"));
    return view ? view->model() : nullptr;
}

///
/// \brief Builds the variables a subtree crawl reports, already filtered and flattened.
/// \param variableCount Number of variables found anywhere below the dropped node.
/// \return Crawl result.
///
QVector<OpcUaNodeInfo> makeSubtreeVariables(int variableCount)
{
    QVector<OpcUaNodeInfo> variables;
    for (int i = 0; i < variableCount; ++i) {
        OpcUaNodeInfo variable;
        variable.nodeId = QStringLiteral("ns=2;s=Var%1").arg(i);
        variable.displayName = QStringLiteral("Var%1").arg(i);
        variable.nodeClass = OpcUa::Variable;
        variable.hasChildren = false;
        variables.append(variable);
    }
    return variables;
}

///
/// \brief Drops a folder onto Data Access with the subtree crawl off, delivering a browse result.
/// \param harness Coordinator harness to drive.
/// \param children Browse result to deliver.
///
void dropFolder(CoordinatorHarness &harness, const QVector<OpcUaNodeInfo> &children)
{
    AppSettings().setRecursiveFolderDrop(false);
    emit harness.dataView.dataAccess()->folderDropRequested(kFolderNodeId);
    emit harness.backend.browseFinished(kFolderNodeId, children, QString());
}

///
/// \brief Drops a folder onto Data Access and delivers the variables a subtree crawl found.
/// \param harness Coordinator harness to drive.
/// \param variables Crawl result to deliver.
///
void dropFolderSubtree(CoordinatorHarness &harness, const QVector<OpcUaNodeInfo> &variables)
{
    AppSettings().setRecursiveFolderDrop(true);
    emit harness.dataView.dataAccess()->folderDropRequested(kFolderNodeId);
    emit harness.backend.subtreeVariablesReady(kFolderNodeId, variables, QString());
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