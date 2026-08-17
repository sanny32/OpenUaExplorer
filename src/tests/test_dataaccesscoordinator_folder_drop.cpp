// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_dataaccesscoordinator_folder_drop.cpp
/// \brief Tests DataAccessCoordinator folder drops and subscribe requests.
///

#include <QTest>
#include <QGuiApplication>

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
    void folderDropCrawlsDroppedFolder();
    void folderDropBrowsesDroppedFolder();
    void folderDropUsesWaitCursorWhileCounting();
    void folderDropWaitCursorCoversConcurrentCounts();
    void folderDropAddsSubtreeVariables();
    void folderDropAddsDirectVariablesWithoutPrompt();
    void folderDropIgnoresBrowseResultsOfOtherNodes();
    void folderDropAsksBeforeAddingManyVariables();
    void folderDropDeclinedLeavesTableEmpty();
    void folderDropCapsVariablesAtHardLimit();
    void folderDropCapFollowsTheConfiguredLimit();
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
/// \brief With the subtree crawl on, a dropped folder has its whole subtree crawled.
///
void TestDataAccessCoordinatorFolderDrop::folderDropCrawlsDroppedFolder()
{
    CoordinatorHarness harness;
    harness.backend.setState(OpcUaConnectionState::Connected);

    AppSettings().setRecursiveFolderDrop(true);
    emit harness.dataView.dataAccess()->folderDropRequested(kFolderNodeId);

    QCOMPARE(harness.backend.crawledNodeIds, QStringList{kFolderNodeId});
    QVERIFY(harness.backend.browsedNodeIds.isEmpty());
}

///
/// \brief A folder dropped onto Data Access is browsed for its children by default.
///
void TestDataAccessCoordinatorFolderDrop::folderDropBrowsesDroppedFolder()
{
    CoordinatorHarness harness;
    harness.backend.setState(OpcUaConnectionState::Connected);

    emit harness.dataView.dataAccess()->folderDropRequested(kFolderNodeId);

    QCOMPARE(harness.backend.browsedNodeIds, QStringList{kFolderNodeId});
    QVERIFY(harness.backend.crawledNodeIds.isEmpty());
}

///
/// \brief A dropped folder keeps the wait cursor until its subtree count completes.
///
void TestDataAccessCoordinatorFolderDrop::folderDropUsesWaitCursorWhileCounting()
{
    CoordinatorHarness harness;
    harness.backend.setState(OpcUaConnectionState::Connected);
    AppSettings().setRecursiveFolderDrop(true);

    QVERIFY(!QGuiApplication::overrideCursor());
    emit harness.dataView.dataAccess()->folderDropRequested(kFolderNodeId);

    QVERIFY(QGuiApplication::overrideCursor());
    QCOMPARE(QGuiApplication::overrideCursor()->shape(), Qt::WaitCursor);

    emit harness.backend.subtreeVariablesReady(kFolderNodeId, makeSubtreeVariables(1), QString());

    QVERIFY(!QGuiApplication::overrideCursor());
}

///
/// \brief The wait cursor remains until all simultaneous dropped-folder counts complete.
///
void TestDataAccessCoordinatorFolderDrop::folderDropWaitCursorCoversConcurrentCounts()
{
    CoordinatorHarness harness;
    harness.backend.setState(OpcUaConnectionState::Connected);
    AppSettings().setRecursiveFolderDrop(true);
    const QString otherFolderNodeId = QStringLiteral("ns=2;s=OtherDevice");

    emit harness.dataView.dataAccess()->folderDropRequested(kFolderNodeId);
    emit harness.dataView.dataAccess()->folderDropRequested(otherFolderNodeId);
    emit harness.backend.subtreeVariablesReady(kFolderNodeId, makeSubtreeVariables(1), QString());

    QVERIFY(QGuiApplication::overrideCursor());
    QCOMPARE(QGuiApplication::overrideCursor()->shape(), Qt::WaitCursor);

    emit harness.backend.subtreeVariablesReady(otherFolderNodeId, makeSubtreeVariables(1), QString());

    QVERIFY(!QGuiApplication::overrideCursor());
}

///
/// \brief The variables a subtree crawl found are added, however deep they sat.
///
void TestDataAccessCoordinatorFolderDrop::folderDropAddsSubtreeVariables()
{
    CoordinatorHarness harness;
    harness.backend.setState(OpcUaConnectionState::Connected);

    dropFolderSubtree(harness, makeSubtreeVariables(4));

    QAbstractItemModel *model = dataAccessModel(harness);
    QVERIFY(model);
    QCOMPARE(model->rowCount(), 4);
    QCOMPARE(harness.backend.readNodeIds.size(), 4);
    QCOMPARE(model->data(model->index(0, DataAccessModel::ColDisplayName)).toString(),
             QStringLiteral("Var0"));
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
/// \brief Beyond the default cap only the first variables are added.
///
void TestDataAccessCoordinatorFolderDrop::folderDropCapsVariablesAtHardLimit()
{
    CoordinatorHarness harness;
    harness.backend.setState(OpcUaConnectionState::Connected);

    const int cap = AppSettings::defaultFolderDropMaxNodes;
    answerNextDialog(DialogButtonBox::Ok);
    dropFolder(harness, makeFolderChildren(cap + 50));

    QAbstractItemModel *model = dataAccessModel(harness);
    QVERIFY(model);
    QCOMPARE(model->rowCount(), cap);
    QCOMPARE(harness.backend.readNodeIds.size(), cap);
    QCOMPARE(model->data(model->index(cap - 1, DataAccessModel::ColDisplayName)).toString(),
             QStringLiteral("Var%1").arg(cap - 1));
}

///
/// \brief The cap the user configured decides how many variables a drop contributes.
///
void TestDataAccessCoordinatorFolderDrop::folderDropCapFollowsTheConfiguredLimit()
{
    CoordinatorHarness harness;
    harness.backend.setState(OpcUaConnectionState::Connected);
    AppSettings().setFolderDropMaxNodes(5);

    answerNextDialog(DialogButtonBox::Ok);
    dropFolder(harness, makeFolderChildren(8));

    QAbstractItemModel *model = dataAccessModel(harness);
    QVERIFY(model);
    QCOMPARE(model->rowCount(), 5);
    QCOMPARE(harness.backend.readNodeIds.size(), 5);

    // Exactly the raised cap is accepted without a warning and contributes the sixth row.
    AppSettings().setFolderDropMaxNodes(6);
    bool dialogShown = false;
    QTimer::singleShot(0, qApp, [&dialogShown]() {
        auto *modal = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!modal)
            return;
        dialogShown = true;
        modal->reject();
    });
    dropFolder(harness, makeFolderChildren(6));
    QCoreApplication::processEvents();

    QVERIFY(!dialogShown);
    QCOMPARE(model->rowCount(), 6);
    QCOMPARE(harness.backend.readNodeIds.size(), 11);
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
/// \brief Subscribing a folder from the tree adds the same variables a drop would.
///
void TestDataAccessCoordinatorFolderDrop::subscribeRequestOnFolderAddsItsVariables()
{
    CoordinatorHarness harness;
    harness.backend.setState(OpcUaConnectionState::Connected);
    AppSettings().setRecursiveFolderDrop(false);

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
