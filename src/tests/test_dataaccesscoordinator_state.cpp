// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_dataaccesscoordinator_state.cpp
/// \brief Tests DataAccessCoordinator state persistence and restoration.
///

#include <QTest>

#include "test_dataaccesscoordinator_support.h"

///
/// \brief Tests the related behavior in an isolated Qt Test executable.
///
class TestDataAccessCoordinatorState : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();
    void pageStateSurvivesSaveRestoreRoundTrip();
    void closedPagesSurviveSaveRestoreRoundTrip();
    void restoredNodesKeepSavedOrderWhenReadsFinishOutOfOrder();
    void refreshedDetailsCompleteEveryRestoredRowWithoutAddingNodes();
    void failedRestoredNodeRemainsSavedAndSettles();

private:
    QTemporaryDir _settingsDirectory;
};

///
/// \brief Routes QSettings to a throwaway directory so tests never touch real configuration.
///
void TestDataAccessCoordinatorState::initTestCase()
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
void TestDataAccessCoordinatorState::cleanup()
{
    SettingsStore settings;
    settings.clear();
}

///
/// \brief The visible page round-trips through saveState/restoreState.
///
void TestDataAccessCoordinatorState::pageStateSurvivesSaveRestoreRoundTrip()
{
    CoordinatorHarness harness;
    harness.coordinator->setPageVisible(DataView::EventsPage, true);
    QCOMPARE(harness.dataView.currentPage(), static_cast<int>(DataView::EventsPage));

    AppSettings settings;
    harness.coordinator->saveState(settings);

    harness.coordinator->setPageVisible(DataView::DataAccessPage, true);
    QCOMPARE(harness.dataView.currentPage(), static_cast<int>(DataView::DataAccessPage));

    harness.coordinator->restoreState(settings);
    QCOMPARE(harness.dataView.currentPage(), static_cast<int>(DataView::EventsPage));
}

///
/// \brief Closed tabs stay closed after a restart, and showAllPages() brings them back.
///
void TestDataAccessCoordinatorState::closedPagesSurviveSaveRestoreRoundTrip()
{
    CoordinatorHarness harness;
    harness.coordinator->setPageVisible(DataView::EventsPage, false);
    harness.coordinator->setPageVisible(DataView::DataAccessPage, false);

    AppSettings settings;
    harness.coordinator->saveState(settings);

    harness.coordinator->showAllPages();
    QVERIFY(harness.dataView.isPageVisible(DataView::EventsPage));

    harness.coordinator->restoreState(settings);
    QVERIFY(!harness.dataView.isPageVisible(DataView::EventsPage));
    QVERIFY(!harness.dataView.isPageVisible(DataView::DataAccessPage));

    harness.coordinator->showAllPages();
    QVERIFY(harness.dataView.isPageVisible(DataView::EventsPage));
    QVERIFY(harness.dataView.isPageVisible(DataView::DataAccessPage));
}

///
/// \brief Attribute responses update restored rows without changing their saved order.
///
void TestDataAccessCoordinatorState::restoredNodesKeepSavedOrderWhenReadsFinishOutOfOrder()
{
    CoordinatorHarness harness;
    const QVector<SessionNode> savedNodes{
        {QStringLiteral("ns=2;s=Third"), QStringLiteral("Fast"), HighlightMode::FollowDefault},
        {QStringLiteral("ns=2;s=First"), QString(), HighlightMode::Enabled},
        {QStringLiteral("ns=2;s=Second"), QStringLiteral("Default"), HighlightMode::Disabled}
    };

    harness.coordinator->restoreMonitoredNodes(savedNodes);

    QCOMPARE(harness.backend.readNodeIds,
             QStringList({savedNodes.at(0).nodeId, savedNodes.at(1).nodeId,
                          savedNodes.at(2).nodeId}));
    QCOMPARE(harness.coordinator->monitoredNodes(), savedNodes);

    for (int index : {1, 2, 0}) {
        OpcUaNodeDetails details;
        details.nodeId = savedNodes.at(index).nodeId;
        details.displayName = QStringLiteral("Node %1").arg(index);
        details.nodeClass = OpcUa::Variable;
        emit harness.backend.nodeDetailsReady(details, QString());
    }

    QCOMPARE(harness.coordinator->monitoredNodes(), savedNodes);
    QAbstractItemModel *model = dataAccessModel(harness);
    QVERIFY(model);
    for (int row = 0; row < savedNodes.size(); ++row) {
        QCOMPARE(model->data(model->index(row, DataAccessModel::ColNodeId)).toString(),
                 savedNodes.at(row).nodeId);
    }
}

