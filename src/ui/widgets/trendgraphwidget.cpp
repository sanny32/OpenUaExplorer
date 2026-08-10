// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file trendgraphwidget.cpp
/// \brief Implements the trend graph widget.
///

#include "trendgraphwidget.h"
#include "ui_trendgraphwidget.h"

#include <algorithm>
#include <utility>

#include <QAction>
#include <QContextMenuEvent>
#include <QDateTime>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QIcon>
#include <QImage>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPointF>
#include <QTimer>
#include <QWheelEvent>

#include "appcolors.h"
#include "appicons.h"
#include "charttypes.h"
#include "chartviewfactory.h"
#include "chartzoom.h"
#include "dialogs/customintervaldialog.h"
#include "dialogs/messageboxdialog.h"
#include "dialogs/trendsettingsdialog.h"
#include "formatters/durationformatter.h"
#include "ichartview.h"
#include "models/addressspacemimedata.h"
#include "trendgraphtoolbar.h"

namespace {

constexpr qint64 kLiveWindowMs = 60000;
constexpr int kLiveTickMs = 1000;
constexpr int kZoomHistoryDelayMs = 400;

///
/// \brief Reports whether a history window matches a toolbar preset range.
/// \param windowMs Window length in milliseconds.
/// \return True for the 1m, 10m and 1h presets.
///
bool isPresetWindow(qint64 windowMs)
{
    return windowMs == 60000 || windowMs == 600000 || windowMs == 3600000;
}

const QVector<QColor> kSeriesPalette = {
    QColor(0x00, 0xb4, 0x46),
    QColor(0x25, 0x63, 0xeb),
    QColor(0xc0, 0x7d, 0x00),
    QColor(0xd1, 0x34, 0x38),
    QColor(0x8b, 0x5c, 0xf6),
    QColor(0x08, 0x91, 0xb2),
    QColor(0xdb, 0x27, 0x77),
    QColor(0x65, 0xa3, 0x0d),
};

QIcon swatchIcon(const QColor &color)
{
    QPixmap pixmap(12, 12);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawRoundedRect(pixmap.rect(), 3, 3);
    painter.end();
    return QIcon(pixmap);
}

}

///
/// \brief Builds the chart view and lays it out.
/// \param parent Parent widget.
///
TrendGraphWidget::TrendGraphWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TrendGraphWidget)
    , _chart(ChartViewFactory::createChartView(this))
{
    ui->setupUi(this);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAcceptDrops(true);

    QWidget *chartWidget = _chart->widget();
    ui->chartLayout->addWidget(chartWidget);
    chartWidget->installEventFilter(this);
    const QList<QWidget *> chartChildren = chartWidget->findChildren<QWidget *>();
    for (QWidget *child : chartChildren)
        child->installEventFilter(this);

    connectToolbar();

    _liveTimer = new QTimer(this);
    _liveTimer->setInterval(kLiveTickMs);
    connect(_liveTimer, &QTimer::timeout, this, &TrendGraphWidget::refreshLiveView);

    _zoomHistoryTimer = new QTimer(this);
    _zoomHistoryTimer->setSingleShot(true);
    _zoomHistoryTimer->setInterval(kZoomHistoryDelayMs);
    connect(_zoomHistoryTimer, &QTimer::timeout, this, &TrendGraphWidget::refreshZoomedHistory);

    applyTheme();
    applyDisplaySettings();

    enterLiveMode();
}

///
/// \brief Wires toolbar commands to chart behaviour.
///
void TrendGraphWidget::connectToolbar()
{
    connect(ui->toolbar, &TrendGraphToolbar::liveRequested,
            this, &TrendGraphWidget::enterLiveMode);
    connect(ui->toolbar, &TrendGraphToolbar::livePauseToggled,
            this, &TrendGraphWidget::setLivePaused);
    connect(ui->toolbar, &TrendGraphToolbar::historyRequested,
            this, &TrendGraphWidget::enterHistoryMode);
    connect(ui->toolbar, &TrendGraphToolbar::customIntervalRequested,
            this, &TrendGraphWidget::openCustomInterval);
    connect(ui->toolbar, &TrendGraphToolbar::refreshRequested,
            this, &TrendGraphWidget::refreshHistory);
    connect(ui->toolbar, &TrendGraphToolbar::autoScaleRequested,
            this, &TrendGraphWidget::autoScale);
    connect(ui->toolbar, &TrendGraphToolbar::fitRequested,
            this, &TrendGraphWidget::fit);
    connect(ui->toolbar, &TrendGraphToolbar::exportRequested,
            this, &TrendGraphWidget::exportChart);
    connect(ui->toolbar, &TrendGraphToolbar::settingsRequested,
            this, &TrendGraphWidget::openSettings);
}

///
/// \brief Destroys the widget and its chart view.
///
TrendGraphWidget::~TrendGraphWidget()
{
    delete ui;
}

///
/// \brief Returns a distinct palette colour for a series index.
/// \param index Series index.
/// \return Cycled palette colour.
///
QColor TrendGraphWidget::paletteColor(int index) const
{
    return kSeriesPalette.at(index % kSeriesPalette.size());
}

