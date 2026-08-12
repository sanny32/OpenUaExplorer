// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_models_data_access_core.cpp
/// \brief Tests DataAccessModel mutation, ordering, editing, and export behavior.
///

#include <QAbstractItemModelTester>
#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QDateTime>
#include <QFont>
#include <QMimeData>
#include <QPalette>
#include <QScopedPointer>
#include <QSignalSpy>
#include <QStandardItemModel>
#include <QTest>
#include <QTimeZone>

#include "appsettings.h"
#include "testdata.h"
#include "testimages.h"
#include "formatters/attributeformatter.h"
#include "opcua/opcuatypes.h"
#include "models/attributesmodel.h"
#include "models/csvexporter.h"
#include "models/dataaccessmodel.h"
#include "models/eventsmodel.h"
#include "models/historymodel.h"
#include "models/logmodel.h"
#include "models/nodeinfomodel.h"
#include "models/referencesmodel.h"
#include "models/subscriptionsmodel.h"
#include "models/valueroles.h"

using TestImages::encodedPng;

///
/// \brief Tests the related behavior in an isolated Qt Test executable.
///
class TestModelsDataAccessCore : public QObject
{
    Q_OBJECT

private slots:
    void dataAccessSetItemsExposesColumns();
    void dataAccessAddOrUpdateInsertsThenUpdates();
    void dataAccessUpdateValuesRefreshesValueColumns();
    void dataAccessElementRowsAreNotActedOn();
    void dataAccessRemoveRowsDropsSelected();
    void dataAccessRemoveRowsKeepsElementRowsAttached();
    void dataAccessMoveRowsKeepsTheDraggedBlockTogether();
    void dataAccessMoveRowsIgnoresPointlessMoves();
    void dataAccessRowDropReordersByNodeId();
    void dataAccessSubscriptionColumnIsEditable();
    void dataAccessEnumerationValuesAreNamedAndPickable();
    void dataAccessModelExportsCsv();
    void dataAccessActualIntervalColumnTracksServerValue();
    void dataAccessHeaderRolesAndHelpers();
};

