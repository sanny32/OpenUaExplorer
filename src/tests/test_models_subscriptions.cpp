// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_models_subscriptions.cpp
/// \brief Tests SubscriptionsModel behavior.
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
class TestModelsSubscriptions : public QObject
{
    Q_OBJECT

private slots:
    void subscriptionsModelHeaderRolesAndReset();
    void subscriptionsModelEditingAndMutators();
    void subscriptionsModelBuiltinRowsAreEditable();
};

///
/// \brief SubscriptionsModel: headerData, the reset path in setItems and mutators.
///
void TestModelsSubscriptions::subscriptionsModelHeaderRolesAndReset()
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
void TestModelsSubscriptions::subscriptionsModelEditingAndMutators()
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

    // Values outside the offered range are clamped rather than rejected.
    QVERIFY(model.setData(intervalIndex, 1.0, Qt::EditRole));
    QCOMPARE(model.itemAt(0).publishingInterval, double(minPublishingIntervalMs));
    QVERIFY(model.setData(intervalIndex, 5000000.0, Qt::EditRole));
    QCOMPARE(model.itemAt(0).publishingInterval, double(maxPublishingIntervalMs));
    QVERIFY(model.setData(intervalIndex, 2000.0, Qt::EditRole));
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
/// \brief SubscriptionsModel: built-in rows are editable but stay visually distinct.
///
void TestModelsSubscriptions::subscriptionsModelBuiltinRowsAreEditable()
{
    SubscriptionsModel model;
    SubscriptionItem builtin;
    builtin.name = QStringLiteral("Default");
    builtin.id = DefaultSubscriptionId;
    builtin.builtin = true;
    model.setItems({builtin});

    const QModelIndex nameIndex = model.index(0, SubscriptionsModel::ColName);
    const QModelIndex intervalIndex = model.index(0, SubscriptionsModel::ColPublishingInterval);
    QVERIFY(model.flags(nameIndex) & Qt::ItemIsEditable);
    QVERIFY(model.flags(intervalIndex) & Qt::ItemIsEditable);

    QVERIFY(model.setData(nameIndex, QStringLiteral("Telemetry"), Qt::EditRole));
    QVERIFY(model.setData(intervalIndex, 50.0, Qt::EditRole));
    QCOMPARE(model.itemAt(0).name, QStringLiteral("Telemetry"));
    QCOMPARE(model.itemAt(0).publishingInterval, 50.0);
    QVERIFY(model.itemAt(0).isBuiltin());

    // The lock decoration is gone, but the shaded background still marks the row as permanent.
    QVERIFY(!model.data(nameIndex, Qt::DecorationRole).isValid());
    model.setBuiltinBackground(QBrush(Qt::gray));
    QVERIFY(model.data(nameIndex, Qt::BackgroundRole).isValid());
}


QTEST_MAIN(TestModelsSubscriptions)

#include "test_models_subscriptions.moc"