///
/// \brief Adds a node series if not already present.
/// \param nodeId Node NodeId.
/// \param displayName Human-readable node name.
/// \param displayPath Human-readable node path.
/// \return True when a new series was added.
///
bool TrendGraphWidget::addNode(const QString &nodeId, const QString &displayName,
                               const QString &displayPath)
{
    if (nodeId.isEmpty() || _series.contains(nodeId))
        return false;

    TrendSeries series(nodeId, displayName, displayPath);
    const QColor color = paletteColor(_series.size());
    series.setColor(color);
    _series.insert(nodeId, series);

    _chart->addSeries(nodeId, series.seriesLabel(_display.labelMode), color);

    emit nodeAdded(nodeId, displayName, displayPath);
    if (_mode == Mode::Live) {
        if (_display.autoScrollLive)
            applyWindow();
        subscribeNode(nodeId);
    }
    requestHistory(nodeId);
    return true;
}

///
/// \brief Removes a node series.
/// \param nodeId Node to remove.
///
void TrendGraphWidget::removeNode(const QString &nodeId)
{
    if (_series.remove(nodeId) == 0)
        return;
    _chart->removeSeries(nodeId);
    unsubscribeNode(nodeId);
    _state.removePendingHistory(nodeId);
    emit nodeRemoved(nodeId);
}

///
/// \brief Reports whether a node is charted here.
/// \param nodeId Node to test.
/// \return True when the node has a series.
///
bool TrendGraphWidget::hasNode(const QString &nodeId) const
{
    return _series.contains(nodeId);
}

///
/// \brief Returns the NodeIds of all charted series.
/// \return Charted NodeIds.
///
QStringList TrendGraphWidget::chartedNodeIds() const
{
    return _series.keys();
}

///
/// \brief Appends streamed values to any matching series.
/// \param values Latest data-access values.
///
void TrendGraphWidget::applyLiveValues(const QVector<OpcUaDataValue> &values)
{
    if (_mode != Mode::Live)
        return;
    QSet<QString> updatedNodes;
    for (const OpcUaDataValue &value : values) {
        auto it = _series.find(value.nodeId);
        if (it == _series.end())
            continue;
        const int before = it->points().size();
        if (!it->appendLive(value))
            continue;
        const QVector<QPointF> &points = it->points();
        if (points.size() == before)
            continue;
        const QPointF &point = points.constLast();
        if (_display.lineType == TrendLineType::Step && points.size() >= 2)
            _chart->appendPoint(value.nodeId, toChartX(point.x()),
                                points.at(points.size() - 2).y(), value.status);
        _chart->appendPoint(value.nodeId, toChartX(point.x()), point.y(), value.status);
        updatedNodes.insert(value.nodeId);
    }
    if (!_livePaused && !updatedNodes.isEmpty()) {
        const qreal now = toChartX(QDateTime::currentMSecsSinceEpoch());
        for (const QString &nodeId : std::as_const(updatedNodes))
            _chart->extendSeriesTo(nodeId, now);
    }
}

///
/// \brief Feeds history into a series, replacing in History mode and backfilling
/// behind live points in Live mode.
/// \param nodeId Node whose history arrived.
/// \param values History samples in time order.
///
void TrendGraphWidget::applyHistory(const QString &nodeId,
                                    const QVector<OpcUaHistoryValue> &values)
{
    auto it = _series.find(nodeId);
    if (it == _series.end())
        return;
    if (_mode == Mode::Live)
        it->backfillHistory(values);
    else
        it->setHistory(values);
    refeedSeries(*it);
    if (_mode == Mode::Live && !_livePaused)
        _chart->extendSeriesTo(nodeId, toChartX(QDateTime::currentMSecsSinceEpoch()));
}

///
/// \brief Sets the visible time window in milliseconds since the epoch.
/// \param startMsEpoch Window start.
/// \param endMsEpoch Window end.
///
void TrendGraphWidget::setTimeWindow(qreal startMsEpoch, qreal endMsEpoch)
{
    _chart->setTimeWindow(toChartX(startMsEpoch), toChartX(endMsEpoch));
}

///
/// \brief Scales the value axis to the data.
///
void TrendGraphWidget::autoScale()
{
    _valueZoomed = false;
    _chart->autoScaleY();
}

///
/// \brief Fits both axes to the full data extent.
///
void TrendGraphWidget::fit()
{
    _valueZoomed = false;
    _chart->fit();
}

///
/// \brief Renders the chart to an image for export.
/// \param size Target image size.
/// \return Rendered image.
///
QImage TrendGraphWidget::renderToImage(const QSize &size) const
{
    return _chart->renderToImage(size);
}

///
/// \brief Applies history results if this chart requested them for the node.
/// \param nodeId Node whose history arrived.
/// \param error Read error, empty on success.
/// \param values History samples in time order.
/// \return True when this chart had requested the node's history.
///
bool TrendGraphWidget::consumeHistory(const QString &nodeId, const QString &error,
                                      const QVector<OpcUaHistoryValue> &values)
{
    if (!_state.consumePendingHistory(nodeId))
        return false;
    if (error.isEmpty() && hasNode(nodeId)) {
        applyHistory(nodeId, values);
        if (!_valueZoomed)
            autoScale();
    }
    return true;
}

///
/// \brief Removes all series and their points, unsubscribing live nodes.
///
void TrendGraphWidget::clear()
{
    const QSet<QString> subscribed = _state.subscribedNodes();
    for (const QString &nodeId : subscribed)
        unsubscribeNode(nodeId);
    _state.clearPendingHistory();
    _series.clear();
    _chart->clearAll();
}

