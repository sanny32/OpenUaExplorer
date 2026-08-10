// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_dataaccesscoordinator_actions.cpp
/// \brief Tests DataAccessCoordinator action and monitoring state.
///

#include <QTest>

#include "test_dataaccesscoordinator_support.h"

///
/// \brief Tests the related behavior in an isolated Qt Test executable.
///
class TestDataAccessCoordinatorActions : public QObject
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
    void offlineKeepsTheCollectedRows();

private:
    QTemporaryDir _settingsDirectory;
};

///
/// \brief Routes QSettings to a throwaway directory so tests never touch real configuration.
///
void TestDataAccessCoordinatorActions::initTestCase()
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
void TestDataAccessCoordinatorActions::cleanup()
{
    SettingsStore settings;
    settings.clear();
}

///
/// \brief Details for a connected writable variable enable the matching actions.
///
void TestDataAccessCoordinatorActions::detailsReadyEnablesSelectionActions()
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
void TestDataAccessCoordinatorActions::clearedSelectionDisablesActions()
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
void TestDataAccessCoordinatorActions::disconnectedClientKeepsMonitoringDisabled()
{
    CoordinatorHarness harness;
    harness.publishWritableVariable(QStringLiteral("ns=2;s=Demo"));

    QVERIFY(!harness.actions.subscribe->isEnabled());
    QVERIFY(!harness.actions.unsubscribe->isEnabled());
}

///
/// \brief Subscribing marks the node pending and disables both monitoring actions.
///
void TestDataAccessCoordinatorActions::subscribeSelectedMarksNodePending()
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
void TestDataAccessCoordinatorActions::monitoringResultTogglesActions()
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
void TestDataAccessCoordinatorActions::clearRuntimeStateResetsMonitoring()
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
/// \brief Going offline keeps the listed rows the user collected, unlike clearing (issue #7).
///
void TestDataAccessCoordinatorActions::offlineKeepsTheCollectedRows()
{
    CoordinatorHarness harness;
    harness.backend.setState(OpcUaConnectionState::Connected);
    const QString nodeId = QStringLiteral("ns=2;s=Demo");
    harness.publishWritableVariable(nodeId);
    harness.coordinator->addSelectedToView();
    QVERIFY(harness.dataView.dataAccess()->hasData());

    harness.backend.setState(OpcUaConnectionState::Disconnected);
    harness.coordinator->setOffline(true);

    QCOMPARE(harness.dataView.dataAccess()->monitoredNodes().size(), 1);
    QVERIFY(!harness.actions.subscribe->isEnabled());
    QVERIFY(!harness.actions.unsubscribe->isEnabled());

    harness.coordinator->clearRuntimeState();
    QVERIFY(!harness.dataView.dataAccess()->hasData());
}


int main(int argc, char *argv[])
{
    Application app(argc, argv);
    TestDataAccessCoordinatorActions test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_dataaccesscoordinator_actions.moc"
