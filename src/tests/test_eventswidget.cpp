// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_eventswidget.cpp
/// \brief Tests EventsWidget toolbar behaviour.
///

#include <QDateTime>
#include <QSignalSpy>
#include <QTableView>
#include <QTest>
#include <QToolButton>

#include "widgets/eventswidget.h"
#include "widgets/themedtoolbutton.h"
#include "widgets/valuelineedit.h"

///
/// \brief UI tests for EventsWidget.
///
class TestEventsWidget : public QObject
{
    Q_OBJECT

private slots:
    void exportAndClearButtonsFollowEvents();
    void sourceClearUnsubscribesAndClearsEvents();
    void exportButtonUsesExportIcon();
    void exportFileNameUsesSource();
};

///
/// \brief Export and Clear are enabled only when there are rows.
///
void TestEventsWidget::exportAndClearButtonsFollowEvents()
{
    EventsWidget widget;
    auto *exportButton = widget.findChild<QToolButton *>(QStringLiteral("eventsExportButton"));
    auto *clearButton = widget.findChild<QToolButton *>(QStringLiteral("eventsClearButton"));
    QVERIFY(exportButton);
    QVERIFY(clearButton);
    QVERIFY(!exportButton->isEnabled());
    QVERIFY(!clearButton->isEnabled());

    OpcUaEvent event;
    event.time = QDateTime(QDate(2026, 6, 26), QTime(11, 5, 20, 335), Qt::UTC);
    event.severity = 500;
    event.sourceName = QStringLiteral("MyLevel");
    event.message = QStringLiteral("Level exceeded");
    event.eventType = QStringLiteral("ns=0;i=9482");
    widget.appendEvents({event});

    QVERIFY(exportButton->isEnabled());
    QVERIFY(clearButton->isEnabled());

    widget.clear();
    QVERIFY(!exportButton->isEnabled());
    QVERIFY(!clearButton->isEnabled());
}

///
/// \brief Clearing the event source field unsubscribes and clears event rows.
///
void TestEventsWidget::sourceClearUnsubscribesAndClearsEvents()
{
    EventsWidget widget;
    widget.setEventSource(QStringLiteral("ns=6;s=MyDevice"), QStringLiteral("MyDevice"));
    widget.setEventMonitoringState(QStringLiteral("ns=6;s=MyDevice"), true);

    OpcUaEvent event;
    event.time = QDateTime(QDate(2026, 6, 26), QTime(11, 38, 1, 329), Qt::UTC);
    event.severity = 700;
    event.sourceName = QStringLiteral("MyLevel");
    event.message = QStringLiteral("Level exceeded");
    event.eventType = QStringLiteral("ns=0;i=9482");
    widget.appendEvents({event});

    auto *nodeEdit = widget.findChild<ValueLineEdit *>(QStringLiteral("eventsNodeEdit"));
    auto *table = widget.findChild<QTableView *>(QStringLiteral("eventsTable"));
    auto *subscribeButton = widget.findChild<QToolButton *>(QStringLiteral("eventsSubscribeButton"));
    auto *unsubscribeButton = widget.findChild<QToolButton *>(QStringLiteral("eventsUnsubscribeButton"));
    QVERIFY(nodeEdit);
    QVERIFY(table);
    QVERIFY(subscribeButton);
    QVERIFY(unsubscribeButton);
    QCOMPARE(table->model()->rowCount(), 1);
    QVERIFY(unsubscribeButton->isEnabled());
    QCOMPARE(nodeEdit->actions().size(), 1);
    QVERIFY(nodeEdit->actions().constFirst()->isVisible());

    QSignalSpy unsubscribeSpy(&widget, &EventsWidget::eventUnsubscribeRequested);
    nodeEdit->actions().constFirst()->trigger();

    QCOMPARE(unsubscribeSpy.size(), 1);
    QCOMPARE(unsubscribeSpy.first().at(0).toString(), QStringLiteral("ns=6;s=MyDevice"));
    QCOMPARE(table->model()->rowCount(), 0);
    QVERIFY(nodeEdit->text().isEmpty());
    QVERIFY(nodeEdit->toolTip().isEmpty());
    QVERIFY(!subscribeButton->isEnabled());
    QVERIFY(!unsubscribeButton->isEnabled());
}

///
/// \brief The Events export button uses the export icon.
///
void TestEventsWidget::exportButtonUsesExportIcon()
{
    EventsWidget widget;
    auto *button = widget.findChild<ThemedToolButton *>(QStringLiteral("eventsExportButton"));
    QVERIFY(button);
    QCOMPARE(button->iconName(), QStringLiteral("export"));
}

///
/// \brief The suggested export file name includes the event source label.
///
void TestEventsWidget::exportFileNameUsesSource()
{
    EventsWidget widget;
    widget.setEventSource(QStringLiteral("ns=6;s=MyDevice"),
                          QStringLiteral("Area 1/MyDevice:Events"));

    QCOMPARE(widget.suggestedEventsCsvFileName(),
             QStringLiteral("events_Area_1_MyDevice_Events.csv"));
}

QTEST_MAIN(TestEventsWidget)

#include "test_eventswidget.moc"