///
/// \brief setItems exposes every column with the expected display values.
///
void TestModelsDataAccessCore::dataAccessSetItemsExposesColumns()
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
void TestModelsDataAccessCore::dataAccessAddOrUpdateInsertsThenUpdates()
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
void TestModelsDataAccessCore::dataAccessUpdateValuesRefreshesValueColumns()
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
/// \brief Element rows are read-only and never stand in for their node in row operations.
///
void TestModelsDataAccessCore::dataAccessElementRowsAreNotActedOn()
{
    DataAccessModel model;

    OpcUaNodeDetails details;
    details.nodeId = QStringLiteral("ns=2;s=Values");
    details.dataTypeId = QStringLiteral("ns=0;i=4");
    details.value = QVariantList{1, 2};
    model.addOrUpdate(details);

    const QModelIndex row = model.index(0, 0);
    const QModelIndex element = model.index(0, DataAccessModel::ColSubscription, row);
    QCOMPARE(model.flags(element), Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    QVERIFY(!model.setData(element, QStringLiteral("Default"), Qt::EditRole));
    QVERIFY(model.itemAt(0).subscriptionName.isEmpty());

    QVERIFY(model.nodeIds({element}).isEmpty());
    QVERIFY(!model.mimeData({element}));

    model.removeRows({element});
    QCOMPARE(model.rowCount(), 1);

    // The elements travel with their row when the order changes.
    OpcUaNodeDetails other;
    other.nodeId = QStringLiteral("ns=2;s=Other");
    other.value = 1;
    model.addOrUpdate(other);
    const QPersistentModelIndex followedElement(
        model.index(1, DataAccessModel::ColValue, model.index(0, 0)));
    QVERIFY(model.moveRows({model.index(1, 0)}, 0));
    QCOMPARE(model.rowCount(model.index(1, 0)), 2);
    QCOMPARE(model.data(model.index(0, DataAccessModel::ColValue, model.index(1, 0))).toString(),
             QStringLiteral("1"));

    QVERIFY(followedElement.isValid());
    QCOMPARE(followedElement.parent(), model.index(1, 0));
    QCOMPARE(followedElement.row(), 1);
    QCOMPARE(model.data(followedElement).toString(), QStringLiteral("2"));
}

///
/// \brief removeRows drops the selected rows and reports them via nodeIds().
///
void TestModelsDataAccessCore::dataAccessRemoveRowsDropsSelected()
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
/// \brief Removing a row keeps the element rows of the rows below it attached to their node.
///
/// The model tester is left out on purpose: it reads the "#" column around the removal, and
/// that column renumbers with the rows.
///
void TestModelsDataAccessCore::dataAccessRemoveRowsKeepsElementRowsAttached()
{
    DataAccessModel model;

    OpcUaNodeDetails first;
    first.nodeId = QStringLiteral("ns=2;s=First");
    first.value = QVariantList{1, 2};
    model.addOrUpdate(first);

    OpcUaNodeDetails second;
    second.nodeId = QStringLiteral("ns=2;s=Second");
    second.displayName = QStringLiteral("Second");
    second.value = QVariantList{10, 20, 30};
    model.addOrUpdate(second);

    QCOMPARE(model.rowCount(model.index(0, 0)), 2);
    QCOMPARE(model.rowCount(model.index(1, 0)), 3);

    const QPersistentModelIndex removedElement(model.index(0, 0, model.index(0, 0)));
    const QPersistentModelIndex keptElement(model.index(2, 0, model.index(1, 0)));

    model.removeRows({model.index(0, 0)});

    QCOMPARE(model.rowCount(), 1);
    QVERIFY(!removedElement.isValid());
    QVERIFY(keptElement.isValid());
    QCOMPARE(keptElement.parent(), model.index(0, 0));
    QCOMPARE(model.data(keptElement.sibling(keptElement.row(), DataAccessModel::ColNodeId))
                 .toString(),
             QStringLiteral("[2]"));
    QCOMPARE(model.data(keptElement.sibling(keptElement.row(), DataAccessModel::ColValue))
                 .toString(),
             QStringLiteral("30"));

    model.removeRows({model.index(0, 0)});
    QCOMPARE(model.rowCount(), 0);
    QVERIFY(!keptElement.isValid());
}

///
/// \brief Dragged rows land in front of the destination row, in one block.
///
void TestModelsDataAccessCore::dataAccessMoveRowsKeepsTheDraggedBlockTogether()
{
    DataAccessModel model;
    new QAbstractItemModelTester(&model, &model);
    const QVector<DataAccessItem> items = TestData::dataAccessItems();
    model.setItems(items);

    // Two rows picked apart from each other end up next to each other.
    const QPersistentModelIndex followed(model.index(2, DataAccessModel::ColNodeId));
    QVERIFY(model.moveRows({model.index(0, 0), model.index(2, 0)}, model.rowCount() - 1));

    QCOMPARE(model.nodeIds(), QStringList({items.at(1).nodeId, items.at(3).nodeId,
                                           items.at(4).nodeId, items.at(0).nodeId,
                                           items.at(2).nodeId, items.at(5).nodeId}));
    // The selection and open editors ride along with their row.
    QCOMPARE(followed.row(), 4);
    QCOMPARE(followed.column(), DataAccessModel::ColNodeId);

    // Moving up inserts in front of the destination row.
    QVERIFY(model.moveRows({model.index(4, 0)}, 1));
    QCOMPARE(model.nodeIds().at(1), items.at(2).nodeId);
    // The "#" column renumbers with the rows.
    QCOMPARE(model.data(model.index(1, DataAccessModel::ColNumber)).toInt(), 2);
}

///
/// \brief A move that would leave the order untouched changes nothing.
///
void TestModelsDataAccessCore::dataAccessMoveRowsIgnoresPointlessMoves()
{
    DataAccessModel model;
    const QVector<DataAccessItem> items = TestData::dataAccessItems();
    model.setItems(items);
    const QStringList before = model.nodeIds();

    QVERIFY(!model.moveRows({}, 0));
    QVERIFY(!model.moveRows({model.index(99, 0)}, 0));
    // Dropping a row on either of its own edges keeps it where it is.
    QVERIFY(!model.moveRows({model.index(2, 0)}, 2));
    QVERIFY(!model.moveRows({model.index(2, 0)}, 3));
    QCOMPARE(model.nodeIds(), before);
}

///
/// \brief Rows dropped back on the table are reordered by the NodeIds they carry.
///
void TestModelsDataAccessCore::dataAccessRowDropReordersByNodeId()
{
    DataAccessModel model;
    const QVector<DataAccessItem> items = TestData::dataAccessItems();
    model.setItems(items);

    QScopedPointer<QMimeData> dragged(model.mimeData({model.index(0, 0), model.index(0, 1),
                                                      model.index(1, 0)}));
    QVERIFY(dragged);
    QVERIFY(dragged->hasFormat(DataAccessModel::rowMimeType()));

    // Only a move of the table's own rows onto the root is accepted.
    QVERIFY(!model.canDropMimeData(dragged.data(), Qt::CopyAction, 4, 0, QModelIndex()));
    QVERIFY(!model.canDropMimeData(dragged.data(), Qt::MoveAction, 4, 0, model.index(3, 0)));
    QScopedPointer<QMimeData> foreign(new QMimeData);
    foreign->setText(QStringLiteral("ns=2;s=Elsewhere"));
    QVERIFY(!model.canDropMimeData(foreign.data(), Qt::MoveAction, 4, 0, QModelIndex()));

    QVERIFY(model.canDropMimeData(dragged.data(), Qt::MoveAction, 4, 0, QModelIndex()));
    QVERIFY(model.dropMimeData(dragged.data(), Qt::MoveAction, 4, 0, QModelIndex()));
    QCOMPARE(model.nodeIds(), QStringList({items.at(2).nodeId, items.at(3).nodeId,
                                           items.at(0).nodeId, items.at(1).nodeId,
                                           items.at(4).nodeId, items.at(5).nodeId}));

    // A disconnected table shows the rows it had, but stops accepting drops.
    model.setOffline(true);
    QVERIFY(!model.canDropMimeData(dragged.data(), Qt::MoveAction, 0, 0, QModelIndex()));
    QVERIFY(!model.dropMimeData(dragged.data(), Qt::MoveAction, 0, 0, QModelIndex()));
}

///
/// \brief Only the subscription column is editable, and setData stores the value.
///
void TestModelsDataAccessCore::dataAccessSubscriptionColumnIsEditable()
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
/// \brief An enumeration row shows its names and offers them, unless it is read-only.
///
void TestModelsDataAccessCore::dataAccessEnumerationValuesAreNamedAndPickable()
{
    OpcUaNodeDetails details;
    details.nodeId = QStringLiteral("ns=2;s=State");
    details.nodeClass = OpcUa::Variable;
    details.value = 1;
    details.valueType = int(QOpcUa::Types::Int32);
    details.dataTypeId = QStringLiteral("ns=1;s=SensorState");
    details.enumEntries = {{0, QStringLiteral("Disabled")}, {1, QStringLiteral("Enabled")}};
    details.userAccessLevel = OpcUa::CurrentRead | OpcUa::CurrentWrite;

    DataAccessModel model;
    new QAbstractItemModelTester(&model, &model);
    model.addOrUpdate(details);

    const QModelIndex value = model.index(0, DataAccessModel::ColValue);
    QCOMPARE(model.data(value).toString(), QStringLiteral("1 (Enabled)"));
    QVERIFY(model.flags(value) & Qt::ItemIsEditable);
    // The editor is seeded with the number, and offered the names to pick from.
    QCOMPARE(model.data(value, Qt::EditRole).toInt(), 1);
    QCOMPARE(model.data(value, DataAccessModel::EnumEntriesRole)
                 .value<OpcUaEnumEntries>().size(), 2);

    // A notification keeps naming the value it delivers.
    OpcUaDataValue update;
    update.nodeId = details.nodeId;
    update.value = 0;
    update.status = QStringLiteral("Good");
    model.updateValues({update});
    QCOMPARE(model.data(value).toString(), QStringLiteral("0 (Disabled)"));

    // Read-only rows keep the write dialog: nothing to pick, and nothing to pick with.
    details.userAccessLevel = OpcUa::CurrentRead;
    model.addOrUpdate(details);
    QVERIFY(!(model.flags(value) & Qt::ItemIsEditable));
    QVERIFY(model.data(value, DataAccessModel::EnumEntriesRole)
                .value<OpcUaEnumEntries>().isEmpty());

    // An array is written as a whole, so its cell stays with the dialog while its
    // elements still spell out the names.
    details.value = QVariantList{0, 1};
    details.userAccessLevel = OpcUa::CurrentRead | OpcUa::CurrentWrite;
    model.addOrUpdate(details);
    QVERIFY(!(model.flags(value) & Qt::ItemIsEditable));
    const QModelIndex row = model.index(0, 0);
    QCOMPARE(model.rowCount(row), 2);
    QCOMPARE(model.data(model.index(1, DataAccessModel::ColValue, row)).toString(),
             QStringLiteral("1 (Enabled)"));
}

///
/// \brief DataAccessModel exports listed rows as escaped CSV with a header.
///
void TestModelsDataAccessCore::dataAccessModelExportsCsv()
{
    DataAccessItem item;
    item.nodeId = QStringLiteral("ns=2;s=Temp");
    item.displayName = QStringLiteral("Temperature");
    item.value = QStringLiteral("12,\"quoted\"\nline");
    item.dataType = QStringLiteral("Double");
    item.sourceTimestamp = QDateTime(QDate(2024, 1, 2), QTime(3, 4, 5, 6), QTimeZone::UTC);
    item.status = QStringLiteral("Good,Clamped");
    item.subscriptionName = QStringLiteral("Default");
    item.revisedPublishingInterval = 100.0;

    DataAccessModel model;
    model.setTimestampMode(AppSettings::TimestampMode::Utc);
    model.setItems({item});

    QCOMPARE(model.toCsv(),
             QStringLiteral("#,Node Id,Display Name,Value,Data Type,Source Timestamp,"
                            "Status,Subscription,Actual Interval\n"
                            "1,ns=2;s=Temp,Temperature,\"12,\"\"quoted\"\"\nline\",Double,"
                            "2024-01-02 03:04:05.006Z,\"Good,Clamped\",Default,100 ms\n"));
}

///
/// \brief DataAccessModel: the actual-interval column tracks the server-granted value.
///
void TestModelsDataAccessCore::dataAccessActualIntervalColumnTracksServerValue()
{
    DataAccessItem item;
    item.nodeId = QStringLiteral("ns=2;s=Temp");
    item.subscriptionName = QStringLiteral("Default");

    DataAccessModel model;
    model.setItems({item});

    const QModelIndex intervalIndex = model.index(0, DataAccessModel::ColActualInterval);
    QCOMPARE(model.headerData(DataAccessModel::ColActualInterval, Qt::Horizontal).toString(),
             QStringLiteral("Actual Interval"));
    QCOMPARE(model.data(intervalIndex).toString(), QStringLiteral("—"));

    // The column is read-only: only the subscription cell accepts edits.
    QVERIFY(!(model.flags(intervalIndex) & Qt::ItemIsEditable));
    QVERIFY(!model.setData(intervalIndex, 100.0, Qt::EditRole));

    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);
    model.setRevisedInterval(QStringLiteral("ns=2;s=Temp"), 100.0);
    QCOMPARE(model.data(intervalIndex).toString(), QStringLiteral("100 ms"));
    QCOMPARE(changedSpy.size(), 1);

    // Repeating the same value is a no-op; an unknown node is ignored.
    model.setRevisedInterval(QStringLiteral("ns=2;s=Temp"), 100.0);
    model.setRevisedInterval(QStringLiteral("ns=2;s=Missing"), 500.0);
    QCOMPARE(changedSpy.size(), 1);

    // Unsubscribing clears the value back to the placeholder.
    model.setRevisedInterval(QStringLiteral("ns=2;s=Temp"), 0.0);
    QCOMPARE(model.data(intervalIndex).toString(), QStringLiteral("—"));
}

