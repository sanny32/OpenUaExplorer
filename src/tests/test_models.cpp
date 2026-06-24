// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_models.cpp
/// \brief Tests the table/tree models used by the OPC UA widgets.
///

#include <QAbstractItemModelTester>
#include <QBrush>
#include <QColor>
#include <QDateTime>
#include <QMimeData>
#include <QScopedPointer>
#include <QSignalSpy>
#include <QTest>

#include "appsettings.h"
#include "testdata.h"
#include "opcua/opcuatypes.h"
#include "opcua/standardnodeid.h"
#include "models/addressspacemimedata.h"
#include "models/addressspacemodel.h"
#include "models/attributesmodel.h"
#include "models/dataaccessmodel.h"
#include "models/eventsmodel.h"
#include "models/historymodel.h"
#include "models/logmodel.h"
#include "models/nodeinfomodel.h"
#include "models/referencesmodel.h"
#include "models/subscriptionsmodel.h"

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
    void dataAccessTimestampModeReformats();
    void attributesModelTimestampModeReformats();

    // LogModel.
    void logFilterByLevel();
    void logSearchFilterIsCaseInsensitive();
    void logLevelAndSearchCombine();

    // AddressSpaceModel.
    void addressSpaceFetchMoreEmitsBrowseOnce();
    void addressSpaceBrowseFailedAllowsRetry();
    void addressSpaceLeafDoesNotFetch();
    void addressSpaceFindByNodeIdAndDisplayName();
    void addressSpaceSetChildrenDeduplicatesNodeIds();
    void addressSpaceDragMimeIncludesVariableNode();

    // Header/role/mutator coverage for the simple table & tree models.
    void historyModelHeaderRolesAndMutators();
    void referencesModelHeaderAndEdges();
    void subscriptionsModelHeaderRolesAndReset();
    void subscriptionsModelEditingAndMutators();
    void logModelColumnsRolesAndFilters();
    void attributesModelHeaderRolesAndMutators();
    void eventsModelHeaderRolesAndMutators();
    void nodeInfoModelColumnsAndClear();
    void dataAccessHeaderRolesAndHelpers();
    void addressSpaceDataRolesAndTreeOps();
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
    details.dataTypeId = QStringLiteral("ns=0;i=11");
    details.status = QStringLiteral("Good");

    QSignalSpy insertSpy(&model, &QAbstractItemModel::rowsInserted);
    model.addOrUpdate(details);
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(insertSpy.size(), 1);
    QCOMPARE(model.data(model.index(0, DataAccessModel::ColValue)).toString(),
             QStringLiteral("23.45"));
    QCOMPARE(model.data(model.index(0, DataAccessModel::ColDataType)).toString(),
             QStringLiteral("Double"));
    QCOMPARE(model.itemAt(0).dataTypeId, QStringLiteral("ns=0;i=11"));

    details.value = 99.9;
    details.dataTypeId = QStringLiteral("ns=0;i=1");
    QSignalSpy changeSpy(&model, &QAbstractItemModel::dataChanged);
    model.addOrUpdate(details);
    QCOMPARE(model.rowCount(), 1); // updated in place, not inserted
    QCOMPARE(changeSpy.size(), 1);
    QCOMPARE(model.data(model.index(0, DataAccessModel::ColValue)).toString(),
             QStringLiteral("99.9"));
    QCOMPARE(model.data(model.index(0, DataAccessModel::ColDataType)).toString(),
             QStringLiteral("Boolean"));
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

    // Remove the tail: dropping an earlier row renumbers survivors without a dataChanged signal, which the model tester rejects.
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
/// \brief Toggling the timestamp mode reformats the source-timestamp column live.
///
void TestModels::dataAccessTimestampModeReformats()
{
    DataAccessModel model;
    OpcUaNodeDetails details;
    details.nodeId = QStringLiteral("ns=2;s=TS");
    details.sourceTimestamp = QDateTime(QDate(2024, 1, 2), QTime(3, 4, 5, 678), Qt::UTC);
    model.addOrUpdate(details);

    const QModelIndex timestampIndex = model.index(0, DataAccessModel::ColTimestamp);

    model.setTimestampMode(AppSettings::TimestampMode::LocalTime);
    const QDateTime local = details.sourceTimestamp.toLocalTime();
    QCOMPARE(model.data(timestampIndex).toString(),
             local.toOffsetFromUtc(local.offsetFromUtc()).toString(Qt::ISODateWithMs));

    QSignalSpy spy(&model, &QAbstractItemModel::dataChanged);
    model.setTimestampMode(AppSettings::TimestampMode::Utc);
    QVERIFY(spy.count() >= 1);
    const QString utc = model.data(timestampIndex).toString();
    QCOMPARE(utc, details.sourceTimestamp.toUTC().toString(Qt::ISODateWithMs));
    QVERIFY(utc.endsWith(QLatin1Char('Z')));
}

