// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_models.cpp
/// \brief Tests the table/tree models used by the OPC UA widgets.
///

#include <QAbstractItemModelTester>
#include <QSignalSpy>
#include <QTest>

#include "testdata.h"
#include "opcua/opcuatypes.h"
#include "widgets/addressspacemodel.h"
#include "widgets/dataaccessmodel.h"
#include "widgets/eventsmodel.h"
#include "widgets/historymodel.h"
#include "widgets/logmodel.h"
#include "widgets/nodeinfomodel.h"
#include "widgets/referencesmodel.h"
#include "widgets/subscriptionsmodel.h"

///
/// \brief Unit tests for the OPC UA UI models.
///
class TestModels : public QObject
{
    Q_OBJECT

private slots:
    // Simple list/table models populated from TestData.
    void simpleModelsAreStructurallyValid();

    // DataAccessModel.
    void dataAccessSetItemsExposesColumns();
    void dataAccessAddOrUpdateInsertsThenUpdates();
    void dataAccessUpdateValuesRefreshesValueColumns();
    void dataAccessRemoveRowsDropsSelected();
    void dataAccessSubscriptionColumnIsEditable();

    // LogModel.
    void logFilterByLevel();
    void logSearchFilterIsCaseInsensitive();
    void logLevelAndSearchCombine();

    // AddressSpaceModel.
    void addressSpaceFetchMoreEmitsBrowseOnce();
    void addressSpaceBrowseFailedAllowsRetry();
    void addressSpaceLeafDoesNotFetch();
    void addressSpaceFindByNodeIdAndDisplayName();
};

///
/// \brief Runs each simple model through QAbstractItemModelTester after loading
///        the shared sample data, catching contract violations in one place.
///
void TestModels::simpleModelsAreStructurallyValid()
{
    EventsModel events;
    new QAbstractItemModelTester(&events, &events);
    events.setItems(TestData::eventItems());
    QCOMPARE(events.rowCount(), TestData::eventItems().size());
    QCOMPARE(events.columnCount(), int(EventsModel::ColCount));
    QCOMPARE(events.data(events.index(0, EventsModel::ColTime)).toString(),
             TestData::eventItems().first().time);
    QCOMPARE(events.headerData(EventsModel::ColMessage, Qt::Horizontal).toString(),
             QStringLiteral("Message"));
    events.clear();
    QCOMPARE(events.rowCount(), 0);

    HistoryModel history;
    new QAbstractItemModelTester(&history, &history);
    history.setItems(TestData::historyItems());
    QCOMPARE(history.rowCount(), TestData::historyItems().size());
    QCOMPARE(history.data(history.index(0, HistoryModel::ColNode)).toString(),
             TestData::historyItems().first().node);

    SubscriptionsModel subscriptions;
    new QAbstractItemModelTester(&subscriptions, &subscriptions);
    subscriptions.setItems(TestData::subscriptionItems());
    QCOMPARE(subscriptions.rowCount(), TestData::subscriptionItems().size());
    QStringList expectedNames;
    for (const SubscriptionItem &item : TestData::subscriptionItems())
        expectedNames.append(item.name);
    QCOMPARE(subscriptions.names(), expectedNames);

    ReferencesModel references;
    new QAbstractItemModelTester(&references, &references);
    references.setItems(TestData::referenceItems());
    QCOMPARE(references.rowCount(), TestData::referenceItems().size());
    QCOMPARE(references.data(references.index(0, ReferencesModel::ColTarget)).toString(),
             TestData::referenceItems().first().target);

    NodeInfoModel nodeInfo;
    new QAbstractItemModelTester(&nodeInfo, &nodeInfo);
    nodeInfo.setItems(TestData::nodeInfoItems());
    QCOMPARE(nodeInfo.rowCount(), TestData::nodeInfoItems().size());
    QCOMPARE(nodeInfo.data(nodeInfo.index(0, NodeInfoModel::ColLabel)).toString(),
             TestData::nodeInfoItems().first().label);
}

///
/// \brief setItems exposes every column with the expected display values.
///
void TestModels::dataAccessSetItemsExposesColumns()
{
    DataAccessModel model;
    new QAbstractItemModelTester(&model, &model);
    const QVector<DataAccessItem> items = TestData::dataAccessItems();
    model.setItems(items);

    QCOMPARE(model.rowCount(), items.size());
    QCOMPARE(model.columnCount(), int(DataAccessModel::ColCount));
    QCOMPARE(model.data(model.index(0, DataAccessModel::ColNumber)).toInt(), 1);
    QCOMPARE(model.data(model.index(0, DataAccessModel::ColNodeId)).toString(),
             items.first().nodeId);
    QCOMPARE(model.data(model.index(0, DataAccessModel::ColValue)).toString(),
             items.first().value);
}

