// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_models_opcua_capabilities.cpp
/// \brief Tests OPC UA node capability predicates.
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
class TestModelsOpcUaCapabilities : public QObject
{
    Q_OBJECT

private slots:
    void historyReadRequiresHistorizingVariable();
    void eventMonitoringRequiresEventNotifier();
    void eventHistoryReadRequiresHistoryReadNotifier();
};

///
/// \brief HistoryRead availability requires a historizing variable node.
///
void TestModelsOpcUaCapabilities::historyReadRequiresHistorizingVariable()
{
    OpcUaNodeDetails details;
    details.nodeClass = OpcUa::Variable;

    QVERIFY(!OpcUa::canReadHistory(details));

    details.historizing = true;
    QCOMPARE(OpcUa::canReadHistory(details), OpcUa::isHistoryReadSupported());

    details.nodeClass = OpcUa::Object;
    QVERIFY(!OpcUa::canReadHistory(details));

    OpcUaNodeInfo node;
    node.nodeClass = OpcUa::Variable;
    QVERIFY(!OpcUa::canReadHistory(node));

    node.historizing = true;
    QCOMPARE(OpcUa::canReadHistory(node), OpcUa::isHistoryReadSupported());

    node.nodeClass = OpcUa::Object;
    QVERIFY(!OpcUa::canReadHistory(node));
}

///
/// \brief Event monitoring availability requires the SubscribeToEvents bit.
///
void TestModelsOpcUaCapabilities::eventMonitoringRequiresEventNotifier()
{
    OpcUaNodeDetails details;
    details.nodeClass = OpcUa::Object;

    QVERIFY(!OpcUa::canMonitorEvents(details));

    details.eventNotifier = OpcUa::SubscribeToEvents;
    QVERIFY(OpcUa::canMonitorEvents(details));

    OpcUaNodeInfo node;
    node.nodeClass = OpcUa::Object;
    QVERIFY(!OpcUa::canMonitorEvents(node));

    node.eventNotifier = OpcUa::SubscribeToEvents;
    QVERIFY(OpcUa::canMonitorEvents(node));
}

///
/// \brief Event history availability requires the EventNotifier HistoryRead bit.
///
void TestModelsOpcUaCapabilities::eventHistoryReadRequiresHistoryReadNotifier()
{
    OpcUaNodeDetails details;
    details.nodeClass = OpcUa::Object;

    QVERIFY(!OpcUa::canReadEventHistory(details));

    details.eventNotifier = OpcUa::SubscribeToEvents;
    QVERIFY(!OpcUa::canReadEventHistory(details));

    details.eventNotifier = OpcUa::HistoryRead;
    QCOMPARE(OpcUa::canReadEventHistory(details), OpcUa::isHistoryReadSupported());

    OpcUaNodeInfo node;
    node.nodeClass = OpcUa::Object;
    node.eventNotifier = OpcUa::SubscribeToEvents;
    QVERIFY(!OpcUa::canReadEventHistory(node));

    node.eventNotifier = OpcUa::HistoryRead;
    QCOMPARE(OpcUa::canReadEventHistory(node), OpcUa::isHistoryReadSupported());
}


QTEST_MAIN(TestModelsOpcUaCapabilities)

#include "test_models_opcua_capabilities.moc"
