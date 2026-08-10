// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_dataview.cpp
/// \brief Tests the DataView tab container behaviour.
///

#include <QAbstractButton>
#include <QTabBar>
#include <QTabWidget>
#include <QTableView>
#include <QTreeView>
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
    void pagesUseLegacyValues();
    void closedTabReopensFromTheMenuPage();
    void closedPagesSkipUnsupportedHistoryTabs();
    void addingNodeReopensTheDataAccessTab();
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

///
/// \brief Returns the close button of a tab, whichever side the style puts it on.
/// \param bar Tab bar to query.
/// \param index Tab to query.
/// \return Close button, or nullptr when the tab has none.
///
QAbstractButton *tabCloseButton(QTabBar *bar, int index)
{
    if (auto *right = qobject_cast<QAbstractButton *>(bar->tabButton(index, QTabBar::RightSide)))
        return right;
    return qobject_cast<QAbstractButton *>(bar->tabButton(index, QTabBar::LeftSide));
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

    QCOMPARE(tabs->count(), 4);
    for (int index = 0; index < tabs->count(); ++index)
        QVERIFY(tabs->tabText(index) != QStringLiteral("Subscriptions"));
    QCOMPARE(tabs->isTabVisible(2), OpcUa::isHistoryReadSupported());
    QCOMPARE(tabs->isTabVisible(3), OpcUa::isHistoryReadSupported());
    QCOMPARE(tabs->tabText(2), QStringLiteral("Data History"));
    QCOMPARE(tabs->tabText(3), QStringLiteral("Events History"));
}

///
/// \brief Page values remain compatible with saved settings after removing Subscriptions.
///
void TestDataView::pagesUseLegacyValues()
{
    DataView view;
    auto *tabs = view.findChild<QTabWidget *>(QStringLiteral("mainTabs"));
    QVERIFY(tabs);

    view.setCurrentPage(DataView::EventsPage);
    QCOMPARE(tabs->currentIndex(), 1);
    QCOMPARE(view.currentPage(), int(DataView::EventsPage));

    view.setCurrentPage(static_cast<DataView::Page>(1));
    QCOMPARE(tabs->currentIndex(), 0);
    QCOMPARE(view.currentPage(), int(DataView::DataAccessPage));

    view.setCurrentPage(static_cast<DataView::Page>(99));
    QCOMPARE(tabs->currentIndex(), 0);
    QCOMPARE(view.currentPage(), int(DataView::DataAccessPage));
}

///
/// \brief The close button hides a page until the View menu opens it again.
///
void TestDataView::closedTabReopensFromTheMenuPage()
{
    DataView view;
    auto *tabs = view.findChild<QTabWidget *>(QStringLiteral("mainTabs"));
    QVERIFY(tabs);
    QVERIFY(tabs->tabsClosable());

    QAbstractButton *closeButton = tabCloseButton(tabs->tabBar(), 1);
    QVERIFY(closeButton);
    closeButton->click();

    QVERIFY(!view.isPageVisible(DataView::EventsPage));
    QVERIFY(tabs->currentIndex() != 1);
    const QList<int> closed{int(DataView::EventsPage)};
    QCOMPARE(view.closedPages(), closed);

    view.setCurrentPage(DataView::EventsPage);

    QVERIFY(view.isPageVisible(DataView::EventsPage));
    QCOMPARE(tabs->currentIndex(), 1);
    QVERIFY(view.closedPages().isEmpty());
}

///
/// \brief Pages the linked Qt OPC UA API cannot serve are never reported or reopened.
///
void TestDataView::closedPagesSkipUnsupportedHistoryTabs()
{
    DataView view;
    view.setClosedPages({int(DataView::DataAccessPage)});

    QVERIFY(!view.isPageVisible(DataView::DataAccessPage));
    QVERIFY(view.isPageVisible(DataView::EventsPage));

    view.setPageVisible(DataView::DataHistoryPage, true);
    QCOMPARE(view.isPageVisible(DataView::DataHistoryPage), OpcUa::isHistoryReadSupported());

    const QList<int> closed = view.closedPages();
    QVERIFY(closed.contains(int(DataView::DataAccessPage)));
    QCOMPARE(closed.contains(int(DataView::DataHistoryPage)), false);

    view.setClosedPages({});
    QVERIFY(view.isPageVisible(DataView::DataAccessPage));
    QVERIFY(view.closedPages().isEmpty());
}

///
/// \brief Adding a node brings the Data Access tab back when the user closed it.
///
void TestDataView::addingNodeReopensTheDataAccessTab()
{
    DataView view;
    view.setClosedPages({int(DataView::DataAccessPage)});
    QVERIFY(!view.isPageVisible(DataView::DataAccessPage));

    view.addNode(makeNodeDetails());

    QVERIFY(view.isPageVisible(DataView::DataAccessPage));
    QCOMPARE(view.currentPage(), int(DataView::DataAccessPage));
}

///
/// \brief clearRuntimeData empties the data and history tabs.
///
void TestDataView::clearRuntimeDataResetsTabs()
{
    DataView view;
    auto *dataTable = view.findChild<QTreeView *>(QStringLiteral("dataView"));
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
    if (!OpcUa::isHistoryReadSupported())
        QSKIP("HistoryRead is not supported by the linked Qt OPC UA API.");

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
