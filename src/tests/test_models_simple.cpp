// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_models_simple.cpp
/// \brief Tests structural contracts shared by the simple models.
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
class TestModelsSimple : public QObject
{
    Q_OBJECT

private slots:
    void simpleModelsAreStructurallyValid();
};

///
/// \brief Runs each simple model through QAbstractItemModelTester after loading
///        the shared sample data, catching contract violations in one place.
///
void TestModelsSimple::simpleModelsAreStructurallyValid()
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
    QCOMPARE(history.columnCount(), int(HistoryModel::ColCount));
    QCOMPARE(history.data(history.index(0, HistoryModel::ColNumber)).toString(),
             QStringLiteral("1"));
    QCOMPARE(history.headerData(HistoryModel::ColValue, Qt::Horizontal).toString(),
             QStringLiteral("Value"));
    history.clear();
    QCOMPARE(history.rowCount(), 0);

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


QTEST_MAIN(TestModelsSimple)

#include "test_models_simple.moc"
