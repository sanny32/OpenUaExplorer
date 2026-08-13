// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_trendpanelwidget.cpp
/// \brief Tests TrendPanelWidget mode switching and data routing.
///

#include <QAbstractButton>
#include <QCoreApplication>
#include <QDateTime>
#include <QGraphicsEllipseItem>
#include <QSignalSpy>
#include <QTest>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QtCharts/QAbstractAxis>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include "opcua/opcuatypes.h"
#include "widgets/trendpanelwidget.h"

namespace {

constexpr auto kNodeId = "ns=2;s=Demo";

///
/// \brief Rotates the wheel over a chart view as the window manager would.
/// \param view Chart view receiving the event.
/// \param notches Wheel notches; positive zooms in.
/// \param modifiers Keyboard modifiers held during the rotation.
///
void sendWheel(QChartView *view, int notches, Qt::KeyboardModifiers modifiers = Qt::NoModifier)
{
    const QPointF center(view->viewport()->rect().center());
    QWheelEvent event(center, view->viewport()->mapToGlobal(center.toPoint()), QPoint(),
                      QPoint(0, notches * 120), Qt::NoButton, modifiers,
                      Qt::NoScrollPhase, false);
    QCoreApplication::sendEvent(view->viewport(), &event);
}

///
/// \brief Builds two history samples far outside the default value axis.
/// \return Samples a value auto-scale would visibly react to.
///
///
/// \brief Builds history samples that all carry the same value.
/// \param count Number of samples spread over the last minute.
/// \return Samples drawing a horizontal line across the window.
///
QVector<OpcUaHistoryValue> flatHistorySamples(int count)
{
    const QDateTime now = QDateTime::currentDateTime();
    QVector<OpcUaHistoryValue> values;
    values.reserve(count);
    for (int i = 0; i < count; ++i) {
        OpcUaHistoryValue value;
        value.nodeId = QString::fromLatin1(kNodeId);
        value.value = 42.0;
        value.status = QStringLiteral("Good");
        value.sourceTimestamp = now.addSecs(i * 10 - 50);
        values.append(value);
    }
    return values;
}

QVector<OpcUaHistoryValue> historySamples()
{
    const QDateTime now = QDateTime::currentDateTime();
    QVector<OpcUaHistoryValue> values;
    for (int i = 0; i < 2; ++i) {
        OpcUaHistoryValue value;
        value.nodeId = QString::fromLatin1(kNodeId);
        value.value = 100.0 * (i + 1);
        value.status = QStringLiteral("Good");
        value.sourceTimestamp = now.addSecs(i - 2);
        values.append(value);
    }
    return values;
}

///
/// \brief Maps a sample to a position in the chart view's viewport.
/// \param view Chart view holding the series.
/// \param series Series the sample belongs to.
/// \param sample Sample in axis coordinates.
/// \return Sample position in viewport pixels.
///
QPoint viewportPos(QChartView *view, QLineSeries *series, const QPointF &sample)
{
    const QPointF itemPos = view->chart()->mapToPosition(sample, series);
    return view->mapFromScene(view->chart()->mapToScene(itemPos));
}

///
/// \brief Finds the floating value plaque owned by a chart view.
/// \param view Chart view to search.
/// \return The plaque window, or nullptr when the chart has none.
///
/// The plaque is a private backend widget with no exported type; it is the chart
/// view's only tool-tip window, which identifies it well enough for a test.
///
QWidget *valuePlaque(QChartView *view)
{
    const QList<QWidget *> children = view->findChildren<QWidget *>();
    for (QWidget *child : children) {
        if (child->windowFlags().testFlag(Qt::ToolTip))
            return child;
    }
    return nullptr;
}

///
/// \brief Finds the dot the chart draws at the hovered position.
/// \param view Chart view to search.
/// \return The marker item, or nullptr when the chart has none.
///
QGraphicsEllipseItem *hoverMarker(QChartView *view)
{
    const QList<QGraphicsItem *> items = view->chart()->childItems();
    for (QGraphicsItem *item : items) {
        if (auto *ellipse = qgraphicsitem_cast<QGraphicsEllipseItem *>(item))
            return ellipse;
    }
    return nullptr;
}

///
/// \brief Waits until the chart has been laid out and can map pixels to values.
/// \param view Chart view to wait for.
/// \return True once the plot area has a usable size.
///
bool waitForPlotArea(QChartView *view)
{
    return QTest::qWaitFor([view]() { return !view->chart()->plotArea().isEmpty(); }, 2000);
}

///
/// \brief Posts one mouse event to a widget.
/// \param viewport Widget receiving the event.
/// \param type Mouse event type.
/// \param pos Position in the widget's coordinates.
/// \param button Button that changed state.
/// \param buttons Buttons held during the event.
///
/// Each event is built right before it is sent: several QMouseEvents alive at once
/// share the pointing device's point state, which would give them all one position.
///
void sendMouse(QWidget *viewport, QEvent::Type type, const QPoint &pos,
               Qt::MouseButton button, Qt::MouseButtons buttons)
{
    QMouseEvent event(type, QPointF(pos), QPointF(viewport->mapToGlobal(pos)),
                      button, buttons, Qt::NoModifier);
    QCoreApplication::sendEvent(viewport, &event);
}

///
/// \brief Shows a panel charting one flat history series, ready for hover tests.
/// \param panel Panel to set up; shown and sized by this call.
/// \param view Receives the panel's chart view.
/// \param series Receives the charted series.
/// \return True once the chart is laid out and holds the samples.
///
bool showFlatSeries(TrendPanelWidget *panel, QChartView **view, QLineSeries **series)
{
    panel->resize(800, 600);
    panel->show();
    if (!QTest::qWaitForWindowExposed(panel))
        return false;

    panel->addNode(QString::fromLatin1(kNodeId), QStringLiteral("Demo"));
    auto *oneMinute = panel->findChild<QAbstractButton *>(QStringLiteral("oneMinuteButton"));
    if (!oneMinute)
        return false;
    oneMinute->click();
    if (!panel->consumeHistory(QString::fromLatin1(kNodeId), QString(), flatHistorySamples(6)))
        return false;

    *view = panel->findChild<QChartView *>();
    if (!*view || !waitForPlotArea(*view) || (*view)->chart()->series().isEmpty())
        return false;

    *series = qobject_cast<QLineSeries *>((*view)->chart()->series().constFirst());
    return *series && (*series)->count() > 1;
}

///
/// \brief Drags the mouse across a chart view as a user panning it would.
/// \param view Chart view receiving the events.
/// \param pixels Displacement applied in one move, in viewport pixels.
///
void dragChart(QChartView *view, const QPoint &pixels)
{
    QWidget *viewport = view->viewport();
    const QPoint start = viewport->rect().center();
    const QPoint end = start + pixels;

    sendMouse(viewport, QEvent::MouseButtonPress, start, Qt::LeftButton, Qt::LeftButton);
    sendMouse(viewport, QEvent::MouseMove, end, Qt::NoButton, Qt::LeftButton);
    sendMouse(viewport, QEvent::MouseButtonRelease, end, Qt::LeftButton, Qt::NoButton);
}

///
/// \brief Returns the horizontal (time) axis of a chart.
/// \param view Chart view to inspect.
/// \return Time axis, or nullptr when the chart has none.
///
QDateTimeAxis *timeAxis(QChartView *view)
{
    const QList<QAbstractAxis *> axes = view->chart()->axes(Qt::Horizontal);
    return axes.isEmpty() ? nullptr : qobject_cast<QDateTimeAxis *>(axes.constFirst());
}

///
/// \brief Returns the visible time span of a chart in milliseconds.
/// \param view Chart view to inspect.
/// \return Length of the horizontal axis range.
///
qint64 timeSpanMs(QChartView *view)
{
    QDateTimeAxis *axis = timeAxis(view);
    return axis ? axis->min().msecsTo(axis->max()) : -1;
}

} // namespace

