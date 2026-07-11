// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_datahistorywidget.cpp
/// \brief Tests DataHistoryWidget query, export and clear behaviour.
///

#include <QAction>
#include <QDateTimeEdit>
#include <QSignalSpy>
#include <QSpinBox>
#include <QTableView>
#include <QTest>
#include <QTimeZone>
#include <QToolButton>

#include "opcua/opcuatypes.h"
#include "testdata.h"
#include "widgets/datahistorywidget.h"
#include "widgets/themedtoolbutton.h"
#include "widgets/valuelineedit.h"

///
/// \brief UI tests for DataHistoryWidget.
///
class TestDataHistoryWidget : public QObject
{
    Q_OBJECT

private slots:
    void exportButtonFollowsResults();
    void exportFileNameDescribesQuery();
    void dateTimeFieldsFitZoneSuffix();
    void nodeFieldShowsNameAndNodeId();
    void nodeClearClearsResults();
    void clearButtonUsesTrashIcon();
};

///
/// \brief The data history export button is enabled only when there are rows to save.
///
void TestDataHistoryWidget::exportButtonFollowsResults()
{
    if (!OpcUa::isHistoryReadSupported())
        QSKIP("HistoryRead is not supported by this Qt OPC UA build.");

    DataHistoryWidget widget;
    auto *button = widget.findChild<QToolButton *>(QStringLiteral("dataHistoryExportButton"));
    QVERIFY(button);
    QVERIFY(!button->isEnabled());

    OpcUaHistoryValue value;
    value.value = 42;
    widget.setDataHistoryResults({value});
    QVERIFY(button->isEnabled());

    widget.setDataHistoryResults({});
    QVERIFY(!button->isEnabled());
}

///
/// \brief The suggested CSV file name includes tag, interval, and max limit.
///
void TestDataHistoryWidget::exportFileNameDescribesQuery()
{
    if (!OpcUa::isHistoryReadSupported())
        QSKIP("HistoryRead is not supported by this Qt OPC UA build.");

    DataHistoryWidget widget;
    widget.requestDataHistoryForNode(QStringLiteral("ns=2;s=Temperature"),
                                 QStringLiteral("Area 1/Temperature:PV"));

    auto *startEdit = widget.findChild<QDateTimeEdit *>(QStringLiteral("dataHistoryStartEdit"));
    auto *endEdit = widget.findChild<QDateTimeEdit *>(QStringLiteral("dataHistoryEndEdit"));
    auto *maxEdit = widget.findChild<QSpinBox *>(QStringLiteral("dataHistoryMaxEdit"));
    QVERIFY(startEdit);
    QVERIFY(endEdit);
    QVERIFY(maxEdit);

    startEdit->setDateTime(QDateTime(QDate(2026, 6, 25), QTime(12, 41, 36), QTimeZone::UTC));
    endEdit->setDateTime(QDateTime(QDate(2026, 6, 25), QTime(13, 41, 36), QTimeZone::UTC));
    maxEdit->setValue(1000);

    QCOMPARE(widget.suggestedDataHistoryCsvFileName(),
             QStringLiteral("Area_1_Temperature_PV_20260625_124136_20260625_134136_max1000.csv"));

    maxEdit->setValue(maxEdit->minimum());
    QCOMPARE(widget.suggestedDataHistoryCsvFileName(),
             QStringLiteral("Area_1_Temperature_PV_20260625_124136_20260625_134136.csv"));
}

///
/// \brief The Data History date-time fields leave room for the time-zone suffix.
///
void TestDataHistoryWidget::dateTimeFieldsFitZoneSuffix()
{
    DataHistoryWidget widget;

    auto *startEdit = widget.findChild<QDateTimeEdit *>(QStringLiteral("dataHistoryStartEdit"));
    auto *endEdit = widget.findChild<QDateTimeEdit *>(QStringLiteral("dataHistoryEndEdit"));
    QVERIFY(startEdit);
    QVERIFY(endEdit);

    QVERIFY(startEdit->minimumWidth() >= 190);
    QVERIFY(endEdit->minimumWidth() >= 190);
}

