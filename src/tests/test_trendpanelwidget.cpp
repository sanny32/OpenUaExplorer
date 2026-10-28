// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_trendpanelwidget.cpp
/// \brief Tests TrendPanelWidget mode switching and data routing.
///

#include <QAbstractButton>
#include <QApplication>
#include <QContextMenuEvent>
#include <QCoreApplication>
#include <QDateTime>
#include <QGraphicsEllipseItem>
#include <QLineEdit>
#include <QMenu>
#include <QSignalSpy>
#include <QTabBar>
#include <QTabWidget>
#include <QTest>
#include <QMouseEvent>
#include <QTimer>
#include <QWheelEvent>
#include <QtCharts/QAbstractAxis>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include "opcua/opcuatypes.h"
#include "suffixtranslator.h"
#include "widgets/trendgraphwidget.h"
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
/// \brief Opens a widget's context menu and returns its non-separator action texts.
/// \param widget Widget receiving the context-menu event.
/// \return Visible action texts in menu order.
///
QStringList contextMenuTexts(QWidget *widget)
{
    QStringList texts;
    QTimer::singleShot(0, qApp, [&texts]() {
        auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
        if (!menu)
            return;
        for (QAction *action : menu->actions()) {
            if (!action->isSeparator())
                texts.append(action->text());
        }
        menu->close();
    });

    const QPoint pos = widget->rect().center();
    QContextMenuEvent event(QContextMenuEvent::Mouse, pos, widget->mapToGlobal(pos));
    QCoreApplication::sendEvent(widget, &event);
    return texts;
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

///
/// \brief Shows a panel and double-clicks its first chart tab to start a rename.
/// \param panel Panel to set up; shown by this call.
/// \return The inline title editor, or nullptr when it did not open.
///
QLineEdit *startFirstTabRename(TrendPanelWidget *panel)
{
    panel->resize(800, 600);
    panel->show();
    if (!QTest::qWaitForWindowExposed(panel))
        return nullptr;

    auto *tabs = panel->findChild<QTabWidget *>(QStringLiteral("trendTabs"));
    if (!tabs)
        return nullptr;
    QTest::mouseDClick(tabs->tabBar(), Qt::LeftButton, Qt::NoModifier,
                       tabs->tabBar()->tabRect(0).center());

    auto *editor = panel->findChild<QLineEdit *>(QStringLiteral("tabNameEditor"));
    return editor && editor->isVisible() ? editor : nullptr;
}

///
/// \brief Returns the title of a panel's tab.
/// \param panel Panel holding the tabs.
/// \param index Tab index.
/// \return Tab title, or an empty string when the panel has no tab widget.
///
QString tabTitle(TrendPanelWidget *panel, int index)
{
    auto *tabs = panel->findChild<QTabWidget *>(QStringLiteral("trendTabs"));
    return tabs ? tabs->tabText(index) : QString();
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
    void fitContextActionFollowsMode();
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
    void languageChangeKeepsStateDependentTooltips();
    void doubleClickRenamesTab();
    void escapeCancelsTabRename();
    void blankTitleKeepsTabName();
    void contextMenuRenameOpensEditor();
    void addTabCannotBeRenamed();
    void f2RenamesActiveTab();
    void f2RenamesClickedTab();
    void tabTitlesSurviveCaptureAndRestore();
    void defaultTabTitlesFollowTheLanguage();
    void defaultTabTitlesAreNotSaved();
    void savedDefaultTitleFollowsTheLanguage();
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
/// \brief Fit is absent from the live context menu and offered for history.
///
void TestTrendPanelWidget::fitContextActionFollowsMode()
{
    TrendPanelWidget panel;
    panel.addNode(QString::fromLatin1(kNodeId), QStringLiteral("Demo"));
    auto *graph = panel.findChild<TrendGraphWidget *>();
    QVERIFY(graph);

    const QString fitText = QStringLiteral("Fit");
    QVERIFY(!contextMenuTexts(graph).contains(fitText));

    auto *oneMinute = panel.findChild<QAbstractButton *>(QStringLiteral("oneMinuteButton"));
    QVERIFY(oneMinute);
    oneMinute->click();
    QVERIFY(contextMenuTexts(graph).contains(fitText));
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

///
/// \brief Collapse and live-pause tooltips still name the next action after a language change.
///
void TestTrendPanelWidget::languageChangeKeepsStateDependentTooltips()
{
    TrendPanelWidget panel;
    panel.setCollapsed(true);
    auto *collapse = panel.findChild<QAbstractButton *>(QStringLiteral("collapseButton"));
    auto *livePause = panel.findChild<QAbstractButton *>(QStringLiteral("livePauseButton"));
    QVERIFY(collapse);
    QVERIFY(livePause);
    QVERIFY(livePause->isEnabled());
    livePause->click();
    QCOMPARE(livePause->toolTip(), QStringLiteral("Resume"));

    TestTranslation::SuffixTranslator translator;
    TestTranslation::switchLanguage(&translator, true);
    QCOMPARE(collapse->toolTip(), QStringLiteral("Expand trend panel-xx"));
    QCOMPARE(livePause->toolTip(), QStringLiteral("Resume-xx"));

    TestTranslation::switchLanguage(&translator, false);
    QCOMPARE(collapse->toolTip(), QStringLiteral("Expand trend panel"));
    QCOMPARE(livePause->toolTip(), QStringLiteral("Resume"));
}

///
/// \brief Double-clicking a chart tab edits its title in place; Enter applies it.
///
void TestTrendPanelWidget::doubleClickRenamesTab()
{
    TrendPanelWidget panel;
    QLineEdit *editor = startFirstTabRename(&panel);
    QVERIFY(editor);
    QCOMPARE(editor->text(), QStringLiteral("Trend 1"));

    editor->setText(QStringLiteral("  Pumps  "));
    QTest::keyClick(editor, Qt::Key_Return);

    QCOMPARE(tabTitle(&panel, 0), QStringLiteral("Pumps"));
    QVERIFY(!editor->isVisible());
}

///
/// \brief Escape closes the editor and leaves the title untouched.
///
void TestTrendPanelWidget::escapeCancelsTabRename()
{
    TrendPanelWidget panel;
    QLineEdit *editor = startFirstTabRename(&panel);
    QVERIFY(editor);

    editor->setText(QStringLiteral("Pumps"));
    QTest::keyClick(editor, Qt::Key_Escape);

    QVERIFY(!editor->isVisible());
    QCOMPARE(tabTitle(&panel, 0), QStringLiteral("Trend 1"));
}

///
/// \brief A blank title is rejected so a tab never ends up nameless.
///
void TestTrendPanelWidget::blankTitleKeepsTabName()
{
    TrendPanelWidget panel;
    QLineEdit *editor = startFirstTabRename(&panel);
    QVERIFY(editor);

    editor->setText(QStringLiteral("   "));
    QTest::keyClick(editor, Qt::Key_Return);

    QVERIFY(!editor->isVisible());
    QCOMPARE(tabTitle(&panel, 0), QStringLiteral("Trend 1"));
}

///
/// \brief The tab context menu offers Rename, which opens the inline editor.
///
void TestTrendPanelWidget::contextMenuRenameOpensEditor()
{
    TrendPanelWidget panel;
    panel.resize(800, 600);
    panel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&panel));

    auto *tabs = panel.findChild<QTabWidget *>(QStringLiteral("trendTabs"));
    QVERIFY(tabs);
    QTabBar *bar = tabs->tabBar();

    QStringList texts;
    QTimer::singleShot(0, qApp, [&texts]() {
        auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
        if (!menu)
            return;
        for (QAction *action : menu->actions()) {
            texts.append(action->text());
            if (action->text() == QStringLiteral("Rename"))
                action->trigger();
        }
        menu->close();
    });
    const QPoint pos = bar->tabRect(0).center();
    QContextMenuEvent event(QContextMenuEvent::Mouse, pos, bar->mapToGlobal(pos));
    QCoreApplication::sendEvent(bar, &event);

    QCOMPARE(texts, QStringList({QStringLiteral("Rename"), QStringLiteral("Close")}));
    auto *editor = panel.findChild<QLineEdit *>(QStringLiteral("tabNameEditor"));
    QVERIFY(editor);
    QVERIFY(editor->isVisible());
    QCOMPARE(editor->text(), QStringLiteral("Trend 1"));
}

///
/// \brief The trailing "+" tab is not a chart and offers no rename.
///
void TestTrendPanelWidget::addTabCannotBeRenamed()
{
    TrendPanelWidget panel;
    auto *tabs = panel.findChild<QTabWidget *>(QStringLiteral("trendTabs"));
    QVERIFY(tabs);

    emit tabs->tabBarDoubleClicked(tabs->count() - 1);

    auto *editor = panel.findChild<QLineEdit *>(QStringLiteral("tabNameEditor"));
    QVERIFY(editor);
    QVERIFY(editor->isHidden());
}

///
/// \brief F2 inside the panel renames the active tab and leaves an open edit alone.
///
void TestTrendPanelWidget::f2RenamesActiveTab()
{
    TrendPanelWidget panel;
    panel.addNode(QString::fromLatin1(kNodeId), QStringLiteral("Demo"));
    panel.resize(800, 600);
    panel.show();
    panel.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&panel));

    auto *graph = panel.findChild<TrendGraphWidget *>();
    QVERIFY(graph);
    graph->setFocus();
    QTest::keyClick(graph, Qt::Key_F2);

    auto *editor = panel.findChild<QLineEdit *>(QStringLiteral("tabNameEditor"));
    QVERIFY(editor);
    QVERIFY(editor->isVisible());
    QCOMPARE(editor->text(), QStringLiteral("Trend 1"));

    editor->setText(QStringLiteral("Pumps"));
    QTest::keyClick(editor, Qt::Key_F2);
    QCOMPARE(editor->text(), QStringLiteral("Pumps"));

    QTest::keyClick(editor, Qt::Key_Return);
    QCOMPARE(tabTitle(&panel, 0), QStringLiteral("Pumps"));
}