///
/// \brief Returns the active mode as an int for persistence.
/// \return 0 for Live, 1 for History.
///
int TrendGraphWidget::modeState() const
{
    return static_cast<int>(_mode);
}

///
/// \brief Returns the active window length in milliseconds.
/// \return Window length.
///
qint64 TrendGraphWidget::windowState() const
{
    return _windowMs;
}

///
/// \brief Restores a persisted mode and window.
/// \param mode 0 for Live, 1 for History.
/// \param windowMs Window length in milliseconds.
///
void TrendGraphWidget::applyModeState(int mode, qint64 windowMs)
{
    if (static_cast<Mode>(mode) == Mode::History)
        enterHistoryMode(windowMs);
    else
        enterLiveMode();
}

///
/// \brief Returns the current display, range and auto-scroll settings.
/// \return Active settings.
///
TrendDisplaySettings TrendGraphWidget::displaySettings() const
{
    TrendDisplaySettings settings = _display;
    settings.mode = modeState();
    settings.windowMs = windowState();
    return settings;
}

///
/// \brief Applies display, range and auto-scroll settings to the chart.
/// \param settings Settings to apply.
///
void TrendGraphWidget::setDisplaySettings(const TrendDisplaySettings &settings)
{
    const bool intervalChanged =
        settings.mode != modeState() || settings.windowMs != windowState();
    const bool liveUpdateChanged = settings.liveSubscription != _display.liveSubscription;
    const bool lineTypeChanged = settings.lineType != _display.lineType;

    _display = settings;
    applyDisplaySettings();
    if (lineTypeChanged)
        for (const TrendSeries &series : std::as_const(_series))
            refeedSeries(series);
    if (intervalChanged)
        applyModeState(settings.mode, settings.windowMs);
    else if (liveUpdateChanged && _mode == Mode::Live)
        resubscribeLiveNodes();
    if (_display.autoScale)
        autoScale();
}

///
/// \brief Pushes the display toggles (legend, grid, smoothing, points) to the chart.
///
void TrendGraphWidget::applyDisplaySettings()
{
    _chart->setLegendVisible(_display.showLegend);
    _chart->setGridVisible(_display.showGrid);
    _chart->setSmoothLines(_display.smoothLines);
    _chart->setPointsVisible(_display.showPoints);
    _chart->setHoverValueVisible(_display.showValueTooltip);
    _liveTimer->setInterval(qMax(1, static_cast<int>(liveIntervalMs())));
    for (const TrendSeries &series : std::as_const(_series))
        _chart->setSeriesName(series.nodeId(), series.seriesLabel(_display.labelMode));
}

///
/// \brief Returns the charted series with their colours and visibility.
/// \return Series in legend-label order.
///
QVector<TrendSeriesInfo> TrendGraphWidget::seriesInfos() const
{
    const TrendLabelMode mode = _display.labelMode;
    QList<TrendSeries> ordered = _series.values();
    std::sort(ordered.begin(), ordered.end(),
              [mode](const TrendSeries &lhs, const TrendSeries &rhs) {
                  return lhs.seriesLabel(mode).localeAwareCompare(rhs.seriesLabel(mode)) < 0;
              });

    QVector<TrendSeriesInfo> result;
    result.reserve(ordered.size());
    for (const TrendSeries &series : std::as_const(ordered))
        result.append({ series.nodeId(), series.seriesLabel(mode),
                        series.displayName(), series.displayPath(),
                        series.color(), series.isVisible() });
    return result;
}

///
/// \brief Applies edited per-series colours and visibility.
/// \param series Series carrying updated colour and visibility.
///
void TrendGraphWidget::applySeriesInfos(const QVector<TrendSeriesInfo> &series)
{
    for (const TrendSeriesInfo &info : series) {
        auto it = _series.find(info.nodeId);
        if (it == _series.end())
            continue;
        if (it->color() != info.color) {
            it->setColor(info.color);
            _chart->setSeriesColor(info.nodeId, info.color);
        }
        if (it->isVisible() != info.visible) {
            it->setVisible(info.visible);
            _chart->setSeriesVisible(info.nodeId, info.visible);
        }
    }
    if (_display.autoScale)
        autoScale();
}

///
/// \brief Opens the settings dialog and applies any accepted changes.
///
void TrendGraphWidget::openSettings()
{
    emit settingsRequested();

    TrendSettingsDialog dialog(this);
    connect(&dialog, &TrendSettingsDialog::subscriptionCreationRequested,
            this, &TrendGraphWidget::subscriptionCreationRequested);
    dialog.setSubscriptions(_subscriptions);
    dialog.setDisplaySettings(displaySettings());
    dialog.setSeries(seriesInfos());
    if (dialog.exec() != QDialog::Accepted)
        return;

    setDisplaySettings(dialog.displaySettings());
    applySeriesInfos(dialog.series());
}

///
/// \brief Switches to live streaming and subscribes the charted nodes.
///
void TrendGraphWidget::enterLiveMode()
{
    _mode = Mode::Live;
    _livePaused = false;
    _valueZoomed = false;
    _customInterval = false;
    _absoluteInterval = false;
    _windowMs = kLiveWindowMs;
    _windowEndMs = 0;
    ui->toolbar->setInterval(QString(), QString());
    ui->toolbar->selectLive();
    ui->toolbar->setRefreshEnabled(false);
    ui->toolbar->setFitEnabled(false);
    const QStringList ids = chartedNodeIds();
    for (const QString &nodeId : ids)
        subscribeNode(nodeId);
    applyWindow();
    if (!_liveTimer->isActive())
        _liveTimer->start();
}