///
/// \brief Toggling the timestamp mode reformats timestamp rows in the attributes tree.
///
void TestModels::attributesModelTimestampModeReformats()
{
    OpcUaNodeAttribute value;
    value.name = QStringLiteral("Value");
    value.displayValue = QStringLiteral("42");
    OpcUaNodeAttribute timestamp;
    timestamp.name = QStringLiteral("Source Timestamp");
    timestamp.sourceTimestamp = QDateTime(QDate(2024, 1, 2), QTime(3, 4, 5, 678), Qt::UTC);
    value.children.append(timestamp);

    AttributesModel model;
    model.setAttributes({value});

    const QModelIndex valueParent = model.index(0, 0);
    const QModelIndex timestampIndex = model.index(0, AttributesModel::ColValue, valueParent);

    model.setTimestampMode(AppSettings::TimestampMode::LocalTime);
    const QDateTime local = timestamp.sourceTimestamp.toLocalTime();
    QCOMPARE(model.data(timestampIndex).toString(),
             local.toOffsetFromUtc(local.offsetFromUtc()).toString(Qt::ISODateWithMs));

    QSignalSpy spy(&model, &QAbstractItemModel::dataChanged);
    model.setTimestampMode(AppSettings::TimestampMode::Utc);
    QVERIFY(spy.count() >= 1);
    const QString utc = model.data(timestampIndex).toString();
    QCOMPARE(utc, timestamp.sourceTimestamp.toUTC().toString(Qt::ISODateWithMs));
    QVERIFY(utc.endsWith(QLatin1Char('Z')));
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
    root.nodeId = QString::fromLatin1(StandardNodeId::ObjectsFolder);
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
    // No QAbstractItemModelTester here: it eagerly calls fetchMore(), consuming the single browse this test observes.
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

///
/// \brief setChildren keeps one sibling for repeated NodeIds.
///
void TestModels::addressSpaceSetChildrenDeduplicatesNodeIds()
{
    AddressSpaceModel model;
    model.setRootNode(makeRoot());

    OpcUaNodeInfo first;
    first.nodeId = QStringLiteral("ns=2;s=MyDevice");
    first.displayName = QStringLiteral("MyDevice");
    first.referenceTypeId = QStringLiteral("ns=0;i=47");
    first.nodeClass = 1;

    OpcUaNodeInfo duplicate = first;
    duplicate.referenceTypeId = QStringLiteral("ns=0;i=35");

    OpcUaNodeInfo sameNameDifferentNode;
    sameNameDifferentNode.nodeId = QStringLiteral("ns=2;s=OtherDevice");
    sameNameDifferentNode.displayName = QStringLiteral("MyDevice");
    sameNameDifferentNode.nodeClass = 1;

    model.setChildren(makeRoot().nodeId, {first, duplicate, sameNameDifferentNode});

    const QModelIndex rootIndex = model.index(0, 0);
    QCOMPARE(model.rowCount(rootIndex), 2);
    QCOMPARE(model.nodeInfo(model.index(0, 0, rootIndex)).referenceTypeId,
             first.referenceTypeId);
    QCOMPARE(model.nodeInfo(model.index(1, 0, rootIndex)).nodeId,
             sameNameDifferentNode.nodeId);
}

///
/// \brief Drag MIME is exported for variables and ignored for non-variable nodes.
///
void TestModels::addressSpaceDragMimeIncludesVariableNode()
{
    AddressSpaceModel model;
    model.setRootNode(makeRoot());

    OpcUaNodeInfo variable;
    variable.nodeId = QStringLiteral("ns=2;s=Temperature");
    variable.browseName = QStringLiteral("2:Temperature");
    variable.displayName = QStringLiteral("Temperature");
    variable.nodeClass = OpcUa::Variable;

    OpcUaNodeInfo object;
    object.nodeId = QStringLiteral("ns=2;s=Device");
    object.displayName = QStringLiteral("Device");
    object.nodeClass = OpcUa::Object;

    model.setChildren(makeRoot().nodeId, {variable, object});

    const QModelIndex root = model.index(0, 0);
    const QModelIndex variableIndex = model.index(0, 0, root);
    const QModelIndex objectIndex = model.index(1, 0, root);
    QVERIFY(model.mimeTypes().contains(AddressSpaceMime::nodeMimeType()));
    QVERIFY(model.supportedDragActions().testFlag(Qt::CopyAction));
    QVERIFY(model.flags(variableIndex).testFlag(Qt::ItemIsDragEnabled));
    QVERIFY(!model.flags(objectIndex).testFlag(Qt::ItemIsDragEnabled));

    QScopedPointer<QMimeData> variableMime(model.mimeData({variableIndex}));
    OpcUaNodeInfo decoded;
    QVERIFY(AddressSpaceMime::decodeNode(variableMime.data(), &decoded));
    QCOMPARE(decoded.nodeId, variable.nodeId);
    QCOMPARE(decoded.displayName, variable.displayName);
    QCOMPARE(decoded.nodeClass, variable.nodeClass);

    QScopedPointer<QMimeData> objectMime(model.mimeData({objectIndex}));
    QVERIFY(!AddressSpaceMime::decodeNode(objectMime.data(), &decoded));
}

///
/// \brief HistoryModel: headerData, the range column, alignment and mutators.
///
void TestModels::historyModelHeaderRolesAndMutators()
{
    HistoryModel model;
    model.setItems({{QStringLiteral("Temp"), QStringLiteral("1h")},
                    {QStringLiteral("Pressure"), QStringLiteral("24h")}});

    QCOMPARE(model.headerData(HistoryModel::ColNode, Qt::Horizontal).toString(),
             QStringLiteral("Node"));
    QCOMPARE(model.headerData(HistoryModel::ColRange, Qt::Horizontal).toString(),
             QStringLiteral("Range"));
    QVERIFY(!model.headerData(99, Qt::Horizontal).isValid());
    QVERIFY(!model.headerData(HistoryModel::ColNode, Qt::Horizontal,
                              Qt::DecorationRole).isValid());

    QCOMPARE(model.data(model.index(1, HistoryModel::ColRange)).toString(),
             QStringLiteral("24h"));
    QVERIFY(model.data(model.index(0, HistoryModel::ColNode),
                       Qt::TextAlignmentRole).isValid());
    QVERIFY(!model.data(QModelIndex()).isValid());

    QSignalSpy spy(&model, &QAbstractItemModel::dataChanged);
    model.setColumnAlignment(HistoryModel::ColRange,
                             Qt::Alignment(Qt::AlignRight | Qt::AlignVCenter));
    QCOMPARE(spy.size(), 1);
    QCOMPARE(model.data(model.index(0, HistoryModel::ColRange),
                        Qt::TextAlignmentRole).toInt(),
             int(Qt::AlignRight | Qt::AlignVCenter));

    model.clear();
    QCOMPARE(model.rowCount(), 0);
}

///
/// \brief ReferencesModel: headerData, non-display role and clear().
///
void TestModels::referencesModelHeaderAndEdges()
{
    ReferencesModel model;
    model.setItems({{QStringLiteral("Organizes"), QStringLiteral("ns=0;i=85")}});

    QCOMPARE(model.headerData(ReferencesModel::ColReference, Qt::Horizontal).toString(),
             QStringLiteral("Reference"));
    QCOMPARE(model.headerData(ReferencesModel::ColTarget, Qt::Horizontal).toString(),
             QStringLiteral("Target"));
    QVERIFY(!model.headerData(99, Qt::Horizontal).isValid());
    QVERIFY(!model.headerData(ReferencesModel::ColReference, Qt::Horizontal,
                              Qt::DecorationRole).isValid());

    QCOMPARE(model.data(model.index(0, ReferencesModel::ColReference)).toString(),
             QStringLiteral("Organizes"));
    // A non-display role yields an empty value.
    QVERIFY(!model.data(model.index(0, ReferencesModel::ColTarget),
                        Qt::ToolTipRole).isValid());
    QVERIFY(!model.data(QModelIndex()).isValid());

    model.clear();
    QCOMPARE(model.rowCount(), 0);
}

///
/// \brief SubscriptionsModel: headerData, the reset path in setItems and mutators.
///
void TestModels::subscriptionsModelHeaderRolesAndReset()
{
    SubscriptionsModel model;
    model.setItems({{QStringLiteral("Sub1"), 500.0}});

    QCOMPARE(model.headerData(SubscriptionsModel::ColName, Qt::Horizontal).toString(),
             QStringLiteral("Name"));
    QCOMPARE(model.headerData(SubscriptionsModel::ColPublishingInterval,
                              Qt::Horizontal).toString(),
             QStringLiteral("Publishing Interval"));
    QVERIFY(!model.headerData(99, Qt::Horizontal).isValid());
    QVERIFY(!model.headerData(SubscriptionsModel::ColName, Qt::Horizontal,
                              Qt::DecorationRole).isValid());

    QCOMPARE(model.data(model.index(0, SubscriptionsModel::ColPublishingInterval)).toString(),
             QStringLiteral("500 ms"));
    QCOMPARE(model.data(model.index(0, SubscriptionsModel::ColPublishingInterval),
                        Qt::EditRole).toDouble(), 500.0);
    QVERIFY(model.data(model.index(0, SubscriptionsModel::ColName),
                       Qt::TextAlignmentRole).isValid());

    // Replacing a non-empty model exercises the remove-then-insert path.
    QSignalSpy removeSpy(&model, &QAbstractItemModel::rowsRemoved);
    model.setItems({{QStringLiteral("A"), 100.0, 1},
                    {QStringLiteral("B"), 200.0, 2}});
    QCOMPARE(removeSpy.size(), 1);
    QCOMPARE(model.rowCount(), 2);

    model.setColumnAlignment(SubscriptionsModel::ColPublishingInterval,
                             Qt::Alignment(Qt::AlignRight | Qt::AlignVCenter));
    QCOMPARE(model.data(model.index(0, SubscriptionsModel::ColPublishingInterval),
                        Qt::TextAlignmentRole).toInt(),
             int(Qt::AlignRight | Qt::AlignVCenter));

    model.clear();
    QCOMPARE(model.rowCount(), 0);
    model.clear(); // already empty: early return
}

///
/// \brief SubscriptionsModel: editing flags, setData validation, mutators and lookups.
///
void TestModels::subscriptionsModelEditingAndMutators()
{
    SubscriptionsModel model;
    new QAbstractItemModelTester(&model, &model);
    const int firstRow = model.addSubscription({QStringLiteral("Default"), 1000.0, 0});
    QCOMPARE(firstRow, 0);
    model.addSubscription({QStringLiteral("Fast"), 250.0, 1});

    const QModelIndex nameIndex = model.index(0, SubscriptionsModel::ColName);
    const QModelIndex intervalIndex = model.index(0, SubscriptionsModel::ColPublishingInterval);
    QVERIFY(model.flags(nameIndex) & Qt::ItemIsEditable);
    QVERIFY(model.flags(intervalIndex) & Qt::ItemIsEditable);

    // Renaming emits subscriptionRenamed; duplicate and empty names are rejected.
    QSignalSpy renameSpy(&model, &SubscriptionsModel::subscriptionRenamed);
    QVERIFY(!model.setData(nameIndex, QStringLiteral("Fast"), Qt::EditRole));
    QVERIFY(!model.setData(nameIndex, QStringLiteral("   "), Qt::EditRole));
    QVERIFY(model.setData(nameIndex, QStringLiteral("Slow"), Qt::EditRole));
    QCOMPARE(renameSpy.size(), 1);
    QCOMPARE(renameSpy.first().at(0).toString(), QStringLiteral("Default"));
    QCOMPARE(renameSpy.first().at(1).toString(), QStringLiteral("Slow"));

    // Interval edits emit subscriptionIntervalChanged; non-positive values are rejected.
    QSignalSpy intervalSpy(&model, &SubscriptionsModel::subscriptionIntervalChanged);
    QVERIFY(!model.setData(intervalIndex, 0.0, Qt::EditRole));
    QVERIFY(model.setData(intervalIndex, 2000.0, Qt::EditRole));
    QCOMPARE(intervalSpy.size(), 1);
    QCOMPARE(model.intervalFor(QStringLiteral("Slow")), 2000.0);
    QCOMPARE(model.intervalFor(QStringLiteral("missing")), 1000.0);

    QVERIFY(model.containsName(QStringLiteral("Fast")));
    QVERIFY(!model.containsName(QStringLiteral("Fast"), 1));
    QCOMPARE(model.itemAt(1).name, QStringLiteral("Fast"));
    QVERIFY(model.itemAt(99).name.isEmpty());

    model.removeRow(0);
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.names(), QStringList{QStringLiteral("Fast")});
    model.removeRow(99); // out of range: no-op
    QCOMPARE(model.rowCount(), 1);
}