///
/// \brief The selected Data History node field shows both display name and NodeId.
///
void TestDataHistoryWidget::nodeFieldShowsNameAndNodeId()
{
    if (!OpcUa::isHistoryReadSupported())
        QSKIP("HistoryRead is not supported by this Qt OPC UA build.");

    DataHistoryWidget widget;
    auto *nodeEdit = widget.findChild<ValueLineEdit *>(QStringLiteral("dataHistoryNodeEdit"));
    QVERIFY(nodeEdit);

    widget.requestDataHistoryForNode(QStringLiteral("ns=2;s=Temperature"), QStringLiteral("Temperature"));
    QCOMPARE(nodeEdit->text(), QStringLiteral("Temperature (ns=2;s=Temperature)"));
    QCOMPARE(nodeEdit->toolTip(), QStringLiteral("ns=2;s=Temperature"));

    widget.requestDataHistoryForNode(QStringLiteral("ns=2;s=Temperature"),
                                 QStringLiteral("Temperature"),
                                 QStringLiteral("Objects/Device/Temperature"));
    QCOMPARE(nodeEdit->text(), QStringLiteral("Objects/Device/Temperature (ns=2;s=Temperature)"));

    widget.requestDataHistoryForNode(QStringLiteral("ns=2;s=Temperature"), QStringLiteral("ns=2;s=Temperature"));
    QCOMPARE(nodeEdit->text(), QStringLiteral("ns=2;s=Temperature"));
}

///
/// \brief Clearing the Data History node field clears the selected node and result rows.
///
void TestDataHistoryWidget::nodeClearClearsResults()
{
    if (!OpcUa::isHistoryReadSupported())
        QSKIP("HistoryRead is not supported by this Qt OPC UA build.");

    DataHistoryWidget widget;
    widget.requestDataHistoryForNode(QStringLiteral("ns=2;s=Temperature"), QStringLiteral("Temperature"));
    widget.setDataHistoryResults(TestData::historyItems());

    auto *nodeEdit = widget.findChild<ValueLineEdit *>(QStringLiteral("dataHistoryNodeEdit"));
    auto *table = widget.findChild<QTableView *>(QStringLiteral("dataHistoryTable"));
    auto *readButton = widget.findChild<QToolButton *>(QStringLiteral("dataHistoryReadButton"));
    QVERIFY(nodeEdit);
    QVERIFY(table);
    QVERIFY(readButton);
    QCOMPARE(table->model()->rowCount(), TestData::historyItems().size());
    QCOMPARE(nodeEdit->defaultValue(), QString());
    QCOMPARE(nodeEdit->actions().size(), 1);
    QVERIFY(nodeEdit->actions().constFirst()->isVisible());

    QSignalSpy readSpy(&widget, &DataHistoryWidget::dataHistoryReadRequested);
    nodeEdit->actions().constFirst()->trigger();

    QCOMPARE(table->model()->rowCount(), 0);
    QVERIFY(nodeEdit->text().isEmpty());
    QVERIFY(nodeEdit->toolTip().isEmpty());
    QVERIFY(!nodeEdit->actions().constFirst()->isVisible());

    readButton->click();
    QCOMPARE(readSpy.size(), 0);
}

///
/// \brief The data history Clear button uses the trash icon.
///
void TestDataHistoryWidget::clearButtonUsesTrashIcon()
{
    DataHistoryWidget widget;
    auto *button = widget.findChild<ThemedToolButton *>(QStringLiteral("dataHistoryClearButton"));
    QVERIFY(button);
    QCOMPARE(button->iconName(), QStringLiteral("trash"));
}

QTEST_MAIN(TestDataHistoryWidget)

#include "test_datahistorywidget.moc"