///
/// \brief Pauses or resumes live scrolling and rescaling without dropping data.
///
/// While paused the rolling window and value axis stay frozen so the chart can be
/// inspected; incoming samples keep buffering and become visible on resume, which
/// snaps the window back to now.
///
/// \param paused True to freeze the live view, false to resume.
///
void TrendGraphWidget::setLivePaused(bool paused)
{
    if (_mode != Mode::Live || _livePaused == paused)
        return;
    _livePaused = paused;
    ui->toolbar->setLivePaused(paused);
    if (!paused) {
        extendLiveSeries(QDateTime::currentMSecsSinceEpoch());
        applyWindow();
        if (_display.autoScale)
            autoScale();
    }
}

///
/// \brief Switches to historical reads over a rolling window.
/// \param windowMs Window length in milliseconds.
///
void TrendGraphWidget::enterHistoryMode(qint64 windowMs)
{
    _mode = Mode::History;
    _valueZoomed = false;
    _customInterval = !isPresetWindow(windowMs);
    _absoluteInterval = false;
    _windowMs = windowMs;
    _windowEndMs = QDateTime::currentMSecsSinceEpoch();
    if (!_customInterval)
        ui->toolbar->setInterval(QString(), QString());
    ui->toolbar->selectHistoryWindow(windowMs);
    ui->toolbar->setRefreshEnabled(true);
    ui->toolbar->setFitEnabled(true);
    _liveTimer->stop();
    const QSet<QString> subscribed = _state.subscribedNodes();
    for (const QString &nodeId : subscribed)
        unsubscribeNode(nodeId);
    applyWindow();
    const QStringList ids = chartedNodeIds();
    for (const QString &nodeId : ids)
        requestHistory(nodeId);
}

///
/// \brief Opens the custom interval dialog and applies an accepted range.
///
/// On cancel the toolbar selection is restored to the active mode so the Custom
/// button does not appear checked while a different interval is shown.
///
void TrendGraphWidget::openCustomInterval()
{
    const qint64 end = (_mode == Mode::History && _windowEndMs > 0)
        ? _windowEndMs
        : QDateTime::currentMSecsSinceEpoch();
    const qint64 start = end - _windowMs;

    CustomIntervalDialog dialog(this);
    dialog.setInterval(QDateTime::fromMSecsSinceEpoch(start),
                       QDateTime::fromMSecsSinceEpoch(end));
    if (dialog.exec() != QDialog::Accepted) {
        if (_mode == Mode::Live)
            ui->toolbar->selectLive();
        else if (_absoluteInterval)
            ui->toolbar->selectCustom();
        else
            ui->toolbar->selectHistoryWindow(_windowMs);
        return;
    }

    enterCustomInterval(dialog.fromDateTime(), dialog.toDateTime(), dialog.isRelative());
}

///
/// \brief Switches to a history read over an explicit start/end interval.
///
/// A relative range (the dialog's "last N units") tracks "now" on later refreshes;
/// an absolute From/To range stays pinned so refresh re-reads the same window.
///
/// \param start Inclusive interval start.
/// \param end Inclusive interval end.
/// \param relative True when the range should follow "now" on refresh.
///
void TrendGraphWidget::enterCustomInterval(const QDateTime &start, const QDateTime &end,
                                           bool relative)
{
    _mode = Mode::History;
    _valueZoomed = false;
    _customInterval = true;
    _absoluteInterval = !relative;
    _windowMs = start.msecsTo(end);
    _windowEndMs = end.toMSecsSinceEpoch();
    ui->toolbar->selectCustom();
    ui->toolbar->setRefreshEnabled(true);
    ui->toolbar->setFitEnabled(true);
    _liveTimer->stop();
    const QSet<QString> subscribed = _state.subscribedNodes();
    for (const QString &nodeId : subscribed)
        unsubscribeNode(nodeId);
    applyWindow();
    const QStringList ids = chartedNodeIds();
    for (const QString &nodeId : ids)
        requestHistory(nodeId);
}

///
/// \brief Re-anchors the history window to now and re-reads every series.
///
/// History reads stay pinned to the window captured when the mode was entered so
/// that series added later share one interval; this button advances the anchor to
/// the current time and refetches, giving an explicit, aligned refresh.
///
void TrendGraphWidget::refreshHistory()
{
    if (_mode != Mode::History)
        return;
    if (!_absoluteInterval)
        _windowEndMs = QDateTime::currentMSecsSinceEpoch();
    applyWindow();
    const QStringList ids = chartedNodeIds();
    for (const QString &nodeId : ids)
        requestHistory(nodeId);
}

///
/// \brief Advances live series to now and refreshes the rolling axes.
///
void TrendGraphWidget::refreshLiveView()
{
    if (_livePaused || _series.isEmpty())
        return;

    extendLiveSeries(QDateTime::currentMSecsSinceEpoch());
    if (_display.autoScrollLive)
        applyWindow();
    if (_display.autoScale)
        autoScale();
}