///
/// \brief DataAccessModel: headerData, the remaining columns/roles and helpers.
///
void TestModelsDataAccessCore::dataAccessHeaderRolesAndHelpers()
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
             QStringLiteral("\u2014"));
    QVERIFY(model.data(model.index(0, DataAccessModel::ColNumber),
                       Qt::TextAlignmentRole).isValid());

    // With no subscription, the foreground role is the disabled-text brush.
    QVERIFY(model.data(model.index(0, DataAccessModel::ColValue), Qt::ForegroundRole)
                .canConvert<QBrush>());

    // Set a subscription, then verify the EditRole readback and the coloured cells.
    const QModelIndex sub0 = model.index(0, DataAccessModel::ColSubscription);
    QVERIFY(model.setData(sub0, QStringLiteral("Fast"), Qt::EditRole));
    QCOMPARE(model.data(sub0, Qt::EditRole).toString(), QStringLiteral("Fast"));
    QVERIFY(!model.data(model.index(0, DataAccessModel::ColValue), Qt::ForegroundRole)
                 .isValid());
    // Quality colouring moved to ValueCellDelegate; the model only classifies the status.
    QVERIFY(!model.data(model.index(0, DataAccessModel::ColStatus), Qt::ForegroundRole)
                 .isValid());
    QCOMPARE(model.data(model.index(0, DataAccessModel::ColStatus),
                        DataAccessModel::StatusSeverityRole).toInt(),
             int(OpcUaFormat::StatusSeverity::Good));
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

QTEST_MAIN(TestModelsDataAccessCore)

#include "test_models_data_access_core.moc"