///
/// \brief Later attribute reads complete every restored row without inserting unrelated rows.
///
void TestDataAccessCoordinatorState::refreshedDetailsCompleteEveryRestoredRowWithoutAddingNodes()
{
    CoordinatorHarness harness;
    const QVector<SessionNode> savedNodes{
        {QStringLiteral("ns=2;s=FirstState"), QStringLiteral("Default"),
         HighlightMode::FollowDefault},
        {QStringLiteral("ns=2;s=SecondState"), QStringLiteral("Fast"),
         HighlightMode::Enabled}
    };
    harness.coordinator->restoreMonitoredNodes(savedNodes);

    for (int index = 0; index < savedNodes.size(); ++index) {
        OpcUaNodeDetails details;
        details.nodeId = savedNodes.at(index).nodeId;
        details.displayName = QStringLiteral("State %1").arg(index + 1);
        details.nodeClass = OpcUa::Variable;
        details.value = index;
        details.dataTypeId = QStringLiteral("ns=1;s=SensorState");
        emit harness.backend.nodeDetailsReady(details, QString());
    }

    const OpcUaEnumEntries entries{{0, QStringLiteral("Disabled")},
                                   {1, QStringLiteral("Enabled")}};
    for (int index = 0; index < savedNodes.size(); ++index) {
        OpcUaNodeDetails details;
        details.nodeId = savedNodes.at(index).nodeId;
        details.displayName = QStringLiteral("Named state %1").arg(index + 1);
        details.nodeClass = OpcUa::Variable;
        details.value = index;
        details.dataTypeId = QStringLiteral("ns=1;s=SensorState");
        details.enumEntries = entries;
        emit harness.backend.nodeDetailsReady(details, QString());
    }

    OpcUaNodeDetails unlisted;
    unlisted.nodeId = QStringLiteral("ns=2;s=Unlisted");
    unlisted.nodeClass = OpcUa::Variable;
    unlisted.value = 1;
    unlisted.enumEntries = entries;
    emit harness.backend.nodeDetailsReady(unlisted, QString());

    QAbstractItemModel *model = dataAccessModel(harness);
    QVERIFY(model);
    QCOMPARE(model->rowCount(), savedNodes.size());
    QCOMPARE(model->data(model->index(0, DataAccessModel::ColDisplayName)).toString(),
             QStringLiteral("Named state 1"));
    QCOMPARE(model->data(model->index(0, DataAccessModel::ColValue)).toString(),
             QStringLiteral("0 (Disabled)"));
    QCOMPARE(model->data(model->index(1, DataAccessModel::ColDisplayName)).toString(),
             QStringLiteral("Named state 2"));
    QCOMPARE(model->data(model->index(1, DataAccessModel::ColValue)).toString(),
             QStringLiteral("1 (Enabled)"));
}

///
/// \brief A failed restore read leaves the saved row reusable and no longer pending.
///
void TestDataAccessCoordinatorState::failedRestoredNodeRemainsSavedAndSettles()
{
    CoordinatorHarness harness;
    const QVector<SessionNode> savedNodes{
        {QStringLiteral("ns=2;s=Unavailable"), QStringLiteral("Default"),
         HighlightMode::FollowDefault}
    };

    harness.coordinator->restoreMonitoredNodes(savedNodes);

    OpcUaNodeDetails failed;
    failed.nodeId = savedNodes.first().nodeId;
    emit harness.backend.nodeDetailsReady(failed, QStringLiteral("Node read timed out."));

    QCOMPARE(harness.coordinator->monitoredNodes(), savedNodes);
    QAbstractItemModel *model = dataAccessModel(harness);
    QVERIFY(model);
    QVERIFY(model->flags(model->index(0, DataAccessModel::ColSubscription))
            & Qt::ItemIsEditable);
}


int main(int argc, char *argv[])
{
    Application app(argc, argv);
    TestDataAccessCoordinatorState test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_dataaccesscoordinator_state.moc"