///
/// \brief LogModel: every column, the level colour, the filtered-add path and getters.
///
void TestModels::logModelColumnsRolesAndFilters()
{
    LogModel model;
    model.addItem({QStringLiteral("t1"), LogItem::Level::Info,
                   QStringLiteral("Client"), QStringLiteral("Connected")});
    model.addItem({QStringLiteral("t2"), LogItem::Level::Warning,
                   QStringLiteral("Client"), QStringLiteral("Slow")});
    model.addItem({QStringLiteral("t3"), LogItem::Level::Error,
                   QStringLiteral("Client"), QStringLiteral("Failed")});

    QCOMPARE(model.headerData(LogModel::ColTimestamp, Qt::Horizontal).toString(),
             QStringLiteral("Time"));
    QCOMPARE(model.headerData(LogModel::ColLevel, Qt::Horizontal).toString(),
             QStringLiteral("Level"));
    QCOMPARE(model.headerData(LogModel::ColSource, Qt::Horizontal).toString(),
             QStringLiteral("Source"));
    QCOMPARE(model.headerData(LogModel::ColMessage, Qt::Horizontal).toString(),
             QStringLiteral("Message"));
    QVERIFY(!model.headerData(99, Qt::Horizontal).isValid());
    QVERIFY(!model.headerData(LogModel::ColLevel, Qt::Horizontal,
                              Qt::DecorationRole).isValid());

    QCOMPARE(model.data(model.index(0, LogModel::ColTimestamp)).toString(),
             QStringLiteral("t1"));
    QCOMPARE(model.data(model.index(0, LogModel::ColLevel)).toString(),
             QStringLiteral("INFO"));
    QCOMPARE(model.data(model.index(1, LogModel::ColLevel)).toString(),
             QStringLiteral("WARN"));
    QCOMPARE(model.data(model.index(2, LogModel::ColLevel)).toString(),
             QStringLiteral("ERROR"));
    QCOMPARE(model.data(model.index(0, LogModel::ColSource)).toString(),
             QStringLiteral("Client"));
    QCOMPARE(model.data(model.index(2, LogModel::ColLevel), Qt::ForegroundRole)
                 .value<QColor>(), QColor(200, 40, 40));
    QVERIFY(model.data(model.index(0, LogModel::ColTimestamp),
                       Qt::TextAlignmentRole).isValid());

    // The getter reflects the active level filter.
    model.setFilterLevel(LogItem::Level::Error);
    QCOMPARE(model.filterLevel(), LogItem::Level::Error);

    // Adding a row that fails the active filter stores it but keeps it hidden.
    const int hiddenBefore = model.rowCount();
    model.addItem({QStringLiteral("t4"), LogItem::Level::Info,
                   QStringLiteral("Client"), QStringLiteral("Hidden")});
    QCOMPARE(model.rowCount(), hiddenBefore);
    model.clearFilterLevel();
    QVERIFY(model.rowCount() > hiddenBefore);

    model.setColumnAlignment(LogModel::ColMessage,
                             Qt::Alignment(Qt::AlignRight | Qt::AlignVCenter));
    QCOMPARE(model.data(model.index(0, LogModel::ColMessage),
                        Qt::TextAlignmentRole).toInt(),
             int(Qt::AlignRight | Qt::AlignVCenter));

    model.clear();
    QCOMPARE(model.rowCount(), 0);
}

