// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_historywidget.cpp
/// \brief Tests HistoryWidget query, export and clear behaviour.
///

#include <QAction>
#include <QDateTimeEdit>
#include <QSignalSpy>
#include <QSpinBox>
#include <QTableView>
#include <QTest>
#include <QToolButton>

#include "opcua/opcuatypes.h"
#include "testdata.h"
#include "widgets/historywidget.h"
#include "widgets/themedtoolbutton.h"
#include "widgets/valuelineedit.h"

///
/// \brief UI tests for HistoryWidget.
///
class TestHistoryWidget : public QObject
{
    Q_OBJECT

private slots:
    void exportButtonFollowsResults();
    void exportFileNameDescribesQuery();
    void dateTimeFieldsFitZoneSuffix();
    void nodeClearClearsResults();
    void clearButtonUsesTrashIcon();
};

///
/// \brief The history export button is enabled only when there are rows to save.
///
void TestHistoryWidget::exportButtonFollowsResults()
{
    if (!OpcUa::isHistoryReadSupported())
        QSKIP("HistoryRead is not supported by this Qt OPC UA build.");

    HistoryWidget widget;
    auto *button = widget.findChild<QToolButton *>(QStringLiteral("historyExportButton"));
    QVERIFY(button);
    QVERIFY(!button->isEnabled());

    OpcUaHistoryValue value;
    value.value = 42;
    widget.setHistoryResults({value});
    QVERIFY(button->isEnabled());

    widget.setHistoryResults({});
    QVERIFY(!button->isEnabled());
}

///
/// \brief The suggested CSV file name includes tag, interval, and max limit.
///
void TestHistoryWidget::exportFileNameDescribesQuery()
{
    if (!OpcUa::isHistoryReadSupported())
        QSKIP("HistoryRead is not supported by this Qt OPC UA build.");

    HistoryWidget widget;
    widget.requestHistoryForNode(QStringLiteral("ns=2;s=Temperature"),
                                 QStringLiteral("Area 1/Temperature:PV"));

    auto *startEdit = widget.findChild<QDateTimeEdit *>(QStringLiteral("historyStartEdit"));
    auto *endEdit = widget.findChild<QDateTimeEdit *>(QStringLiteral("historyEndEdit"));
    auto *maxEdit = widget.findChild<QSpinBox *>(QStringLiteral("historyMaxEdit"));
    QVERIFY(startEdit);
    QVERIFY(endEdit);
    QVERIFY(maxEdit);

    startEdit->setDateTime(QDateTime(QDate(2026, 6, 25), QTime(12, 41, 36), Qt::UTC));
    endEdit->setDateTime(QDateTime(QDate(2026, 6, 25), QTime(13, 41, 36), Qt::UTC));
    maxEdit->setValue(1000);

    QCOMPARE(widget.suggestedHistoryCsvFileName(),
             QStringLiteral("Area_1_Temperature_PV_20260625_124136_20260625_134136_max1000.csv"));

    maxEdit->setValue(maxEdit->minimum());
    QCOMPARE(widget.suggestedHistoryCsvFileName(),
             QStringLiteral("Area_1_Temperature_PV_20260625_124136_20260625_134136.csv"));
}

///
/// \brief The History date-time fields leave room for the time-zone suffix.
///
void TestHistoryWidget::dateTimeFieldsFitZoneSuffix()
{
    HistoryWidget widget;

    auto *startEdit = widget.findChild<QDateTimeEdit *>(QStringLiteral("historyStartEdit"));
    auto *endEdit = widget.findChild<QDateTimeEdit *>(QStringLiteral("historyEndEdit"));
    QVERIFY(startEdit);
    QVERIFY(endEdit);

    QVERIFY(startEdit->minimumWidth() >= 190);
    QVERIFY(endEdit->minimumWidth() >= 190);
}

///
/// \brief Clearing the History node field clears the selected node and result rows.
///
void TestHistoryWidget::nodeClearClearsResults()
{
    if (!OpcUa::isHistoryReadSupported())
        QSKIP("HistoryRead is not supported by this Qt OPC UA build.");

    HistoryWidget widget;
    widget.requestHistoryForNode(QStringLiteral("ns=2;s=Temperature"), QStringLiteral("Temperature"));
    widget.setHistoryResults(TestData::historyItems());

    auto *nodeEdit = widget.findChild<ValueLineEdit *>(QStringLiteral("historyNodeEdit"));
    auto *table = widget.findChild<QTableView *>(QStringLiteral("historyTable"));
    auto *readButton = widget.findChild<QToolButton *>(QStringLiteral("historyReadButton"));
    QVERIFY(nodeEdit);
    QVERIFY(table);
    QVERIFY(readButton);
    QCOMPARE(table->model()->rowCount(), TestData::historyItems().size());
    QCOMPARE(nodeEdit->defaultValue(), QString());
    QCOMPARE(nodeEdit->actions().size(), 1);
    QVERIFY(nodeEdit->actions().constFirst()->isVisible());

    QSignalSpy readSpy(&widget, &HistoryWidget::historyReadRequested);
    nodeEdit->actions().constFirst()->trigger();

    QCOMPARE(table->model()->rowCount(), 0);
    QVERIFY(nodeEdit->text().isEmpty());
    QVERIFY(nodeEdit->toolTip().isEmpty());
    QVERIFY(!nodeEdit->actions().constFirst()->isVisible());

    readButton->click();
    QCOMPARE(readSpy.size(), 0);
}

///
/// \brief The history Clear button uses the trash icon.
///
void TestHistoryWidget::clearButtonUsesTrashIcon()
{
    HistoryWidget widget;
    auto *button = widget.findChild<ThemedToolButton *>(QStringLiteral("historyClearButton"));
    QVERIFY(button);
    QCOMPARE(button->iconName(), QStringLiteral("trash"));
}

QTEST_MAIN(TestHistoryWidget)

#include "test_historywidget.moc"