///
/// \brief Extends every non-empty live series with its last known value.
/// \param endMsEpoch Trailing timestamp in milliseconds since the epoch.
///
void TrendGraphWidget::extendLiveSeries(qint64 endMsEpoch)
{
    const qreal chartEnd = toChartX(static_cast<qreal>(endMsEpoch));
    for (auto it = _series.cbegin(); it != _series.cend(); ++it)
        _chart->extendSeriesTo(it.key(), chartEnd);
}

///
/// \brief Applies the rolling time window to the chart.
///
void TrendGraphWidget::applyWindow()
{
    const qint64 end = (_mode == Mode::History && _windowEndMs > 0)
        ? _windowEndMs
        : QDateTime::currentMSecsSinceEpoch();
    const qint64 start = end - _windowMs;
    setTimeWindow(static_cast<qreal>(start), static_cast<qreal>(end));
    if (_customInterval)
        updateIntervalBar(start, end);
}

///
/// \brief Shows the visible custom interval as text beside the Custom button.
/// \param startMs Window start in milliseconds since the epoch.
/// \param endMs Window end in milliseconds since the epoch.
///
void TrendGraphWidget::updateIntervalBar(qint64 startMs, qint64 endMs)
{
    const bool utc = _timestampMode == AppSettings::TimestampMode::Utc;
    auto format = [utc](qint64 ms) {
        QDateTime dt = QDateTime::fromMSecsSinceEpoch(ms);
        if (utc)
            dt = dt.toUTC();
        return dt.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    };
    ui->toolbar->setInterval(QStringLiteral("%1 \u2014 %2").arg(format(startMs), format(endMs)),
                             formatDuration(endMs - startMs));
}

///
/// \brief Subscribes a node once, emitting the request on first reference.
/// \param nodeId Node to monitor.
///
void TrendGraphWidget::subscribeNode(const QString &nodeId)
{
    if (_state.subscribe(nodeId))
        emit subscribeRequested(nodeId, liveIntervalMs());
}

///
/// \brief Resolves the live publishing interval from the selected subscription.
/// \return Publishing interval in milliseconds, or the default when unresolved.
///
double TrendGraphWidget::liveIntervalMs() const
{
    for (const SubscriptionItem &item : _subscriptions) {
        if (item.name == _display.liveSubscription)
            return item.publishingInterval;
    }
    return kLiveTickMs;
}

///
/// \brief Re-requests monitoring for every live node at the current interval.
///
/// Called when the live update interval changes so the host re-negotiates the
/// publishing interval of already-subscribed nodes.
///
void TrendGraphWidget::resubscribeLiveNodes()
{
    const QSet<QString> subscribed = _state.subscribedNodes();
    for (const QString &nodeId : subscribed)
        emit subscribeRequested(nodeId, liveIntervalMs());
}

///
/// \brief Updates the known subscriptions and re-negotiates the live interval.
/// \param subscriptions Current subscriptions in row order.
///
void TrendGraphWidget::setSubscriptions(const QVector<SubscriptionItem> &subscriptions)
{
    const double before = liveIntervalMs();
    _subscriptions = subscriptions;
    const double after = liveIntervalMs();
    if (qFuzzyCompare(before, after))
        return;
    _liveTimer->setInterval(qMax(1, static_cast<int>(after)));
    if (_mode == Mode::Live)
        resubscribeLiveNodes();
}

///
/// \brief Repoints the live subscription reference when a subscription is renamed.
/// \param oldName Previous subscription name.
/// \param newName New subscription name.
///
void TrendGraphWidget::applySubscriptionRename(const QString &oldName, const QString &newName)
{
    if (_display.liveSubscription == oldName)
        _display.liveSubscription = newName;
}

///
/// \brief Unsubscribes a node, emitting the request when it was monitored.
/// \param nodeId Node to stop monitoring.
///
void TrendGraphWidget::unsubscribeNode(const QString &nodeId)
{
    if (_state.unsubscribe(nodeId))
        emit unsubscribeRequested(nodeId);
}

///
/// \brief Requests a history read for a node over the active window.
/// \param nodeId Node whose history is read.
///
void TrendGraphWidget::requestHistory(const QString &nodeId)
{
    const QDateTime end = (_windowEndMs > 0)
        ? QDateTime::fromMSecsSinceEpoch(_windowEndMs)
        : QDateTime::currentDateTime();
    const QDateTime start = end.addMSecs(-_windowMs);
    _state.addPendingHistory(nodeId);
    emit historyReadRequested(nodeId, start, end, 0);
}

///
/// \brief Saves the chart as a PNG image.
///
void TrendGraphWidget::exportChart()
{
    const QString fileName = QFileDialog::getSaveFileName(
        this, tr("Export Trend"), QStringLiteral("trend.png"),
        tr("PNG Image (*.png);;All Files (*)"));
    if (fileName.isEmpty())
        return;

    const QImage image = renderToImage(size() * 2);
    if (!image.save(fileName)) {
        MessageBoxDialog::warning(this, tr("Export Trend"),
                                  tr("Could not save '%1'.").arg(fileName),
                                  DialogButtonBox::Ok);
    }
}

///
/// \brief Applies the timestamp display mode and re-feeds the chart.
/// \param mode Local time or UTC.
///
void TrendGraphWidget::setTimestampMode(AppSettings::TimestampMode mode)
{
    if (_timestampMode == mode)
        return;
    _timestampMode = mode;
    for (const TrendSeries &series : std::as_const(_series))
        refeedSeries(series);
    if (_mode == Mode::Live && !_livePaused)
        extendLiveSeries(QDateTime::currentMSecsSinceEpoch());
    applyWindow();
}