///
/// \brief Verifies subscribe/history requests and history routing.
///
class TestTrendPanelWidget : public QObject
{
    Q_OBJECT

private slots:
    void addingNodeInLiveModeSubscribes();
    void switchingToHistoryModeReadsHistory();
    void refreshingHistoryReanchorsWindow();
    void consumeHistoryMatchesPendingNode();
    void unchangedLiveValueExtendsToNow();
    void wheelLeavesLiveChartUntouched();
    void fitIsHiddenWhileLive();
    void wheelZoomsHistoryTimeWindow();
    void controlWheelZoomsValueAxis();
    void wheelInHistoryModeRereadsZoomedRange();
    void draggingHistoryChartMovesWindow();
    void draggingShowsClosedHandCursor();
    void liveChartKeepsIdleCursor();
    void draggingLiveChartKeepsWindow();
    void hoverShowsPlaqueAlongLine();
    void hoverTracksPositionBetweenSamples();
    void hoverAlongHorizontalLineKeepsPlaque();
    void livePlaqueSurvivesWindowScrolling();
};

///
/// \brief Adding a node while live requests a subscription.
///
void TestTrendPanelWidget::addingNodeInLiveModeSubscribes()
{
    TrendPanelWidget panel;
    QSignalSpy spy(&panel, &TrendPanelWidget::subscribeRequested);

    panel.addNode(QString::fromLatin1(kNodeId), QStringLiteral("Demo"));

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(0).toString(), QString::fromLatin1(kNodeId));
}