///
/// \brief addOrUpdate inserts a new node first, then updates it in place.
///
void TestModels::dataAccessAddOrUpdateInsertsThenUpdates()
{
    DataAccessModel model;
    new QAbstractItemModelTester(&model, &model);

    OpcUaNodeDetails details;
    details.nodeId = QStringLiteral("ns=2;s=Temp");
    details.displayName = QStringLiteral("Temperature");
    details.value = 23.45;
    details.status = QStringLiteral("Good");

    QSignalSpy insertSpy(&model, &QAbstractItemModel::rowsInserted);
    model.addOrUpdate(details);
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(insertSpy.size(), 1);
    QCOMPARE(model.data(model.index(0, DataAccessModel::ColValue)).toString(),
             QStringLiteral("23.45"));

    details.value = 99.9;
    QSignalSpy changeSpy(&model, &QAbstractItemModel::dataChanged);
    model.addOrUpdate(details);
    QCOMPARE(model.rowCount(), 1); // updated in place, not inserted
    QCOMPARE(changeSpy.size(), 1);
    QCOMPARE(model.data(model.index(0, DataAccessModel::ColValue)).toString(),
             QStringLiteral("99.9"));
}

///
/// \brief updateValues refreshes value/status of the matching node only.
///
void TestModels::dataAccessUpdateValuesRefreshesValueColumns()
{
    DataAccessModel model;
    model.setItems(TestData::dataAccessItems());
    const QString targetNodeId = TestData::dataAccessItems().first().nodeId;

    OpcUaDataValue value;
    value.nodeId = targetNodeId;
    value.value = 42.0;
    value.status = QStringLiteral("Good");
    model.updateValues({value});

    QCOMPARE(model.data(model.index(0, DataAccessModel::ColValue)).toString(),
             QStringLiteral("42"));
    QCOMPARE(model.data(model.index(0, DataAccessModel::ColStatus)).toString(),
             QStringLiteral("Good"));
}

///
/// \brief removeRows drops the selected rows and reports them via nodeIds().
///
void TestModels::dataAccessRemoveRowsDropsSelected()
{
    DataAccessModel model;
    new QAbstractItemModelTester(&model, &model);
    const QVector<DataAccessItem> items = TestData::dataAccessItems();
    model.setItems(items);

    // Remove the last row: the positional "#" column means dropping an earlier
    // row would renumber the survivors without a dataChanged signal, which the
    // model tester rejects. Removing the tail leaves the numbering untouched.
    const int lastRow = items.size() - 1;
    model.removeRows({model.index(lastRow, 0)});

    QCOMPARE(model.rowCount(), items.size() - 1);
    QVERIFY(!model.nodeIds().contains(items.last().nodeId));
}

///
/// \brief Only the subscription column is editable, and setData stores the value.
///
void TestModels::dataAccessSubscriptionColumnIsEditable()
{
    DataAccessModel model;
    model.setItems(TestData::dataAccessItems());

    const QModelIndex nodeIdIndex = model.index(0, DataAccessModel::ColNodeId);
    const QModelIndex subscriptionIndex = model.index(0, DataAccessModel::ColSubscription);
    QVERIFY(!(model.flags(nodeIdIndex) & Qt::ItemIsEditable));
    QVERIFY(model.flags(subscriptionIndex) & Qt::ItemIsEditable);

    QVERIFY(model.setData(subscriptionIndex, QStringLiteral("Fast"), Qt::EditRole));
    QCOMPARE(model.data(subscriptionIndex).toString(), QStringLiteral("Fast"));
    // Editing a non-editable column is rejected.
    QVERIFY(!model.setData(nodeIdIndex, QStringLiteral("x"), Qt::EditRole));
}

///
/// \brief setFilterLevel keeps only matching rows; clearFilterLevel restores all.
///
void TestModels::logFilterByLevel()
{
    LogModel model;
    new QAbstractItemModelTester(&model, &model);
    for (const TestData::LogEntry &entry : TestData::logItems())
        model.addItem({QString(), entry.level, entry.source, entry.message});

    const int total = TestData::logItems().size();
    int errorCount = 0;
    for (const TestData::LogEntry &entry : TestData::logItems())
        if (entry.level == LogItem::Level::Error)
            ++errorCount;

    QCOMPARE(model.rowCount(), total);
    model.setFilterLevel(LogItem::Level::Error);
    QCOMPARE(model.rowCount(), errorCount);
    model.clearFilterLevel();
    QCOMPARE(model.rowCount(), total);
}