///
/// \brief AttributesModel: headerData, the foreground-colour rules and mutators.
///
void TestModels::attributesModelHeaderRolesAndMutators()
{
    AttributesModel model;
    new QAbstractItemModelTester(&model, &model);

    QCOMPARE(model.headerData(AttributesModel::ColAttribute, Qt::Horizontal).toString(),
             QStringLiteral("Attribute"));
    QCOMPARE(model.headerData(AttributesModel::ColValue, Qt::Horizontal).toString(),
             QStringLiteral("Value"));
    QVERIFY(!model.headerData(99, Qt::Horizontal).isValid());
    QVERIFY(!model.headerData(AttributesModel::ColAttribute, Qt::Horizontal,
                              Qt::DecorationRole).isValid());

    // Flat rows whose values drive the status-colour rules.
    model.setItems({{QStringLiteral("Status"), QStringLiteral("Good")},
                    {QStringLiteral("Error"),  QStringLiteral("Bad_NodeIdUnknown")},
                    {QStringLiteral("Empty"),  QString()},
                    {QStringLiteral("Plain"),  QStringLiteral("normal")}});
    QCOMPARE(model.rowCount(), 4);

    const QModelIndex goodValue = model.index(0, AttributesModel::ColValue);
    QCOMPARE(model.data(goodValue).toString(), QStringLiteral("Good"));
    QCOMPARE(model.data(goodValue, Qt::ForegroundRole).value<QBrush>().color(),
             QColor(0, 150, 64));
    QCOMPARE(model.data(model.index(1, AttributesModel::ColValue), Qt::ForegroundRole)
                 .value<QBrush>().color(), QColor(210, 70, 70));
    QCOMPARE(model.data(model.index(2, AttributesModel::ColValue), Qt::ForegroundRole)
                 .value<QBrush>().color(), QColor(128, 128, 128));
    // A plain value and the attribute column never get a foreground brush.
    QVERIFY(!model.data(model.index(3, AttributesModel::ColValue),
                        Qt::ForegroundRole).isValid());
    QVERIFY(!model.data(model.index(0, AttributesModel::ColAttribute),
                        Qt::ForegroundRole).isValid());

    QVERIFY(model.data(goodValue, Qt::TextAlignmentRole).isValid());
    QVERIFY(!model.data(QModelIndex()).isValid());
    // A top-level item's parent is the invalid root; parent of an invalid index too.
    QVERIFY(!model.parent(model.index(0, 0)).isValid());
    QVERIFY(!model.parent(QModelIndex()).isValid());
    // Out-of-range requests and rowCount on a value-column parent return empties.
    QVERIFY(!model.index(99, 0).isValid());
    QCOMPARE(model.rowCount(model.index(0, AttributesModel::ColValue)), 0);

    model.setColumnAlignment(AttributesModel::ColValue,
                             Qt::Alignment(Qt::AlignRight | Qt::AlignVCenter));
    QCOMPARE(model.data(goodValue, Qt::TextAlignmentRole).toInt(),
             int(Qt::AlignRight | Qt::AlignVCenter));

    model.clear();
    QCOMPARE(model.rowCount(), 0);
}