///
/// \brief Choosing a range button unsubscribes and reads history.
///
void TestTrendPanelWidget::switchingToHistoryModeReadsHistory()
{
    TrendPanelWidget panel;
    panel.addNode(QString::fromLatin1(kNodeId), QStringLiteral("Demo"));

    QSignalSpy unsubscribeSpy(&panel, &TrendPanelWidget::unsubscribeRequested);
    QSignalSpy historySpy(&panel, &TrendPanelWidget::historyReadRequested);

    auto *oneMinute = panel.findChild<QAbstractButton *>(QStringLiteral("oneMinuteButton"));
    QVERIFY(oneMinute);
    oneMinute->click();

    QCOMPARE(unsubscribeSpy.count(), 1);
    QCOMPARE(historySpy.count(), 1);
    QCOMPARE(historySpy.first().at(0).toString(), QString::fromLatin1(kNodeId));
}

///
/// \brief Refreshing history keeps the range length and advances the window anchor.
///
void TestTrendPanelWidget::refreshingHistoryReanchorsWindow()
{
    TrendPanelWidget panel;
    panel.addNode(QString::fromLatin1(kNodeId), QStringLiteral("Demo"));

    QSignalSpy historySpy(&panel, &TrendPanelWidget::historyReadRequested);

    auto *oneMinute = panel.findChild<QAbstractButton *>(QStringLiteral("oneMinuteButton"));
    QVERIFY(oneMinute);
    oneMinute->click();
    QCOMPARE(historySpy.count(), 1);

    const QDateTime firstStart = historySpy.first().at(1).toDateTime();
    const QDateTime firstEnd = historySpy.first().at(2).toDateTime();
    QCOMPARE(firstStart.msecsTo(firstEnd), qint64(60000));

    QTest::qWait(20);
    auto *refresh = panel.findChild<QAbstractButton *>(QStringLiteral("refreshButton"));
    QVERIFY(refresh);
    refresh->click();

    QCOMPARE(historySpy.count(), 2);
    const QDateTime secondStart = historySpy.at(1).at(1).toDateTime();
    const QDateTime secondEnd = historySpy.at(1).at(2).toDateTime();
    QCOMPARE(secondStart.msecsTo(secondEnd), qint64(60000));
    QVERIFY(secondEnd >= firstEnd);
}

///
/// \brief consumeHistory claims pending nodes and ignores others.
///
void TestTrendPanelWidget::consumeHistoryMatchesPendingNode()
{
    TrendPanelWidget panel;
    panel.addNode(QString::fromLatin1(kNodeId), QStringLiteral("Demo"));

    auto *oneMinute = panel.findChild<QAbstractButton *>(QStringLiteral("oneMinuteButton"));
    QVERIFY(oneMinute);
    oneMinute->click();

    QVector<OpcUaHistoryValue> values;
    QVERIFY(!panel.consumeHistory(QStringLiteral("ns=2;s=Other"), QString(), values));
    QVERIFY(panel.consumeHistory(QString::fromLatin1(kNodeId), QString(), values));
    QVERIFY(!panel.consumeHistory(QString::fromLatin1(kNodeId), QString(), values));
}