///
/// \brief setSearchFilter matches the message substring case-insensitively.
///
void TestModels::logSearchFilterIsCaseInsensitive()
{
    LogModel model;
    model.addItem({QString(), LogItem::Level::Info, "Client", "Connected to server"});
    model.addItem({QString(), LogItem::Level::Info, "Client", "Browse completed"});
    model.addItem({QString(), LogItem::Level::Error, "Client", "Read failed"});

    model.setSearchFilter(QStringLiteral("connect")); // lowercase vs "Connected"
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0, LogModel::ColMessage)).toString(),
             QStringLiteral("Connected to server"));

    model.setSearchFilter(QString());
    QCOMPARE(model.rowCount(), 3);
}

///
/// \brief Level and search filters are combined with logical AND.
///
void TestModels::logLevelAndSearchCombine()
{
    LogModel model;
    model.addItem({QString(), LogItem::Level::Warning, "Client", "timeout on read"});
    model.addItem({QString(), LogItem::Level::Error, "Client", "timeout on write"});
    model.addItem({QString(), LogItem::Level::Error, "Client", "bad node id"});

    model.setFilterLevel(LogItem::Level::Error);
    model.setSearchFilter(QStringLiteral("timeout"));
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0, LogModel::ColMessage)).toString(),
             QStringLiteral("timeout on write"));
}

///
/// \brief Builds a browsable root node for the address space tests.
///
static OpcUaNodeInfo makeRoot()
{
    OpcUaNodeInfo root;
    root.nodeId = QStringLiteral("ns=0;i=84");
    root.displayName = QStringLiteral("Root");
    root.nodeClass = 1;
    root.hasChildren = true;
    return root;
}

///
/// \brief fetchMore requests a browse once; a second call is a no-op.
///
void TestModels::addressSpaceFetchMoreEmitsBrowseOnce()
{
    // No QAbstractItemModelTester here: it eagerly calls fetchMore() while
    // probing the model, which would consume the single browse this test wants
    // to observe.
    AddressSpaceModel model;
    model.setRootNode(makeRoot());

    const QModelIndex rootIndex = model.index(0, 0);
    QVERIFY(model.canFetchMore(rootIndex));

    QSignalSpy browseSpy(&model, &AddressSpaceModel::browseRequested);
    model.fetchMore(rootIndex);
    model.fetchMore(rootIndex); // browse already started
    QCOMPARE(browseSpy.size(), 1);
}

///
/// \brief A failed browse clears the in-flight flag so the node can be retried.
///
void TestModels::addressSpaceBrowseFailedAllowsRetry()
{
    AddressSpaceModel model;
    model.setRootNode(makeRoot());

    const QModelIndex rootIndex = model.index(0, 0);
    model.fetchMore(rootIndex);
    QVERIFY(!model.canFetchMore(rootIndex)); // browse in flight

    model.setBrowseFailed(makeRoot().nodeId);
    QVERIFY(model.canFetchMore(rootIndex)); // retry is allowed again
}

///
/// \brief A node declared without children never offers a fetch.
///
void TestModels::addressSpaceLeafDoesNotFetch()
{
    AddressSpaceModel model;
    OpcUaNodeInfo leaf = makeRoot();
    leaf.hasChildren = false;
    model.setRootNode(leaf);

    const QModelIndex rootIndex = model.index(0, 0);
    QVERIFY(!model.canFetchMore(rootIndex));
    QVERIFY(!model.hasChildren(rootIndex));
}

///
/// \brief findByNodeId and findFirst locate nodes after children are applied.
///
void TestModels::addressSpaceFindByNodeIdAndDisplayName()
{
    AddressSpaceModel model;
    model.setRootNode(makeRoot());
    model.fetchMore(model.index(0, 0));

    OpcUaNodeInfo child;
    child.nodeId = QStringLiteral("ns=0;i=85");
    child.displayName = QStringLiteral("Objects");
    child.nodeClass = 1;
    model.setChildren(makeRoot().nodeId, {child});

    const QModelIndex byId = model.findByNodeId(child.nodeId);
    QVERIFY(byId.isValid());
    QCOMPARE(model.nodeInfo(byId).displayName, child.displayName);

    const QModelIndex byName = model.findFirst(child.displayName);
    QVERIFY(byName.isValid());
    QCOMPARE(byName, byId);

    QVERIFY(!model.findByNodeId(QStringLiteral("ns=0;i=9999")).isValid());
}

QTEST_MAIN(TestModels)

#include "test_models.moc"
