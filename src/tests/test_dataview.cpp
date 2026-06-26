// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_dataview.cpp
/// \brief Tests the DataView tab container behaviour.
///

#include <QTabWidget>
#include <QTableView>
#include <QTest>

#include "opcua/opcuatypes.h"
#include "testdata.h"
#include "widgets/dataview.h"

///
/// \brief UI tests for DataView.
///
class TestDataView : public QObject
{
    Q_OBJECT

private slots:
    void historyTabFollowsQtSupport();
    void clearRuntimeDataResetsTabs();
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
/// \brief The History page is only visible when Qt OPC UA can perform HistoryRead.
///
void TestDataView::historyTabFollowsQtSupport()
{
    DataView view;
    auto *tabs = view.findChild<QTabWidget *>(QStringLiteral("mainTabs"));
    QVERIFY(tabs);

    QCOMPARE(tabs->isTabVisible(DataView::HistoryPage), OpcUa::isHistoryReadSupported());
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
        view.setHistoryResults(TestData::historyItems());
        auto *historyTable = view.findChild<QTableView *>(QStringLiteral("historyTable"));
        QVERIFY(historyTable);
        QCOMPARE(historyTable->model()->rowCount(), TestData::historyItems().size());

        view.clearRuntimeData();
        QCOMPARE(historyTable->model()->rowCount(), 0);
    } else {
        view.clearRuntimeData();
    }

    QCOMPARE(dataTable->model()->rowCount(), 0);
}

QTEST_MAIN(TestDataView)

#include "test_dataview.moc"
