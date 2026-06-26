// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_eventswidget.cpp
/// \brief Tests EventsWidget toolbar behaviour.
///

#include <QCoreApplication>
#include <QDateTime>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QScopedPointer>
#include <QSignalSpy>
#include <QTableView>
#include <QTest>
#include <QToolButton>

#include "models/addressspacemimedata.h"
#include "widgets/eventswidget.h"
#include "widgets/nodelineedit.h"
#include "widgets/themedtoolbutton.h"
#include "widgets/valuelineedit.h"

namespace {

///
/// \brief Builds an Object node eligible to be dropped as an event source.
/// \param nodeId Node's NodeId.
/// \param displayName Node's display name.
/// \return Node info item.
///
OpcUaNodeInfo makeEventSourceNode(const QString &nodeId, const QString &displayName)
{
    OpcUaNodeInfo node;
    node.nodeId = nodeId;
    node.browseName = displayName;
    node.displayName = displayName;
    node.nodeClass = OpcUa::Object;
    return node;
}

///
/// \brief Sends drag-enter and drop events carrying a node to a widget.
/// \param target Widget receiving the events.
/// \param node Node to drop.
/// \return Whether the drop event was accepted.
///
bool dropNodeOn(QWidget *target, const OpcUaNodeInfo &node)
{
    QScopedPointer<QMimeData> mimeData(AddressSpaceMime::createNodeMimeData(node));

    QDragEnterEvent enterEvent(QPoint(4, 4), Qt::CopyAction, mimeData.data(),
                               Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(target, &enterEvent);

    QDropEvent dropEvent(QPointF(4, 4), Qt::CopyAction, mimeData.data(),
                         Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(target, &dropEvent);
    return dropEvent.isAccepted();
}

} // namespace

///
/// \brief UI tests for EventsWidget.
///
class TestEventsWidget : public QObject
{
    Q_OBJECT

private slots:
    void exportAndClearButtonsFollowEvents();
    void sourceClearUnsubscribesAndClearsEvents();
    void dropReplacingMonitoredSourceUnsubscribesPrevious();
    void dropOfSameMonitoredSourceKeepsSubscription();
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
/// \brief Dropping a different node unsubscribes the previously monitored source.
///
void TestEventsWidget::dropReplacingMonitoredSourceUnsubscribesPrevious()
{
    EventsWidget widget;
    widget.setEventSource(QStringLiteral("ns=6;s=DeviceA"), QStringLiteral("DeviceA"));
    widget.setEventMonitoringState(QStringLiteral("ns=6;s=DeviceA"), true);

    auto *nodeEdit = widget.findChild<NodeLineEdit *>(QStringLiteral("eventsNodeEdit"));
    auto *subscribeButton = widget.findChild<QToolButton *>(QStringLiteral("eventsSubscribeButton"));
    auto *unsubscribeButton = widget.findChild<QToolButton *>(QStringLiteral("eventsUnsubscribeButton"));
    QVERIFY(nodeEdit);
    QVERIFY(subscribeButton);
    QVERIFY(unsubscribeButton);
    QVERIFY(unsubscribeButton->isEnabled());

    QSignalSpy unsubscribeSpy(&widget, &EventsWidget::eventUnsubscribeRequested);
    QVERIFY(dropNodeOn(nodeEdit, makeEventSourceNode(QStringLiteral("ns=6;s=DeviceB"),
                                                     QStringLiteral("DeviceB"))));

    QCOMPARE(unsubscribeSpy.size(), 1);
    QCOMPARE(unsubscribeSpy.first().at(0).toString(), QStringLiteral("ns=6;s=DeviceA"));
    QCOMPARE(nodeEdit->nodeId(), QStringLiteral("ns=6;s=DeviceB"));
    QVERIFY(subscribeButton->isEnabled());
    QVERIFY(!unsubscribeButton->isEnabled());
}

///
/// \brief Re-dropping the already-monitored node keeps its subscription intact.
///
void TestEventsWidget::dropOfSameMonitoredSourceKeepsSubscription()
{
    EventsWidget widget;
    widget.setEventSource(QStringLiteral("ns=6;s=DeviceA"), QStringLiteral("DeviceA"));
    widget.setEventMonitoringState(QStringLiteral("ns=6;s=DeviceA"), true);

    auto *nodeEdit = widget.findChild<NodeLineEdit *>(QStringLiteral("eventsNodeEdit"));
    auto *subscribeButton = widget.findChild<QToolButton *>(QStringLiteral("eventsSubscribeButton"));
    auto *unsubscribeButton = widget.findChild<QToolButton *>(QStringLiteral("eventsUnsubscribeButton"));
    QVERIFY(nodeEdit);
    QVERIFY(subscribeButton);
    QVERIFY(unsubscribeButton);

    QSignalSpy unsubscribeSpy(&widget, &EventsWidget::eventUnsubscribeRequested);
    QVERIFY(dropNodeOn(nodeEdit, makeEventSourceNode(QStringLiteral("ns=6;s=DeviceA"),
                                                     QStringLiteral("DeviceA"))));

    QCOMPARE(unsubscribeSpy.size(), 0);
    QVERIFY(!subscribeButton->isEnabled());
    QVERIFY(unsubscribeButton->isEnabled());
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