///
/// \brief An unchanged live value stays visible by moving one trailing point to now.
///
void TestTrendPanelWidget::unchangedLiveValueExtendsToNow()
{
    TrendPanelWidget panel;
    SubscriptionItem subscription;
    subscription.name = QStringLiteral("Default");
    subscription.publishingInterval = 50.0;
    panel.setSubscriptions({subscription});
    panel.addNode(QString::fromLatin1(kNodeId), QStringLiteral("Demo"));

    OpcUaDataValue value;
    value.nodeId = QString::fromLatin1(kNodeId);
    value.value = 7;
    value.sourceTimestamp = QDateTime::currentDateTime().addSecs(-1);
    value.status = QStringLiteral("Good");
    panel.applyLiveValues({value});

    auto *chartView = panel.findChild<QChartView *>();
    QVERIFY(chartView);
    QCOMPARE(chartView->chart()->series().size(), 1);
    auto *series = qobject_cast<QLineSeries *>(chartView->chart()->series().constFirst());
    QVERIFY(series);
    QVERIFY(series->count() >= 2);
    const int pointCount = series->count();
    const qreal firstEnd = series->at(pointCount - 1).x();
    QCOMPARE(series->at(pointCount - 1).y(), 7.0);

    QTRY_VERIFY_WITH_TIMEOUT(series->at(series->count() - 1).x() > firstEnd, 500);
    QCOMPARE(series->count(), pointCount);
    QCOMPARE(series->at(series->count() - 1).y(), 7.0);
}

///
/// \brief A live chart owns its viewport: the wheel leaves both axes alone.
///
void TestTrendPanelWidget::wheelLeavesLiveChartUntouched()
{
    TrendPanelWidget panel;
    panel.addNode(QString::fromLatin1(kNodeId), QStringLiteral("Demo"));

    auto *chartView = panel.findChild<QChartView *>();
    QVERIFY(chartView);
    auto *axisY = qobject_cast<QValueAxis *>(chartView->chart()->axes(Qt::Vertical).constFirst());
    QVERIFY(axisY);
    const qreal valueSpanBefore = axisY->max() - axisY->min();

    sendWheel(chartView, 1);
    QCOMPARE(timeSpanMs(chartView), qint64(60000));

    sendWheel(chartView, 1, Qt::ControlModifier);
    QCOMPARE(axisY->max() - axisY->min(), valueSpanBefore);
}

///
/// \brief Fit is shown for a historical range only, never while streaming.
///
void TestTrendPanelWidget::fitIsHiddenWhileLive()
{
    TrendPanelWidget panel;
    panel.addNode(QString::fromLatin1(kNodeId), QStringLiteral("Demo"));

    auto *fit = panel.findChild<QAbstractButton *>(QStringLiteral("fitButton"));
    QVERIFY(fit);
    QVERIFY(fit->isHidden());

    auto *oneMinute = panel.findChild<QAbstractButton *>(QStringLiteral("oneMinuteButton"));
    QVERIFY(oneMinute);
    oneMinute->click();
    QVERIFY(!fit->isHidden());

    auto *live = panel.findChild<QAbstractButton *>(QStringLiteral("liveButton"));
    QVERIFY(live);
    live->click();
    QVERIFY(fit->isHidden());
}

///
/// \brief Wheel rotation over a history chart shortens and lengthens the window.
///
void TestTrendPanelWidget::wheelZoomsHistoryTimeWindow()
{
    TrendPanelWidget panel;
    panel.addNode(QString::fromLatin1(kNodeId), QStringLiteral("Demo"));

    auto *oneMinute = panel.findChild<QAbstractButton *>(QStringLiteral("oneMinuteButton"));
    QVERIFY(oneMinute);
    oneMinute->click();

    auto *chartView = panel.findChild<QChartView *>();
    QVERIFY(chartView);
    QCOMPARE(timeSpanMs(chartView), qint64(60000));

    sendWheel(chartView, 1);
    QCOMPARE(timeSpanMs(chartView), qint64(48000));

    sendWheel(chartView, -1);
    QCOMPARE(timeSpanMs(chartView), qint64(60000));

    for (int i = 0; i < 40; ++i)
        sendWheel(chartView, 1);
    QCOMPARE(timeSpanMs(chartView), qint64(1000));
}

