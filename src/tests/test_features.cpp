// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_features.cpp
/// \brief Tests the UI feature host, contributed docks, and selection context.
///

#include <QAction>
#include <QDockWidget>
#include <QMenu>
#include <QSettings>
#include <QSignalSpy>
#include <QTableView>
#include <QTemporaryDir>
#include <QTest>

#include "application.h"
#include "features/selectioncontext.h"
#include "mainwindow.h"
#include "opcua/opcuatypes.h"

///
/// \brief Tests feature-hosted UI contributions.
///
class TestFeatures : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void builtinFeaturesContributeExpectedDocks();
    void selectionContextPublishesOnlyCurrentDetails();
    void selectionContextRequestsHistoryOnlyForHistorizingNodes();
    void selectionContextRequestsEventMonitorOnlyForEventNotifiers();
    void selectionContextRequestsEventHistoryOnlyForHistoryReadNotifiers();

private:
    QTemporaryDir _settingsDirectory;
};

///
/// \brief Routes settings to a temporary directory and registers spy metatypes.
///
void TestFeatures::initTestCase()
{
    QVERIFY(_settingsDirectory.isValid());
    QCoreApplication::setOrganizationName(QStringLiteral("OpenUaExplorerFeatureTests"));
    QCoreApplication::setApplicationName(QStringLiteral("OpenUaExplorerFeatureTests"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       _settingsDirectory.path());

    qRegisterMetaType<OpcUaNodeInfo>("OpcUaNodeInfo");
    qRegisterMetaType<OpcUaNodeDetails>("OpcUaNodeDetails");
}

///
/// \brief Clears settings between tests.
///
void TestFeatures::cleanup()
{
    QSettings settings;
    settings.clear();
}

///
/// \brief Built-in features create the migrated docks and expose them through View.
///
void TestFeatures::builtinFeaturesContributeExpectedDocks()
{
    MainWindow window;

    const QStringList dockNames = {
        QStringLiteral("addressSpaceDock"),
        QStringLiteral("nodeDetailsDock"),
        QStringLiteral("attributesDock"),
        QStringLiteral("logDock"),
    };
    QMenu *viewMenu = window.findChild<QMenu *>(QStringLiteral("menuView"));
    QVERIFY(viewMenu);

    for (const QString &dockName : dockNames) {
        QDockWidget *dock = window.findChild<QDockWidget *>(dockName);
        QVERIFY2(dock, qPrintable(dockName));
        QVERIFY(dock->widget());
        QVERIFY(viewMenu->actions().contains(dock->toggleViewAction()));
    }

    QDockWidget *nodeDetailsDock =
        window.findChild<QDockWidget *>(QStringLiteral("nodeDetailsDock"));
    QVERIFY(nodeDetailsDock->widget()->findChild<QTableView *>(QStringLiteral("nodeInfoTable")));
    QVERIFY(nodeDetailsDock->widget()->findChild<QTableView *>(QStringLiteral("referencesTable")));
}

///
/// \brief SelectionContext ignores stale details from previously selected nodes.
///
void TestFeatures::selectionContextPublishesOnlyCurrentDetails()
{
    SelectionContext selection;
    QSignalSpy selectedSpy(&selection, &SelectionContext::nodeSelected);
    QSignalSpy detailsSpy(&selection, &SelectionContext::detailsReady);
    QSignalSpy clearedSpy(&selection, &SelectionContext::cleared);
    QVERIFY(selectedSpy.isValid());
    QVERIFY(detailsSpy.isValid());
    QVERIFY(clearedSpy.isValid());

    OpcUaNodeInfo firstNode;
    firstNode.nodeId = QStringLiteral("ns=2;s=First");
    OpcUaNodeInfo secondNode;
    secondNode.nodeId = QStringLiteral("ns=2;s=Second");
    OpcUaNodeDetails firstDetails;
    firstDetails.nodeId = firstNode.nodeId;
    OpcUaNodeDetails secondDetails;
    secondDetails.nodeId = secondNode.nodeId;

    selection.selectNode(firstNode);
    selection.setDetails(firstDetails, QString());
    QCOMPARE(selectedSpy.count(), 1);
    QCOMPARE(detailsSpy.count(), 1);
    QCOMPARE(selection.currentDetails().nodeId, firstNode.nodeId);

    selection.selectNode(secondNode);
    selection.setDetails(firstDetails, QString());
    QCOMPARE(selectedSpy.count(), 2);
    QCOMPARE(detailsSpy.count(), 1);

    selection.setDetails(secondDetails, QString());
    QCOMPARE(detailsSpy.count(), 2);
    QCOMPARE(selection.currentDetails().nodeId, secondNode.nodeId);

    selection.clear();
    QCOMPARE(clearedSpy.count(), 1);
    QVERIFY(selection.currentNode().nodeId.isEmpty());
    QVERIFY(selection.currentDetails().nodeId.isEmpty());
}

///
/// \brief SelectionContext waits for details and requests history only for historizing variables.
///
void TestFeatures::selectionContextRequestsHistoryOnlyForHistorizingNodes()
{
    SelectionContext selection;
    QSignalSpy selectedSpy(&selection, &SelectionContext::nodeSelected);
    QSignalSpy historySpy(&selection, &SelectionContext::historyReadRequested);
    QVERIFY(selectedSpy.isValid());
    QVERIFY(historySpy.isValid());

    OpcUaNodeInfo node;
    node.nodeId = QStringLiteral("ns=2;s=Trend");
    node.nodeClass = OpcUa::Variable;
    node.displayName = QStringLiteral("Trend");

    selection.requestHistory(node);
    QCOMPARE(selectedSpy.count(), 1);
    QCOMPARE(historySpy.count(), 0);

    OpcUaNodeDetails details;
    details.nodeId = node.nodeId;
    details.nodeClass = OpcUa::Variable;
    selection.setDetails(details, QString());
    QCOMPARE(historySpy.count(), 0);

    selection.requestHistory(node);
    details.historizing = true;
    selection.setDetails(details, QString());
    QCOMPARE(historySpy.count(), 1);
    QCOMPARE(historySpy.first().first().value<OpcUaNodeInfo>().nodeId, node.nodeId);
}

///
/// \brief SelectionContext waits for details and requests events only for event notifiers.
///
void TestFeatures::selectionContextRequestsEventMonitorOnlyForEventNotifiers()
{
    SelectionContext selection;
    QSignalSpy eventSpy(&selection, &SelectionContext::eventMonitorRequested);
    QVERIFY(eventSpy.isValid());

    OpcUaNodeInfo node;
    node.nodeId = QStringLiteral("ns=0;i=2253");
    node.nodeClass = OpcUa::Object;
    node.displayName = QStringLiteral("Server");

    selection.requestEventMonitor(node);
    QCOMPARE(eventSpy.count(), 0);

    OpcUaNodeDetails details;
    details.nodeId = node.nodeId;
    details.nodeClass = OpcUa::Object;
    selection.setDetails(details, QString());
    QCOMPARE(eventSpy.count(), 0);

    selection.requestEventMonitor(node);
    details.eventNotifier = OpcUa::SubscribeToEvents;
    selection.setDetails(details, QString());
    QCOMPARE(eventSpy.count(), 1);
    QCOMPARE(eventSpy.first().first().value<OpcUaNodeInfo>().nodeId, node.nodeId);
}

///
/// \brief SelectionContext waits for details and requests event history only for HistoryRead notifiers.
///
void TestFeatures::selectionContextRequestsEventHistoryOnlyForHistoryReadNotifiers()
{
    SelectionContext selection;
    QSignalSpy eventHistorySpy(&selection, &SelectionContext::eventsHistoryReadRequested);
    QVERIFY(eventHistorySpy.isValid());

    OpcUaNodeInfo node;
    node.nodeId = QStringLiteral("ns=0;i=2253");
    node.nodeClass = OpcUa::Object;
    node.displayName = QStringLiteral("Server");

    selection.requestEventsHistory(node);
    QCOMPARE(eventHistorySpy.count(), 0);

    OpcUaNodeDetails details;
    details.nodeId = node.nodeId;
    details.nodeClass = OpcUa::Object;
    selection.setDetails(details, QString());
    QCOMPARE(eventHistorySpy.count(), 0);

    selection.requestEventsHistory(node);
    details.eventNotifier = OpcUa::HistoryRead;
    selection.setDetails(details, QString());
    QCOMPARE(eventHistorySpy.count(), 1);
    QCOMPARE(eventHistorySpy.first().first().value<OpcUaNodeInfo>().nodeId, node.nodeId);
}

///
/// \brief Runs the suite under the application class used by the main window.
/// \param argc Argument count.
/// \param argv Argument vector.
/// \return Test exit code.
///
int main(int argc, char *argv[])
{
    Application app(argc, argv);
    TestFeatures test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_features.moc"