///
/// \brief Maps an epoch timestamp to the chart's local-time X axis.
/// \param epochMs Source timestamp in milliseconds since the epoch (UTC).
/// \return X position so the local-time axis renders the chosen wall clock.
///
qreal TrendGraphWidget::toChartX(qreal epochMs) const
{
    if (_timestampMode != AppSettings::TimestampMode::Utc)
        return epochMs;
    const qint64 offsetMs = qint64(QDateTime::currentDateTime().offsetFromUtc()) * 1000;
    return epochMs - static_cast<qreal>(offsetMs);
}

///
/// \brief Maps an X position read back from the chart to an epoch timestamp.
/// \param chartX X position on the chart's local-time axis.
/// \return Timestamp in milliseconds since the epoch (UTC).
///
qreal TrendGraphWidget::fromChartX(qreal chartX) const
{
    if (_timestampMode != AppSettings::TimestampMode::Utc)
        return chartX;
    const qint64 offsetMs = qint64(QDateTime::currentDateTime().offsetFromUtc()) * 1000;
    return chartX + static_cast<qreal>(offsetMs);
}

///
/// \brief Pushes a series' buffered points into the chart, honouring the line type.
///
/// OPC UA monitored items report on value change, so the value is constant between
/// samples. In Step mode a corner is inserted at each new time to keep the previous
/// value flat until it steps, matching how the dense server history draws instead of
/// sloping between sparse live samples; in Line mode the raw points are fed straight.
///
/// \param series Series whose points are re-fed.
///
void TrendGraphWidget::refeedSeries(const TrendSeries &series)
{
    const QVector<QPointF> &points = series.points();
    const QVector<QString> &statuses = series.statuses();
    QVector<ChartPoint> mapped;
    mapped.reserve(points.size() * 2);
    for (int i = 0; i < points.size(); ++i) {
        const QString status = i < statuses.size() ? statuses.at(i) : QString();
        if (_display.lineType == TrendLineType::Step && i > 0)
            mapped.append(ChartPoint{toChartX(points.at(i).x()), points.at(i - 1).y(), status});
        mapped.append(ChartPoint{toChartX(points.at(i).x()), points.at(i).y(), status});
    }
    _chart->setPoints(series.nodeId(), mapped);
}

///
/// \brief Builds a chart theme from AppColors and applies it.
///
void TrendGraphWidget::applyTheme()
{
    ChartTheme theme;
    theme.background = palette().color(QPalette::Base);
    theme.grid = AppColors::noticeNeutralBorder();
    theme.axis = AppColors::caption();
    theme.text = AppColors::subtitleText();
    theme.legendText = AppColors::titleText();
    theme.statusGood = AppColors::statusSuccess();
    theme.statusUncertain = AppColors::statusWarning();
    theme.statusBad = AppColors::statusError();
    theme.seriesPalette = kSeriesPalette;
    _chart->setTheme(theme);
}

///
/// \brief Reports whether a drag carries a chartable variable node.
/// \param mimeData Drag MIME data.
/// \return True when the drag holds a numeric scalar variable node.
///
/// Nodes whose DataType cannot be plotted are refused by the drag itself, so the
/// cursor already reports that the drop will not add a series.
///
bool TrendGraphWidget::acceptsNodeDrag(const QMimeData *mimeData) const
{
    OpcUaNodeInfo node;
    return AddressSpaceMime::decodeNode(mimeData, &node)
        && !node.nodeId.isEmpty() && TrendSeries::isTrendable(node);
}

///
/// \brief Adds a dropped variable node as a new series.
/// \param mimeData Drop MIME data.
/// \return True when a chartable variable node was added.
///
bool TrendGraphWidget::dropNode(const QMimeData *mimeData)
{
    OpcUaNodeInfo node;
    if (!AddressSpaceMime::decodeNode(mimeData, &node)
        || node.nodeId.isEmpty() || !TrendSeries::isTrendable(node)) {
        return false;
    }
    const QString label = node.displayName.isEmpty()
        ? (node.browseName.isEmpty() ? node.nodeId : node.browseName)
        : node.displayName;
    addNode(node.nodeId, label, node.displayPath);
    return true;
}

///
/// \brief Zooms the chart under the cursor from a wheel rotation.
/// \param globalPos Wheel position in global screen coordinates.
/// \param angleDelta Wheel rotation in eighths of a degree.
/// \param valueAxis True to zoom the value axis instead of the time axis.
/// \return True when the rotation was taken as a zoom.
///
/// Live mode owns its viewport: the window follows "now" and the value axis tracks
/// the incoming samples, so a zoom there would be undone by the next tick.
///
bool TrendGraphWidget::zoomChart(const QPoint &globalPos, int angleDelta, bool valueAxis)
{
    if (_mode != Mode::History)
        return false;

    const qreal factor = ChartZoom::factorFromWheel(angleDelta);
    if (qFuzzyCompare(factor, 1.0))
        return false;
    if (valueAxis)
        zoomValueRange(globalPos, factor);
    else
        zoomTimeWindow(globalPos, factor);
    return true;
}