///
/// \brief Clicking a tab focuses the tab bar, so F2 right after it renames that tab.
///
void TestTrendPanelWidget::f2RenamesClickedTab()
{
    TrendPanelWidget panel;
    panel.resize(800, 600);
    panel.show();
    panel.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&panel));

    auto *tabs = panel.findChild<QTabWidget *>(QStringLiteral("trendTabs"));
    QVERIFY(tabs);
    QTabBar *bar = tabs->tabBar();
    QTest::mouseClick(bar, Qt::LeftButton, Qt::NoModifier, bar->tabRect(0).center());
    QCOMPARE(QApplication::focusWidget(), bar);

    QTest::keyClick(bar, Qt::Key_F2);

    auto *editor = panel.findChild<QLineEdit *>(QStringLiteral("tabNameEditor"));
    QVERIFY(editor);
    QVERIFY(editor->isVisible());
    QCOMPARE(editor->text(), QStringLiteral("Trend 1"));
}

///
/// \brief Renamed titles are captured for the session and reapplied on restore.
///
void TestTrendPanelWidget::tabTitlesSurviveCaptureAndRestore()
{
    TrendPanelWidget panel;
    QLineEdit *editor = startFirstTabRename(&panel);
    QVERIFY(editor);
    editor->setText(QStringLiteral("Pumps"));
    QTest::keyClick(editor, Qt::Key_Return);

    const QVector<SessionTrendTab> captured = panel.captureTrendTabs();
    QCOMPARE(captured.size(), 1);
    QCOMPARE(captured.first().title, QStringLiteral("Pumps"));

    SessionTrendTab untitled;
    TrendPanelWidget restored;
    restored.restoreTrendTabs({captured.first(), untitled});
    QCOMPARE(tabTitle(&restored, 0), QStringLiteral("Pumps"));
    QCOMPARE(tabTitle(&restored, 1), QStringLiteral("Trend 2"));
}