///
/// \brief Ctrl+wheel scales the value axis instead of the time axis.
///
void TestTrendPanelWidget::controlWheelZoomsValueAxis()
{
    TrendPanelWidget panel;
    panel.addNode(QString::fromLatin1(kNodeId), QStringLiteral("Demo"));

    auto *oneMinute = panel.findChild<QAbstractButton *>(QStringLiteral("oneMinuteButton"));
    QVERIFY(oneMinute);
    oneMinute->click();

    auto *chartView = panel.findChild<QChartView *>();
    QVERIFY(chartView);
    auto *axisY = qobject_cast<QValueAxis *>(chartView->chart()->axes(Qt::Vertical).constFirst());
    QVERIFY(axisY);

    const qint64 spanBefore = timeSpanMs(chartView);
    const qreal valueSpanBefore = axisY->max() - axisY->min();

    sendWheel(chartView, 1, Qt::ControlModifier);

    QCOMPARE(timeSpanMs(chartView), spanBefore);
    QVERIFY(axisY->max() - axisY->min() < valueSpanBefore);

    const qreal zoomedSpan = axisY->max() - axisY->min();
    QVERIFY(panel.consumeHistory(QString::fromLatin1(kNodeId), QString(), historySamples()));
    QCOMPARE(axisY->max() - axisY->min(), zoomedSpan);

    auto *autoScale = panel.findChild<QAbstractButton *>(QStringLiteral("autoScaleButton"));
    QVERIFY(autoScale);
    autoScale->click();
    QVERIFY(axisY->min() < 100.0);
    QVERIFY(axisY->max() > 200.0);
}

///
/// \brief Zooming a history window re-reads the range once the wheel settles.
///
void TestTrendPanelWidget::wheelInHistoryModeRereadsZoomedRange()
{
    TrendPanelWidget panel;
    panel.addNode(QString::fromLatin1(kNodeId), QStringLiteral("Demo"));

    auto *oneMinute = panel.findChild<QAbstractButton *>(QStringLiteral("oneMinuteButton"));
    QVERIFY(oneMinute);
    oneMinute->click();

    auto *chartView = panel.findChild<QChartView *>();
    QVERIFY(chartView);

    QSignalSpy historySpy(&panel, &TrendPanelWidget::historyReadRequested);
    sendWheel(chartView, -1);
    QCOMPARE(timeSpanMs(chartView), qint64(75000));
    QCOMPARE(historySpy.count(), 0);

    QTRY_VERIFY_WITH_TIMEOUT(historySpy.count() == 1, 2000);
    const QDateTime start = historySpy.first().at(1).toDateTime();
    const QDateTime end = historySpy.first().at(2).toDateTime();
    QCOMPARE(start.msecsTo(end), qint64(75000));
}

///
/// \brief Dragging a history chart moves the interval back in time and re-reads it.
///
void TestTrendPanelWidget::draggingHistoryChartMovesWindow()
{
    TrendPanelWidget panel;
    panel.resize(800, 600);
    panel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&panel));
    panel.addNode(QString::fromLatin1(kNodeId), QStringLiteral("Demo"));

    auto *oneMinute = panel.findChild<QAbstractButton *>(QStringLiteral("oneMinuteButton"));
    QVERIFY(oneMinute);
    oneMinute->click();

    auto *chartView = panel.findChild<QChartView *>();
    QVERIFY(chartView);
    QVERIFY(waitForPlotArea(chartView));
    const QDateTime endBefore = timeAxis(chartView)->max();

    QSignalSpy historySpy(&panel, &TrendPanelWidget::historyReadRequested);
    dragChart(chartView, QPoint(200, 0));

    QCOMPARE(timeSpanMs(chartView), qint64(60000));
    QVERIFY(timeAxis(chartView)->max() < endBefore);

    auto *custom = panel.findChild<QAbstractButton *>(QStringLiteral("customButton"));
    QVERIFY(custom);
    QVERIFY(custom->isChecked());

    QTRY_VERIFY_WITH_TIMEOUT(historySpy.count() == 1, 2000);
    const QDateTime start = historySpy.first().at(1).toDateTime();
    const QDateTime end = historySpy.first().at(2).toDateTime();
    QCOMPARE(start.msecsTo(end), qint64(60000));
    QVERIFY(end < endBefore);
}