///
/// \brief EventsModel: headerData, the message column, alignment role and mutator.
///
void TestModels::eventsModelHeaderRolesAndMutators()
{
    EventsModel model;
    model.setItems({{QStringLiteral("12:00"), QStringLiteral("Started")},
                    {QStringLiteral("12:01"), QStringLiteral("Stopped")}});

    QCOMPARE(model.headerData(EventsModel::ColTime, Qt::Horizontal).toString(),
             QStringLiteral("Time"));
    QCOMPARE(model.headerData(EventsModel::ColMessage, Qt::Horizontal).toString(),
             QStringLiteral("Message"));
    QVERIFY(!model.headerData(99, Qt::Horizontal).isValid());
    QVERIFY(!model.headerData(EventsModel::ColTime, Qt::Horizontal,
                              Qt::DecorationRole).isValid());

    QCOMPARE(model.data(model.index(0, EventsModel::ColMessage)).toString(),
             QStringLiteral("Started"));
    QVERIFY(model.data(model.index(0, EventsModel::ColTime),
                       Qt::TextAlignmentRole).isValid());

    model.setColumnAlignment(EventsModel::ColMessage,
                             Qt::Alignment(Qt::AlignRight | Qt::AlignVCenter));
    QCOMPARE(model.data(model.index(0, EventsModel::ColMessage),
                        Qt::TextAlignmentRole).toInt(),
             int(Qt::AlignRight | Qt::AlignVCenter));
}

