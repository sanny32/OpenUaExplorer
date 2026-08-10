// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_dataaccesscoordinator_folder_drop.cpp
/// \brief Tests DataAccessCoordinator folder drops and subscribe requests.
///

#include <QTest>

#include "test_dataaccesscoordinator_support.h"

///
/// \brief Tests the related behavior in an isolated Qt Test executable.
///
class TestDataAccessCoordinatorFolderDrop : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();
    void folderDropBrowsesDroppedFolder();
    void folderDropAddsDirectVariablesWithoutPrompt();
    void folderDropIgnoresBrowseResultsOfOtherNodes();
    void folderDropAsksBeforeAddingManyVariables();
    void folderDropDeclinedLeavesTableEmpty();
    void folderDropCapsVariablesAtHardLimit();
    void folderDropRowSettlesAfterSubscription();
    void folderDropRowSettlesAfterFailedRead();
    void subscribeRequestOnFolderAddsItsVariables();
    void subscribeRequestOnVariableReadsThatNodeOnly();

private:
    QTemporaryDir _settingsDirectory;
};

///
/// \brief Routes QSettings to a throwaway directory so tests never touch real configuration.
///
void TestDataAccessCoordinatorFolderDrop::initTestCase()
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
void TestDataAccessCoordinatorFolderDrop::cleanup()
{
    SettingsStore settings;
    settings.clear();
}

///
/// \brief A folder dropped onto Data Access is browsed for its children.
///
void TestDataAccessCoordinatorFolderDrop::folderDropBrowsesDroppedFolder()
{
    CoordinatorHarness harness;
    harness.backend.setState(OpcUaConnectionState::Connected);

    emit harness.dataView.dataAccess()->folderDropRequested(kFolderNodeId);

    QCOMPARE(harness.backend.browsedNodeIds, QStringList{kFolderNodeId});
}

///
/// \brief Up to the silent limit the folder's direct variables are added without a prompt.
///
void TestDataAccessCoordinatorFolderDrop::folderDropAddsDirectVariablesWithoutPrompt()
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
void TestDataAccessCoordinatorFolderDrop::folderDropIgnoresBrowseResultsOfOtherNodes()
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
void TestDataAccessCoordinatorFolderDrop::folderDropAsksBeforeAddingManyVariables()
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
void TestDataAccessCoordinatorFolderDrop::folderDropDeclinedLeavesTableEmpty()
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
void TestDataAccessCoordinatorFolderDrop::folderDropCapsVariablesAtHardLimit()
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
void TestDataAccessCoordinatorFolderDrop::folderDropRowSettlesAfterSubscription()
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
void TestDataAccessCoordinatorFolderDrop::folderDropRowSettlesAfterFailedRead()
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
/// \brief Subscribing a folder from the tree adds its direct variables, like a drop does.
///
void TestDataAccessCoordinatorFolderDrop::subscribeRequestOnFolderAddsItsVariables()
{
    CoordinatorHarness harness;
    harness.backend.setState(OpcUaConnectionState::Connected);

    OpcUaNodeInfo folder;
    folder.nodeId = kFolderNodeId;
    folder.displayName = QStringLiteral("MyDevice");
    folder.nodeClass = OpcUa::Object;
    harness.selection.requestSubscribe(folder);

    QCOMPARE(harness.backend.browsedNodeIds, QStringList{kFolderNodeId});
    QVERIFY(harness.backend.readNodeIds.isEmpty());

    emit harness.backend.browseFinished(kFolderNodeId, makeFolderChildren(3), QString());

    QAbstractItemModel *model = dataAccessModel(harness);
    QVERIFY(model);
    QCOMPARE(model->rowCount(), 3);
    QCOMPARE(harness.backend.readNodeIds.size(), 3);
}

///
/// \brief Subscribing a variable from the tree still reads just that node.
///
void TestDataAccessCoordinatorFolderDrop::subscribeRequestOnVariableReadsThatNodeOnly()
{
    CoordinatorHarness harness;
    harness.backend.setState(OpcUaConnectionState::Connected);

    OpcUaNodeInfo variable;
    variable.nodeId = QStringLiteral("ns=2;s=Temperature");
    variable.displayName = QStringLiteral("Temperature");
    variable.nodeClass = OpcUa::Variable;
    harness.selection.requestSubscribe(variable);

    QCOMPARE(harness.backend.readNodeIds, QStringList{variable.nodeId});
    QVERIFY(harness.backend.browsedNodeIds.isEmpty());
}


int main(int argc, char *argv[])
{
    Application app(argc, argv);
    TestDataAccessCoordinatorFolderDrop test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_dataaccesscoordinator_folder_drop.moc"
