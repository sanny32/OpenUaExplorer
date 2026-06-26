// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_eventshistorywidget.cpp
/// \brief Tests EventsHistoryWidget query, export and clear behaviour.
///

#include <QDateTimeEdit>
#include <QSignalSpy>
#include <QSpinBox>
#include <QTableView>
#include <QTest>
#include <QToolButton>

#include "widgets/eventshistorywidget.h"
#include "widgets/themedtoolbutton.h"
#include "widgets/valuelineedit.h"

///
/// \brief UI tests for EventsHistoryWidget.
///
class TestEventsHistoryWidget : public QObject
{
    Q_OBJECT

private slots:
    void exportButtonFollowsResults();
    void exportFileNameDescribesQuery();
    void dateTimeFieldsFitZoneSuffix();
    void nodeClearClearsResults();
    void readButtonEmitsQuery();
    void clearButtonUsesTrashIcon();
};

namespace {

///
/// \brief Builds one historical event for widget tests.
/// \return Event item.
///
OpcUaEvent makeEvent()
{
    OpcUaEvent event;
    event.sourceNodeId = QStringLiteral("ns=0;i=2253");
    event.time = QDateTime(QDate(2026, 6, 25), QTime(12, 41, 36), Qt::UTC);
    event.severity = 500;
    event.sourceName = QStringLiteral("Server");
    event.message = QStringLiteral("Started");
    event.eventType = QStringLiteral("ns=0;i=2132");
    return event;
}

} // namespace

///
/// \brief The export button is enabled only when there are events to save.
///
void TestEventsHistoryWidget::exportButtonFollowsResults()
{
    EventsHistoryWidget widget;
    auto *button = widget.findChild<QToolButton *>(QStringLiteral("eventsHistoryExportButton"));
    QVERIFY(button);
    QVERIFY(!button->isEnabled());

    widget.setEventsHistoryResults({makeEvent()});
    QVERIFY(button->isEnabled());

    widget.setEventsHistoryResults({});
    QVERIFY(!button->isEnabled());
}

///
/// \brief The suggested CSV file name includes tag, interval, and max limit.
///
void TestEventsHistoryWidget::exportFileNameDescribesQuery()
{
    EventsHistoryWidget widget;
    widget.requestEventsHistoryForNode(QStringLiteral("ns=0;i=2253"),
                                       QStringLiteral("Area 1/Server:Events"));

    auto *startEdit = widget.findChild<QDateTimeEdit *>(QStringLiteral("eventsHistoryStartEdit"));
    auto *endEdit = widget.findChild<QDateTimeEdit *>(QStringLiteral("eventsHistoryEndEdit"));
    auto *maxEdit = widget.findChild<QSpinBox *>(QStringLiteral("eventsHistoryMaxEdit"));
    QVERIFY(startEdit);
    QVERIFY(endEdit);
    QVERIFY(maxEdit);

    startEdit->setDateTime(QDateTime(QDate(2026, 6, 25), QTime(12, 41, 36), Qt::UTC));
    endEdit->setDateTime(QDateTime(QDate(2026, 6, 25), QTime(13, 41, 36), Qt::UTC));
    maxEdit->setValue(1000);

    QCOMPARE(widget.suggestedEventsHistoryCsvFileName(),
             QStringLiteral("Area_1_Server_Events_20260625_124136_20260625_134136_max1000.csv"));

    maxEdit->setValue(maxEdit->minimum());
    QCOMPARE(widget.suggestedEventsHistoryCsvFileName(),
             QStringLiteral("Area_1_Server_Events_20260625_124136_20260625_134136.csv"));
}

///
/// \brief The date-time fields leave room for the time-zone suffix.
///
void TestEventsHistoryWidget::dateTimeFieldsFitZoneSuffix()
{
    EventsHistoryWidget widget;

    auto *startEdit = widget.findChild<QDateTimeEdit *>(QStringLiteral("eventsHistoryStartEdit"));
    auto *endEdit = widget.findChild<QDateTimeEdit *>(QStringLiteral("eventsHistoryEndEdit"));
    QVERIFY(startEdit);
    QVERIFY(endEdit);

    QVERIFY(startEdit->minimumWidth() >= 190);
    QVERIFY(endEdit->minimumWidth() >= 190);
}

///
/// \brief Clearing the node field clears the selected node and event rows.
///
void TestEventsHistoryWidget::nodeClearClearsResults()
{
    EventsHistoryWidget widget;
    widget.requestEventsHistoryForNode(QStringLiteral("ns=0;i=2253"), QStringLiteral("Server"));
    widget.setEventsHistoryResults({makeEvent()});

    auto *nodeEdit = widget.findChild<ValueLineEdit *>(QStringLiteral("eventsHistoryNodeEdit"));
    auto *table = widget.findChild<QTableView *>(QStringLiteral("eventsHistoryTable"));
    auto *readButton = widget.findChild<QToolButton *>(QStringLiteral("eventsHistoryReadButton"));
    QVERIFY(nodeEdit);
    QVERIFY(table);
    QVERIFY(readButton);
    QCOMPARE(table->model()->rowCount(), 1);
    QVERIFY(nodeEdit->actions().constFirst()->isVisible());

    QSignalSpy readSpy(&widget, &EventsHistoryWidget::eventsHistoryReadRequested);
    nodeEdit->actions().constFirst()->trigger();

    QCOMPARE(table->model()->rowCount(), 0);
    QVERIFY(nodeEdit->text().isEmpty());
    QVERIFY(nodeEdit->toolTip().isEmpty());
    QVERIFY(!nodeEdit->actions().constFirst()->isVisible());

    readButton->click();
    QCOMPARE(readSpy.size(), 0);
}

///
/// \brief The read button emits the selected node and current range.
///
void TestEventsHistoryWidget::readButtonEmitsQuery()
{
    EventsHistoryWidget widget;
    widget.requestEventsHistoryForNode(QStringLiteral("ns=0;i=2253"), QStringLiteral("Server"));

    QSignalSpy spy(&widget, &EventsHistoryWidget::eventsHistoryReadRequested);
    auto *button = widget.findChild<QToolButton *>(QStringLiteral("eventsHistoryReadButton"));
    QVERIFY(spy.isValid());
    QVERIFY(button);

    button->click();
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(0).toString(), QStringLiteral("ns=0;i=2253"));
}

///
/// \brief The Clear button uses the trash icon.
///
void TestEventsHistoryWidget::clearButtonUsesTrashIcon()
{
    EventsHistoryWidget widget;
    auto *button = widget.findChild<ThemedToolButton *>(QStringLiteral("eventsHistoryClearButton"));
    QVERIFY(button);
    QCOMPARE(button->iconName(), QStringLiteral("trash"));
}

QTEST_MAIN(TestEventsHistoryWidget)

#include "test_eventshistorywidget.moc"