///
/// \brief Scales the visible time window around the cursor.
/// \param globalPos Wheel position in global screen coordinates.
/// \param factor Window length multiplier.
///
/// Zooming out reveals time that was never read, hence the deferred re-read once the
/// wheel settles.
///
void TrendGraphWidget::zoomTimeWindow(const QPoint &globalPos, qreal factor)
{
    ChartRange window = _chart->timeWindow();
    window.min = fromChartX(window.min);
    window.max = fromChartX(window.max);

    QPointF cursor;
    const qreal anchor = _chart->valueAt(globalPos, &cursor)
        ? fromChartX(cursor.x())
        : window.max;

    if (!ChartZoom::scaleRange(&window, anchor, factor,
                               ChartZoom::kMinTimeSpanMs, ChartZoom::kMaxTimeSpanMs)) {
        return;
    }

    applyCustomWindow(window);
}

///
/// \brief Starts panning the chart when history is shown.
/// \param globalPos Press position in global screen coordinates.
/// \return True when the press was taken as the start of a pan.
///
bool TrendGraphWidget::startPan(const QPoint &globalPos)
{
    if (_mode != Mode::History)
        return false;
    _panning = true;
    _panOrigin = globalPos;
    for (QWidget *target : chartCursorTargets())
        target->setCursor(Qt::ClosedHandCursor);
    return true;
}

///
/// \brief Drags the visible interval and value range along with the cursor.
/// \param globalPos Current cursor position in global screen coordinates.
///
/// The shift is applied per move event rather than against the press position, so the
/// data under the cursor keeps following it even though the axis ranges change while
/// dragging. A vertical drag takes the value axis over from auto-scaling, as a manual
/// value zoom does.
///
void TrendGraphWidget::panChart(const QPoint &globalPos)
{
    QPointF delta;
    if (!_chart->valueDelta(globalPos - _panOrigin, &delta))
        return;
    _panOrigin = globalPos;

    if (!qFuzzyIsNull(delta.y())) {
        ChartRange values = _chart->valueRange();
        values.min -= delta.y();
        values.max -= delta.y();
        _valueZoomed = true;
        _chart->setValueRange(values);
    }

    if (qFuzzyIsNull(delta.x()))
        return;

    ChartRange window = _chart->timeWindow();
    window.min = fromChartX(window.min) - delta.x();
    window.max = fromChartX(window.max) - delta.x();
    applyCustomWindow(window);
}

///
/// \brief Ends a pan and restores the cursor.
///
void TrendGraphWidget::endPan()
{
    _panning = false;
    for (QWidget *target : chartCursorTargets())
        target->unsetCursor();
}

///
/// \brief Lists the widgets whose cursor covers the plot.
/// \return The chart view and the viewport that draws the plot.
///
/// The pointer sits over the view's viewport, and a scroll area may give that
/// viewport a cursor of its own, which would mask one set on the view alone.
///
QList<QWidget *> TrendGraphWidget::chartCursorTargets() const
{
    QWidget *chartWidget = _chart->widget();
    QList<QWidget *> targets = chartWidget->findChildren<QWidget *>();
    targets.append(chartWidget);
    return targets;
}

///
/// \brief Shows an explicit interval and schedules the history read that fills it.
/// \param window Visible interval in milliseconds since the epoch.
///
void TrendGraphWidget::applyCustomWindow(const ChartRange &window)
{
    _windowMs = qRound64(window.span());
    _windowEndMs = qRound64(window.max);
    setTimeWindow(window.min, window.max);
    _customInterval = true;
    _absoluteInterval = true;
    ui->toolbar->selectCustom();
    updateIntervalBar(qRound64(window.min), _windowEndMs);
    _zoomHistoryTimer->start();
}

///
/// \brief Scales the value axis around the cursor and takes over from auto-scaling.
/// \param globalPos Wheel position in global screen coordinates.
/// \param factor Value span multiplier.
///
/// Arriving history rescales the value axis to the data, which would undo the zoom,
/// so a manual value range holds until an explicit Auto Scale or Fit.
///
void TrendGraphWidget::zoomValueRange(const QPoint &globalPos, qreal factor)
{
    ChartRange range = _chart->valueRange();
    QPointF cursor;
    const qreal anchor = _chart->valueAt(globalPos, &cursor)
        ? cursor.y()
        : (range.min + range.max) / 2.0;

    if (!ChartZoom::scaleRange(&range, anchor, factor, ChartZoom::kMinValueSpan, 0.0))
        return;

    _valueZoomed = true;
    _chart->setValueRange(range);
}

///
/// \brief Re-reads history over the zoomed window once the wheel has settled.
///
void TrendGraphWidget::refreshZoomedHistory()
{
    if (_mode != Mode::History)
        return;
    const QStringList ids = chartedNodeIds();
    for (const QString &nodeId : ids)
        requestHistory(nodeId);
}

