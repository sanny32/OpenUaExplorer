// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_eventswidget.cpp
/// \brief Tests EventsWidget toolbar behaviour.
///

#include <QDateTime>
#include <QTest>
#include <QToolButton>

#include "widgets/eventswidget.h"
#include "widgets/themedtoolbutton.h"

///
/// \brief UI tests for EventsWidget.
///
class TestEventsWidget : public QObject
{
    Q_OBJECT

private slots:
    void exportAndClearButtonsFollowEvents();
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