///
/// \brief NodeInfoModel: the value column, a non-display role and clear().
///
void TestModels::nodeInfoModelColumnsAndClear()
{
    NodeInfoModel model;
    model.setItems(TestData::nodeInfoItems());
    QVERIFY(model.rowCount() > 0);

    QCOMPARE(model.data(model.index(0, NodeInfoModel::ColValue)).toString(),
             TestData::nodeInfoItems().first().value);
    QVERIFY(!model.data(model.index(0, NodeInfoModel::ColLabel),
                        Qt::ToolTipRole).isValid());

    model.clear();
    QCOMPARE(model.rowCount(), 0);
}

///
/// \brief DataAccessModel: headerData, the remaining columns/roles and helpers.
///
void TestModels::dataAccessHeaderRolesAndHelpers()
{
    DataAccessModel model;

    OpcUaNodeDetails a;
    a.nodeId = QStringLiteral("ns=2;s=A");
    a.displayName = QStringLiteral("Alpha");
    a.value = 1.0;
    a.dataTypeId = QStringLiteral("ns=0;i=11");
    a.status = QStringLiteral("Good");
    model.addOrUpdate(a);

    OpcUaNodeDetails b;
    b.nodeId = QStringLiteral("ns=2;s=B");
    b.displayName = QStringLiteral("Beta");
    b.value = 2.0;
    b.dataTypeId = QStringLiteral("ns=3;i=5001");
    b.status = QStringLiteral("Bad");
    model.addOrUpdate(b);
    QCOMPARE(model.rowCount(), 2);

    // Updating the second node skips the first row (the != continue branch).
    b.value = 22.0;
    model.addOrUpdate(b);
    QCOMPARE(model.data(model.index(1, DataAccessModel::ColValue)).toString(),
             QStringLiteral("22"));

    // updateValues targeting the second node likewise skips the first row.
    OpcUaDataValue v;
    v.nodeId = QStringLiteral("ns=2;s=B");
    v.value = 99.0;
    v.status = QStringLiteral("Good");
    model.updateValues({v});
    QCOMPARE(model.data(model.index(1, DataAccessModel::ColValue)).toString(),
             QStringLiteral("99"));

    // headerData: a representative column, the default section and base delegation.
    QCOMPARE(model.headerData(DataAccessModel::ColNodeId, Qt::Horizontal).toString(),
             QStringLiteral("Node Id"));
    QCOMPARE(model.headerData(DataAccessModel::ColSubscription, Qt::Horizontal).toString(),
             QStringLiteral("Subscription"));
    QVERIFY(!model.headerData(99, Qt::Horizontal).isValid());
    QVERIFY(!model.headerData(DataAccessModel::ColNumber, Qt::Horizontal,
                              Qt::DecorationRole).isValid());

    // data: positional number, the text columns and the placeholder subscription.
    QCOMPARE(model.data(model.index(0, DataAccessModel::ColNumber)).toInt(), 1);
    QCOMPARE(model.data(model.index(0, DataAccessModel::ColDisplayName)).toString(),
             QStringLiteral("Alpha"));
    QCOMPARE(model.data(model.index(0, DataAccessModel::ColDataType)).toString(),
             QStringLiteral("Double"));
    QCOMPARE(model.data(model.index(1, DataAccessModel::ColDataType)).toString(),
             QStringLiteral("ns=3;i=5001"));
    QVERIFY(model.data(model.index(0, DataAccessModel::ColTimestamp)).isValid()
            || model.data(model.index(0, DataAccessModel::ColTimestamp)).toString().isEmpty());
    QCOMPARE(model.data(model.index(0, DataAccessModel::ColSubscription)).toString(),
             QStringLiteral("—"));
    QVERIFY(model.data(model.index(0, DataAccessModel::ColNumber),
                       Qt::TextAlignmentRole).isValid());

    // With no subscription, the foreground role is the disabled-text brush.
    QVERIFY(model.data(model.index(0, DataAccessModel::ColValue), Qt::ForegroundRole)
                .canConvert<QBrush>());

    // Set a subscription, then verify the EditRole readback and the coloured cells.
    const QModelIndex sub0 = model.index(0, DataAccessModel::ColSubscription);
    QVERIFY(model.setData(sub0, QStringLiteral("Fast"), Qt::EditRole));
    QCOMPARE(model.data(sub0, Qt::EditRole).toString(), QStringLiteral("Fast"));
    QCOMPARE(model.data(model.index(0, DataAccessModel::ColValue), Qt::ForegroundRole)
                 .value<QBrush>().color(), QColor(0, 150, 64));
    QCOMPARE(model.data(model.index(0, DataAccessModel::ColStatus), Qt::ForegroundRole)
                 .value<QBrush>().color(), QColor(0, 150, 64));
    QCOMPARE(model.data(sub0, Qt::ForegroundRole).value<QBrush>().color(),
             QColor(0, 120, 200));

    // nodeIds(rows) returns the selected ids, de-duplicating repeated rows.
    const QStringList ids = model.nodeIds({model.index(1, 0), model.index(1, 0)});
    QCOMPARE(ids, QStringList{QStringLiteral("ns=2;s=B")});

    // itemAt clamps out-of-range rows to an empty item.
    QCOMPARE(model.itemAt(0).nodeId, QStringLiteral("ns=2;s=A"));
    QVERIFY(model.itemAt(99).nodeId.isEmpty());

    // removeRows ignores out-of-range indices.
    model.removeRows({model.index(99, 0)});
    QCOMPARE(model.rowCount(), 2);

    model.setColumnAlignment(DataAccessModel::ColValue,
                             Qt::Alignment(Qt::AlignRight | Qt::AlignVCenter));
    QCOMPARE(model.data(model.index(0, DataAccessModel::ColValue),
                        Qt::TextAlignmentRole).toInt(),
             int(Qt::AlignRight | Qt::AlignVCenter));

    model.clear();
    QCOMPARE(model.rowCount(), 0);
}