///
/// \brief Shows the chart context menu: view actions plus node removal.
///
/// Auto Scale and Settings are always offered, Fit only outside Live mode where the
/// window follows "now"; per-node Remove entries and Clear appear only while the
/// chart has series.
///
/// \param globalPos Menu position in global screen coordinates.
///
void TrendGraphWidget::showSeriesContextMenu(const QPoint &globalPos)
{
    QMenu menu(this);

    if (_mode == Mode::Live) {
        const QString icon = _livePaused ? QStringLiteral("resume") : QStringLiteral("pause");
        const QString text = _livePaused ? tr("Resume") : tr("Pause");
        menu.addAction(AppIcons::themed(icon), text, this,
                       [this]() { setLivePaused(!_livePaused); });
        menu.addSeparator();
    }

    menu.addAction(tr("Auto Scale"), this, [this]() { autoScale(); });
    QAction *fitAction = menu.addAction(AppIcons::themed(QStringLiteral("fit")), tr("Fit"), this,
                                        [this]() { fit(); });
    fitAction->setEnabled(_mode == Mode::History);
    menu.addAction(AppIcons::themed(QStringLiteral("settings")), tr("Settings"),
                   this, [this]() { openSettings(); });

    if (!_series.isEmpty()) {
        menu.addSeparator();

        const TrendLabelMode mode = _display.labelMode;
        QList<TrendSeries> ordered = _series.values();
        std::sort(ordered.begin(), ordered.end(),
                  [mode](const TrendSeries &lhs, const TrendSeries &rhs) {
                      return lhs.seriesLabel(mode).localeAwareCompare(rhs.seriesLabel(mode)) < 0;
                  });

        for (const TrendSeries &series : std::as_const(ordered)) {
            const QString nodeId = series.nodeId();
            menu.addAction(swatchIcon(series.color()),
                           tr("Remove %1").arg(series.seriesLabel(mode)), this,
                           [this, nodeId]() { removeNode(nodeId); });
        }

        menu.addAction(AppIcons::themed(QStringLiteral("remove")), tr("Clear"),
                       this, [this]() { clear(); });
    }

    menu.exec(globalPos);
}

///
/// \brief Re-applies the chart theme after the palette has switched.
///
/// The colour-scheme change reaches widgets as a palette change; handling it here
/// (rather than on AppTheme::colorSchemeChanged) guarantees palette() already
/// holds the new colours when applyTheme() reads them.
///
void TrendGraphWidget::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);

    if (event->type() == QEvent::PaletteChange
        || event->type() == QEvent::ApplicationPaletteChange) {
        applyTheme();
    } else if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

///
/// \brief Routes drags, wheel zooming, panning and context menus over the chart view here.
///
/// The chart view is a scroll area that handles these itself, so events landing on
/// it never reach the widget's own handlers; this filter intercepts them.
///
bool TrendGraphWidget::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
    case QEvent::DragEnter:
    case QEvent::DragMove: {
        auto *dragEvent = static_cast<QDragMoveEvent *>(event);
        if (acceptsNodeDrag(dragEvent->mimeData())) {
            dragEvent->setDropAction(Qt::CopyAction);
            dragEvent->accept();
            return true;
        }
        dragEvent->ignore();
        return true;
    }
    case QEvent::Drop: {
        auto *dropEvent = static_cast<QDropEvent *>(event);
        if (dropNode(dropEvent->mimeData())) {
            dropEvent->setDropAction(Qt::CopyAction);
            dropEvent->accept();
            return true;
        }
        dropEvent->ignore();
        return true;
    }
    case QEvent::Wheel: {
        auto *wheelEvent = static_cast<QWheelEvent *>(event);
        if (!zoomChart(wheelEvent->globalPosition().toPoint(), wheelEvent->angleDelta().y(),
                       wheelEvent->modifiers().testFlag(Qt::ControlModifier))) {
            break;
        }
        wheelEvent->accept();
        return true;
    }
    case QEvent::MouseButtonPress: {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() != Qt::LeftButton
            || !startPan(mouseEvent->globalPosition().toPoint())) {
            break;
        }
        mouseEvent->accept();
        return true;
    }
    case QEvent::MouseMove: {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (!_panning)
            break;
        panChart(mouseEvent->globalPosition().toPoint());
        mouseEvent->accept();
        return true;
    }
    case QEvent::MouseButtonRelease: {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (!_panning || mouseEvent->button() != Qt::LeftButton)
            break;
        endPan();
        mouseEvent->accept();
        return true;
    }
    case QEvent::ContextMenu: {
        auto *menuEvent = static_cast<QContextMenuEvent *>(event);
        showSeriesContextMenu(menuEvent->globalPos());
        menuEvent->accept();
        return true;
    }
    default:
        break;
    }
    return QWidget::eventFilter(watched, event);
}

///
/// \brief Accepts a drag that carries a droppable variable node.
///
void TrendGraphWidget::dragEnterEvent(QDragEnterEvent *event)
{
    dragMoveEvent(event);
}

///
/// \brief Accepts the drag as a copy while it carries a variable node.
///
void TrendGraphWidget::dragMoveEvent(QDragMoveEvent *event)
{
    if (acceptsNodeDrag(event->mimeData())) {
        event->setDropAction(Qt::CopyAction);
        event->accept();
    } else {
        event->ignore();
    }
}

///
/// \brief Adds the dropped variable node as a new series.
///
void TrendGraphWidget::dropEvent(QDropEvent *event)
{
    if (dropNode(event->mimeData())) {
        event->setDropAction(Qt::CopyAction);
        event->accept();
    } else {
        event->ignore();
    }
}

///
/// \brief Offers the remove menu for right-clicks on the widget's own area.
///
void TrendGraphWidget::contextMenuEvent(QContextMenuEvent *event)
{
    showSeriesContextMenu(event->globalPos());
    event->accept();
}
