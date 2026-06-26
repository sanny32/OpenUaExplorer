// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_dataview.cpp
/// \brief Tests the DataView tab container behaviour.
///

#include <QTabWidget>
#include <QTableView>
#include <QSignalSpy>
#include <QTest>

#include "opcua/opcuatypes.h"
#include "testdata.h"
#include "widgets/dataview.h"
#include "widgets/eventshistorywidget.h"
#include "widgets/eventswidget.h"

///
/// \brief UI tests for DataView.
///
class TestDataView : public QObject
{
    Q_OBJECT

private slots:
    void historyTabsFollowQtSupport();
    void clearRuntimeDataResetsTabs();
    void eventMonitoringRequestTargetsNodeAndSubscribes();
    void eventsHistoryRequestTargetsNodeAndReads();
};

namespace {

///
/// \brief Builds node details for Data View tests.
/// \return Node details item.
///
OpcUaNodeDetails makeNodeDetails()
{
    OpcUaNodeDetails details;
    details.nodeId = QStringLiteral("ns=2;s=Temperature");
    details.displayName = QStringLiteral("Temperature");
    details.nodeClass = OpcUa::Variable;
    details.value = 21.5;
    details.dataTypeId = QStringLiteral("ns=0;i=11");
    details.status = QStringLiteral("Good");
    return details;
}

} // namespace

///
/// \brief The history pages are only visible when Qt OPC UA can perform HistoryRead.
///
void TestDataView::historyTabsFollowQtSupport()
{
    DataView view;
    auto *tabs = view.findChild<QTabWidget *>(QStringLiteral("mainTabs"));
    QVERIFY(tabs);

    QCOMPARE(tabs->isTabVisible(DataView::DataHistoryPage), OpcUa::isHistoryReadSupported());
    QCOMPARE(tabs->isTabVisible(DataView::EventsHistoryPage), OpcUa::isHistoryReadSupported());
    QCOMPARE(tabs->tabText(DataView::DataHistoryPage), QStringLiteral("Data History"));
    QCOMPARE(tabs->tabText(DataView::EventsHistoryPage), QStringLiteral("Events History"));
}

///
/// \brief clearRuntimeData empties the data and history tabs.
///
void TestDataView::clearRuntimeDataResetsTabs()
{
    DataView view;
    auto *dataTable = view.findChild<QTableView *>(QStringLiteral("dataView"));
    QVERIFY(dataTable);

    view.addNode(makeNodeDetails());
    QCOMPARE(dataTable->model()->rowCount(), 1);

    if (OpcUa::isHistoryReadSupported()) {
        view.setDataHistoryResults(TestData::historyItems());
        auto *dataHistoryTable = view.findChild<QTableView *>(QStringLiteral("dataHistoryTable"));
        auto *eventsHistoryTable = view.findChild<QTableView *>(QStringLiteral("eventsHistoryTable"));
        QVERIFY(dataHistoryTable);
        QVERIFY(eventsHistoryTable);
        QCOMPARE(dataHistoryTable->model()->rowCount(), TestData::historyItems().size());
        view.setEventsHistoryResults({{QStringLiteral("ns=0;i=2253"), QDateTime::currentDateTime(),
                                       500, QStringLiteral("Server"), QStringLiteral("Started"),
                                       QStringLiteral("ns=0;i=2132"), {}}});
        QCOMPARE(eventsHistoryTable->model()->rowCount(), 1);

        view.clearRuntimeData();
        QCOMPARE(dataHistoryTable->model()->rowCount(), 0);
        QCOMPARE(eventsHistoryTable->model()->rowCount(), 0);
    } else {
        view.clearRuntimeData();
    }

    QCOMPARE(dataTable->model()->rowCount(), 0);
}

///
/// \brief Requesting event monitoring opens Events and emits a subscribe request.
///
void TestDataView::eventMonitoringRequestTargetsNodeAndSubscribes()
{
    DataView view;
    QSignalSpy spy(view.events(), &EventsWidget::eventSubscribeRequested);
    QVERIFY(spy.isValid());

    view.requestEventMonitoringForNode(QStringLiteral("ns=0;i=2253"), QStringLiteral("Server"));

    QCOMPARE(view.currentPage(), int(DataView::EventsPage));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(0).toString(), QStringLiteral("ns=0;i=2253"));
    QCOMPARE(spy.first().at(1).toDouble(), 1000.0);
}

///
/// \brief Requesting event history opens Events History and emits a read request.
///
void TestDataView::eventsHistoryRequestTargetsNodeAndReads()
{
    DataView view;
    QSignalSpy spy(view.eventsHistory(), &EventsHistoryWidget::eventsHistoryReadRequested);
    QVERIFY(spy.isValid());

    view.requestEventsHistoryForNode(QStringLiteral("ns=0;i=2253"), QStringLiteral("Server"));

    QCOMPARE(view.currentPage(), int(DataView::EventsHistoryPage));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(0).toString(), QStringLiteral("ns=0;i=2253"));
}

QTEST_MAIN(TestDataView)

#include "test_dataview.moc"