///
/// \brief Tabs that keep their default title are retitled on a language change; renamed ones stay.
///
void TestTrendPanelWidget::defaultTabTitlesFollowTheLanguage()
{
    TrendPanelWidget panel;
    QLineEdit *editor = startFirstTabRename(&panel);
    QVERIFY(editor);
    editor->setText(QStringLiteral("Pumps"));
    QTest::keyClick(editor, Qt::Key_Return);
    auto *tabs = panel.findChild<QTabWidget *>(QStringLiteral("trendTabs"));
    QVERIFY(tabs);
    tabs->setCurrentIndex(tabs->count() - 1);
    QCOMPARE(tabTitle(&panel, 1), QStringLiteral("Trend 2"));

    TestTranslation::SuffixTranslator translator({"TrendPanelWidget"});
    TestTranslation::switchLanguage(&translator, true);
    const QString renamed = tabTitle(&panel, 0);
    const QString translated = tabTitle(&panel, 1);
    TestTranslation::switchLanguage(&translator, false);

    QCOMPARE(renamed, QStringLiteral("Pumps"));
    QCOMPARE(translated, QStringLiteral("Trend 2-xx"));
    QCOMPARE(tabTitle(&panel, 1), QStringLiteral("Trend 2"));
}

///
/// \brief A default title is left out of the session so it reopens in the current language.
///
void TestTrendPanelWidget::defaultTabTitlesAreNotSaved()
{
    TrendPanelWidget panel;
    const QVector<SessionTrendTab> captured = panel.captureTrendTabs();
    QCOMPARE(captured.size(), 1);
    QVERIFY(captured.first().title.isEmpty());
}

///
/// \brief A default title saved by an older session is not mistaken for a rename.
///
void TestTrendPanelWidget::savedDefaultTitleFollowsTheLanguage()
{
    SessionTrendTab saved;
    saved.title = QStringLiteral("Trend 1");
    TrendPanelWidget panel;
    panel.restoreTrendTabs({saved});
    QVERIFY(panel.captureTrendTabs().first().title.isEmpty());

    TestTranslation::SuffixTranslator translator({"TrendPanelWidget"});
    TestTranslation::switchLanguage(&translator, true);
    const QString translated = tabTitle(&panel, 0);
    TestTranslation::switchLanguage(&translator, false);

    QCOMPARE(translated, QStringLiteral("Trend 1-xx"));
}

QTEST_MAIN(TestTrendPanelWidget)

#include "test_trendpanelwidget.moc"