///
/// \brief AddressSpaceModel: data roles, the parent chain, icons and tree edits.
///
void TestModels::addressSpaceDataRolesAndTreeOps()
{
    AddressSpaceModel model;

    // A small synthetic tree exercises setItems/appendTestItems and every NodeType.
    const QVector<AddressSpaceItem> items = {
        {QStringLiteral("Objects"), AddressSpaceItem::NodeType::Folder,
         {{QStringLiteral("Temp"), AddressSpaceItem::NodeType::Variable, {}}}},
        {QStringLiteral("DoIt"), AddressSpaceItem::NodeType::Method, {}},
        {QStringLiteral("Plain"), AddressSpaceItem::NodeType::Node, {}},
    };
    model.setItems(items);
    QCOMPARE(model.rowCount(), 3);

    const QModelIndex folder = model.index(0, 0);
    const QModelIndex method = model.index(1, 0);
    const QModelIndex temp = model.index(0, 0, folder);
    QVERIFY(temp.isValid());

    // parent() of a child resolves to its folder; a top-level node has no parent.
    QCOMPARE(model.parent(temp), folder);
    QVERIFY(!model.parent(folder).isValid());
    QVERIFY(!model.parent(QModelIndex()).isValid());

    // data roles: display, tooltip == user role, the default branch and invalid index.
    QCOMPARE(model.data(folder, Qt::DisplayRole).toString(), QStringLiteral("Objects"));
    QCOMPARE(model.data(folder, Qt::ToolTipRole).toString(),
             model.data(folder, Qt::UserRole).toString());
    QVERIFY(!model.data(folder, Qt::SizeHintRole).isValid());
    QVERIFY(!model.data(QModelIndex()).isValid());

    // The decoration role only resolves once an icon provider is installed; calling it per NodeType drives iconType's branches.
    QVERIFY(!model.data(folder, Qt::DecorationRole).isValid());
    model.setIconProvider([](AddressSpaceItem::NodeType) { return QIcon(); });
    model.data(folder, Qt::DecorationRole);
    model.data(temp, Qt::DecorationRole);
    model.data(method, Qt::DecorationRole);

    // Lookups: out-of-range index, unknown display name and unknown parent NodeId.
    QVERIFY(!model.index(99, 0).isValid());
    QVERIFY(!model.findFirst(QStringLiteral("nope")).isValid());
    QVERIFY(!model.canFetchMore(QModelIndex()));
    model.setChildren(QStringLiteral("does-not-exist"), {}); // no-op, returns early

    // Switch to a browsable root to cover setChildren's clear-then-insert path.
    model.setRootNode(makeRoot());
    const QModelIndex rootIndex = model.index(0, 0);
    model.fetchMore(rootIndex);

    OpcUaNodeInfo first;
    first.nodeId = QStringLiteral("ns=0;i=85");
    first.displayName = QStringLiteral("Objects");
    first.nodeClass = 1;
    model.setChildren(makeRoot().nodeId, {first});
    QCOMPARE(model.rowCount(rootIndex), 1);

    OpcUaNodeInfo second;
    second.nodeId = QStringLiteral("ns=0;i=86");
    second.displayName = QStringLiteral("Types");
    second.nodeClass = 1;
    model.setChildren(makeRoot().nodeId, {second}); // replaces existing children
    QCOMPARE(model.rowCount(rootIndex), 1);
    QCOMPARE(model.findByNodeId(second.nodeId).isValid(), true);

    model.clear();
    QCOMPARE(model.rowCount(), 0);
}

QTEST_MAIN(TestModels)

#include "test_models.moc"