///
/// \brief The cursor becomes a closed hand for as long as the button is held.
///
void TestTrendPanelWidget::draggingShowsClosedHandCursor()
{
    TrendPanelWidget panel;
    panel.resize(800, 600);
    panel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&panel));
    panel.addNode(QString::fromLatin1(kNodeId), QStringLiteral("Demo"));

    auto *oneMinute = panel.findChild<QAbstractButton *>(QStringLiteral("oneMinuteButton"));
    QVERIFY(oneMinute);
    oneMinute->click();

    auto *chartView = panel.findChild<QChartView *>();
    QVERIFY(chartView);
    QVERIFY(waitForPlotArea(chartView));

    QWidget *viewport = chartView->viewport();
    const Qt::CursorShape idle = viewport->cursor().shape();

    const QPoint start = viewport->rect().center();
    sendMouse(viewport, QEvent::MouseButtonPress, start, Qt::LeftButton, Qt::LeftButton);
    QCOMPARE(viewport->cursor().shape(), Qt::ClosedHandCursor);

    sendMouse(viewport, QEvent::MouseMove, start + QPoint(40, 0), Qt::NoButton, Qt::LeftButton);
    QCOMPARE(viewport->cursor().shape(), Qt::ClosedHandCursor);

    sendMouse(viewport, QEvent::MouseButtonRelease, start + QPoint(40, 0), Qt::LeftButton,
              Qt::NoButton);
    QCOMPARE(viewport->cursor().shape(), idle);
}

///
/// \brief A live chart is not pannable, so the cursor keeps its idle shape.
///
void TestTrendPanelWidget::liveChartKeepsIdleCursor()
{
    TrendPanelWidget panel;
    panel.resize(800, 600);
    panel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&panel));
    panel.addNode(QString::fromLatin1(kNodeId), QStringLiteral("Demo"));

    auto *chartView = panel.findChild<QChartView *>();
    QVERIFY(chartView);
    QVERIFY(waitForPlotArea(chartView));

    QWidget *viewport = chartView->viewport();
    const Qt::CursorShape idle = viewport->cursor().shape();
    QVERIFY(idle != Qt::ClosedHandCursor);

    sendMouse(viewport, QEvent::MouseButtonPress, viewport->rect().center(), Qt::LeftButton,
              Qt::LeftButton);
    QCOMPARE(viewport->cursor().shape(), idle);
}

///
/// \brief Dragging a live chart leaves its rolling window alone.
///
void TestTrendPanelWidget::draggingLiveChartKeepsWindow()
{
    TrendPanelWidget panel;
    panel.resize(800, 600);
    panel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&panel));
    panel.addNode(QString::fromLatin1(kNodeId), QStringLiteral("Demo"));

    auto *chartView = panel.findChild<QChartView *>();
    QVERIFY(chartView);
    QVERIFY(waitForPlotArea(chartView));
    const QDateTime endBefore = timeAxis(chartView)->max();

    dragChart(chartView, QPoint(200, 0));

    QCOMPARE(timeSpanMs(chartView), qint64(60000));
    QCOMPARE(timeAxis(chartView)->max(), endBefore);
}

///
/// \brief The plaque appears in the band along a line and hides away from it.
///
void TestTrendPanelWidget::hoverShowsPlaqueAlongLine()
{
    TrendPanelWidget panel;
    QChartView *chartView = nullptr;
    QLineSeries *series = nullptr;
    QVERIFY(showFlatSeries(&panel, &chartView, &series));

    QWidget *plaque = valuePlaque(chartView);
    QVERIFY(plaque);
    QVERIFY(!plaque->isVisible());

    const QPoint onLine = viewportPos(chartView, series, series->at(0));
    sendMouse(chartView->viewport(), QEvent::MouseMove, onLine + QPoint(0, 4), Qt::NoButton,
              Qt::NoButton);
    QVERIFY(plaque->isVisible());

    sendMouse(chartView->viewport(), QEvent::MouseMove, onLine + QPoint(0, 120), Qt::NoButton,
              Qt::NoButton);
    QTRY_VERIFY_WITH_TIMEOUT(!plaque->isVisible(), 2000);
}

///
/// \brief The marker rides the line under the cursor instead of jumping between samples.
///
void TestTrendPanelWidget::hoverTracksPositionBetweenSamples()
{
    TrendPanelWidget panel;
    QChartView *chartView = nullptr;
    QLineSeries *series = nullptr;
    QVERIFY(showFlatSeries(&panel, &chartView, &series));

    QGraphicsEllipseItem *marker = hoverMarker(chartView);
    QVERIFY(marker);

    const QPoint first = viewportPos(chartView, series, series->at(0));
    const QPoint second = viewportPos(chartView, series, series->at(1));
    QVERIFY(second.x() - first.x() > 20);

    for (const int offset : { 4, 10, 16 }) {
        const QPoint cursor(first.x() + offset, first.y());
        sendMouse(chartView->viewport(), QEvent::MouseMove, cursor, Qt::NoButton, Qt::NoButton);

        const QPoint markerPos =
            chartView->mapFromScene(chartView->chart()->mapToScene(marker->pos()));
        QVERIFY2(qAbs(markerPos.x() - cursor.x()) <= 1,
                 qPrintable(QStringLiteral("marker at %1 for cursor at %2")
                                .arg(markerPos.x())
                                .arg(cursor.x())));
    }
}

///
/// \brief Travelling along a horizontal line never blinks the plaque away.
///
/// A line's own hover area is a couple of pixels wide, so following it used to emit
/// leave and enter in bursts; the plaque must stay up across the whole sweep.
///
void TestTrendPanelWidget::hoverAlongHorizontalLineKeepsPlaque()
{
    TrendPanelWidget panel;
    QChartView *chartView = nullptr;
    QLineSeries *series = nullptr;
    QVERIFY(showFlatSeries(&panel, &chartView, &series));

    QWidget *plaque = valuePlaque(chartView);
    QVERIFY(plaque);

    const QPoint first = viewportPos(chartView, series, series->at(0));
    const QPoint last = viewportPos(chartView, series, series->at(series->count() - 1));
    QVERIFY(last.x() - first.x() > 20);

    for (int x = first.x(); x <= last.x(); x += 3) {
        const int jitter = (x / 3) % 2 == 0 ? -3 : 3;
        sendMouse(chartView->viewport(), QEvent::MouseMove, QPoint(x, first.y() + jitter),
                  Qt::NoButton, Qt::NoButton);
        QVERIFY2(plaque->isVisible(),
                 qPrintable(QStringLiteral("plaque hidden at x=%1").arg(x)));
    }
}

///
/// \brief A live chart scrolling under the cursor keeps the plaque on its sample.
///
/// The rolling window is re-applied on every live tick, which must re-anchor the
/// plaque instead of dropping it: the cursor does not move, so nothing would bring
/// it back until the user jiggles the mouse.
///
void TestTrendPanelWidget::livePlaqueSurvivesWindowScrolling()
{
    TrendPanelWidget panel;
    panel.resize(800, 600);
    panel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&panel));
    panel.addNode(QString::fromLatin1(kNodeId), QStringLiteral("Demo"));

    OpcUaDataValue value;
    value.nodeId = QString::fromLatin1(kNodeId);
    value.value = 0.5;
    value.sourceTimestamp = QDateTime::currentDateTime().addSecs(-20);
    value.status = QStringLiteral("Good");
    panel.applyLiveValues({value});

    auto *chartView = panel.findChild<QChartView *>();
    QVERIFY(chartView);
    QVERIFY(waitForPlotArea(chartView));
    auto *series = qobject_cast<QLineSeries *>(chartView->chart()->series().constFirst());
    QVERIFY(series);
    QVERIFY(series->count() > 0);

    QWidget *plaque = valuePlaque(chartView);
    QVERIFY(plaque);

    sendMouse(chartView->viewport(), QEvent::MouseMove,
              viewportPos(chartView, series, series->at(0)), Qt::NoButton, Qt::NoButton);
    QVERIFY(plaque->isVisible());

    QTest::qWait(1500);
    QVERIFY(plaque->isVisible());
}

QTEST_MAIN(TestTrendPanelWidget)

#include "test_trendpanelwidget.moc"
