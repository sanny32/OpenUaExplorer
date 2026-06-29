// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file trendgraphwidget.cpp
/// \brief Implements the trend graph widget.
///

#include "trendgraphwidget.h"

#include <algorithm>
#include <utility>

#include <QAbstractButton>
#include <QButtonGroup>
#include <QContextMenuEvent>
#include <QDateTime>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QIcon>
#include <QImage>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QPixmap>
#include <QSpacerItem>
#include <QTimer>
#include <QVBoxLayout>

#include "appcolors.h"
#include "appicons.h"
#include "charttypes.h"
#include "chartviewfactory.h"
#include "ichartview.h"
#include "models/addressspacemimedata.h"
#include "themedtoolbutton.h"

namespace {

constexpr qint64 kLiveWindowMs = 60000;
constexpr int kLiveTickMs = 1000;
constexpr double kPublishingIntervalMs = 1000.0;

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
    , _chart(ChartViewFactory::createChartView(this))
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAcceptDrops(true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    buildToolbar(layout);

    QWidget *chartWidget = _chart->widget();
    layout->addWidget(chartWidget);
    chartWidget->installEventFilter(this);
    const QList<QWidget *> chartChildren = chartWidget->findChildren<QWidget *>();
    for (QWidget *child : chartChildren)
        child->installEventFilter(this);

    _liveTimer = new QTimer(this);
    _liveTimer->setInterval(kLiveTickMs);
    connect(_liveTimer, &QTimer::timeout, this, [this]() {
        applyWindow();
        autoScale();
    });

    applyTheme();

    enterLiveMode();
}

///
/// \brief Builds the per-chart toolbar and wires its buttons.
/// \param layout Vertical layout to host the toolbar row.
///
void TrendGraphWidget::buildToolbar(QVBoxLayout *layout)
{
    const auto makeRangeButton = [this](const QString &name, const QString &text) {
        auto *button = new ThemedToolButton(this);
        button->setObjectName(name);
        button->setText(text);
        button->setCheckable(true);
        button->setToolButtonStyle(Qt::ToolButtonTextOnly);
        button->setMinimumWidth(40);
        return button;
    };

    _liveButton = makeRangeButton(QStringLiteral("liveButton"), tr("Live"));
    _liveButton->setChecked(true);
    _oneMinuteButton = makeRangeButton(QStringLiteral("oneMinuteButton"), tr("1m"));
    _tenMinutesButton = makeRangeButton(QStringLiteral("tenMinutesButton"), tr("10m"));
    _oneHourButton = makeRangeButton(QStringLiteral("oneHourButton"), tr("1h"));
    _oneDayButton = makeRangeButton(QStringLiteral("oneDayButton"), tr("1d"));

    auto *autoScaleButton = new ThemedToolButton(this);
    autoScaleButton->setObjectName(QStringLiteral("autoScaleButton"));
    autoScaleButton->setText(tr("Auto Scale"));
    autoScaleButton->setToolButtonStyle(Qt::ToolButtonTextOnly);

    auto *fitButton = new ThemedToolButton(this);
    fitButton->setObjectName(QStringLiteral("fitButton"));
    fitButton->setText(tr("Fit"));
    fitButton->setIcon(QStringLiteral("fit"));
    fitButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    auto *exportButton = new ThemedToolButton(this);
    exportButton->setObjectName(QStringLiteral("exportButton"));
    exportButton->setSquareIconOnly(true);
    exportButton->setIcon(QStringLiteral("export"));

    auto *settingsButton = new ThemedToolButton(this);
    settingsButton->setObjectName(QStringLiteral("settingsButton"));
    settingsButton->setSquareIconOnly(true);
    settingsButton->setIcon(QStringLiteral("settings"));

    auto *toolbar = new QHBoxLayout;
    toolbar->addWidget(_liveButton);
    toolbar->addWidget(_oneMinuteButton);
    toolbar->addWidget(_tenMinutesButton);
    toolbar->addWidget(_oneHourButton);
    toolbar->addWidget(_oneDayButton);
    toolbar->addStretch(1);
    toolbar->addWidget(autoScaleButton);
    toolbar->addWidget(fitButton);
    toolbar->addWidget(exportButton);
    toolbar->addWidget(settingsButton);
    layout->addLayout(toolbar);

    _modeGroup = new QButtonGroup(this);
    _modeGroup->setExclusive(true);
    _modeGroup->addButton(_liveButton);
    _modeGroup->addButton(_oneMinuteButton);
    _modeGroup->addButton(_tenMinutesButton);
    _modeGroup->addButton(_oneHourButton);
    _modeGroup->addButton(_oneDayButton);

    connect(_liveButton, &QAbstractButton::clicked, this,
            [this]() { enterLiveMode(); });
    connect(_oneMinuteButton, &QAbstractButton::clicked, this,
            [this]() { enterHistoryMode(60000); });
    connect(_tenMinutesButton, &QAbstractButton::clicked, this,
            [this]() { enterHistoryMode(600000); });
    connect(_oneHourButton, &QAbstractButton::clicked, this,
            [this]() { enterHistoryMode(3600000); });
    connect(_oneDayButton, &QAbstractButton::clicked, this,
            [this]() { enterHistoryMode(86400000); });

    connect(autoScaleButton, &QAbstractButton::clicked, this,
            [this]() { autoScale(); });
    connect(fitButton, &QAbstractButton::clicked, this,
            [this]() { fit(); });
    connect(exportButton, &QAbstractButton::clicked, this,
            [this]() { exportChart(); });
    connect(settingsButton, &QAbstractButton::clicked, this,
            &TrendGraphWidget::settingsRequested);
}

///
/// \brief Destroys the widget and its chart view.
///
TrendGraphWidget::~TrendGraphWidget() = default;

///
/// \brief Returns a distinct palette colour for a series index.
/// \param index Series index.
/// \return Cycled palette colour.
///
QColor TrendGraphWidget::paletteColor(int index) const
{
    return kSeriesPalette.at(index % kSeriesPalette.size());
}

bool TrendGraphWidget::addNode(const QString &nodeId, const QString &displayName,
                               const QString &displayPath)
{
    if (nodeId.isEmpty() || _series.contains(nodeId))
        return false;

    TrendSeries series(nodeId, displayName, displayPath);
    const QColor color = paletteColor(_series.size());
    series.setColor(color);
    _series.insert(nodeId, series);

    _chart->addSeries(nodeId, series.label(), color);

    emit nodeAdded(nodeId, displayName, displayPath);
    if (_mode == Mode::Live)
        subscribeNode(nodeId);
    else
        requestHistory(nodeId);
    return true;
}

void TrendGraphWidget::removeNode(const QString &nodeId)
{
    if (_series.remove(nodeId) == 0)
        return;
    _chart->removeSeries(nodeId);
    unsubscribeNode(nodeId);
    _pendingHistory.remove(nodeId);
    emit nodeRemoved(nodeId);
}

bool TrendGraphWidget::hasNode(const QString &nodeId) const
{
    return _series.contains(nodeId);
}

QStringList TrendGraphWidget::chartedNodeIds() const
{
    return _series.keys();
}

void TrendGraphWidget::applyLiveValues(const QVector<OpcUaDataValue> &values)
{
    if (_mode != Mode::Live)
        return;
    for (const OpcUaDataValue &value : values) {
        auto it = _series.find(value.nodeId);
        if (it == _series.end())
            continue;
        const int before = it->points().size();
        if (!it->appendLive(value))
            continue;
        if (it->points().size() == before)
            continue;
        const QPointF &point = it->points().constLast();
        _chart->appendPoint(value.nodeId, toChartX(point.x()), point.y());
    }
}

void TrendGraphWidget::applyHistory(const QString &nodeId,
                                    const QVector<OpcUaHistoryValue> &values)
{
    auto it = _series.find(nodeId);
    if (it == _series.end())
        return;
    it->setHistory(values);
    refeedSeries(*it);
}

void TrendGraphWidget::setTimeWindow(qreal startMsEpoch, qreal endMsEpoch)
{
    _chart->setTimeWindow(toChartX(startMsEpoch), toChartX(endMsEpoch));
}

void TrendGraphWidget::autoScale()
{
    _chart->autoScaleY();
}

void TrendGraphWidget::fit()
{
    _chart->fit();
}

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
    if (!_pendingHistory.contains(nodeId))
        return false;
    _pendingHistory.remove(nodeId);
    if (error.isEmpty() && hasNode(nodeId)) {
        applyHistory(nodeId, values);
        autoScale();
    }
    return true;
}

void TrendGraphWidget::clear()
{
    const QSet<QString> subscribed = _subscribed;
    for (const QString &nodeId : subscribed)
        unsubscribeNode(nodeId);
    _pendingHistory.clear();
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
/// \brief Switches to live streaming and subscribes the charted nodes.
///
void TrendGraphWidget::enterLiveMode()
{
    _mode = Mode::Live;
    _windowMs = kLiveWindowMs;
    _liveButton->setChecked(true);
    const QStringList ids = chartedNodeIds();
    for (const QString &nodeId : ids)
        subscribeNode(nodeId);
    applyWindow();
    if (!_liveTimer->isActive())
        _liveTimer->start();
}

///
/// \brief Switches to historical reads over a rolling window.
/// \param windowMs Window length in milliseconds.
///
void TrendGraphWidget::enterHistoryMode(qint64 windowMs)
{
    _mode = Mode::History;
    _windowMs = windowMs;
    switch (windowMs) {
    case 600000:   _tenMinutesButton->setChecked(true); break;
    case 3600000:  _oneHourButton->setChecked(true); break;
    case 86400000: _oneDayButton->setChecked(true); break;
    default:       _oneMinuteButton->setChecked(true); break;
    }
    _liveTimer->stop();
    const QSet<QString> subscribed = _subscribed;
    for (const QString &nodeId : subscribed)
        unsubscribeNode(nodeId);
    applyWindow();
    const QStringList ids = chartedNodeIds();
    for (const QString &nodeId : ids)
        requestHistory(nodeId);
}

///
/// \brief Applies the rolling time window to the chart.
///
void TrendGraphWidget::applyWindow()
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    setTimeWindow(static_cast<qreal>(now - _windowMs), static_cast<qreal>(now));
}

///
/// \brief Subscribes a node once, emitting the request on first reference.
/// \param nodeId Node to monitor.
///
void TrendGraphWidget::subscribeNode(const QString &nodeId)
{
    if (_subscribed.contains(nodeId))
        return;
    _subscribed.insert(nodeId);
    emit subscribeRequested(nodeId, kPublishingIntervalMs);
}

///
/// \brief Unsubscribes a node, emitting the request when it was monitored.
/// \param nodeId Node to stop monitoring.
///
void TrendGraphWidget::unsubscribeNode(const QString &nodeId)
{
    if (_subscribed.remove(nodeId))
        emit unsubscribeRequested(nodeId);
}

///
/// \brief Requests a history read for a node over the active window.
/// \param nodeId Node whose history is read.
///
void TrendGraphWidget::requestHistory(const QString &nodeId)
{
    const QDateTime end = QDateTime::currentDateTime();
    const QDateTime start = end.addMSecs(-_windowMs);
    _pendingHistory.insert(nodeId);
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
        QMessageBox::warning(this, tr("Export Trend"),
                             tr("Could not save '%1'.").arg(fileName));
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
/// \brief Pushes a series' buffered points into the chart with the active mode.
/// \param series Series whose points are re-fed.
///
void TrendGraphWidget::refeedSeries(const TrendSeries &series)
{
    const QVector<QPointF> &points = series.points();
    QVector<ChartPoint> mapped;
    mapped.reserve(points.size());
    for (const QPointF &point : points)
        mapped.append(ChartPoint{toChartX(point.x()), point.y()});
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
    theme.seriesPalette = kSeriesPalette;
    _chart->setTheme(theme);
}

///
/// \brief Reports whether a drag carries a droppable variable node.
/// \param mimeData Drag MIME data.
/// \return True when the drag holds a variable node.
///
bool TrendGraphWidget::acceptsNodeDrag(const QMimeData *mimeData) const
{
    OpcUaNodeInfo node;
    return AddressSpaceMime::decodeNode(mimeData, &node)
        && !node.nodeId.isEmpty() && OpcUa::isVariable(node.nodeClass);
}

///
/// \brief Adds a dropped variable node as a new series.
/// \param mimeData Drop MIME data.
/// \return True when a variable node was added.
///
bool TrendGraphWidget::dropNode(const QMimeData *mimeData)
{
    OpcUaNodeInfo node;
    if (!AddressSpaceMime::decodeNode(mimeData, &node)
        || node.nodeId.isEmpty() || !OpcUa::isVariable(node.nodeClass)) {
        return false;
    }
    const QString label = node.displayName.isEmpty()
        ? (node.browseName.isEmpty() ? node.nodeId : node.browseName)
        : node.displayName;
    addNode(node.nodeId, label, node.displayPath);
    return true;
}

///
/// \brief Shows the chart context menu: view actions plus node removal.
///
/// Auto Scale, Fit, and Settings are always offered; per-node Remove entries
/// and Remove All appear only while the chart has series.
///
/// \param globalPos Menu position in global screen coordinates.
///
void TrendGraphWidget::showSeriesContextMenu(const QPoint &globalPos)
{
    QMenu menu(this);

    menu.addAction(tr("Auto Scale"), this, [this]() { autoScale(); });
    menu.addAction(AppIcons::themed(QStringLiteral("fit")), tr("Fit"), this,
                   [this]() { fit(); });
    menu.addAction(AppIcons::themed(QStringLiteral("settings")), tr("Settings"),
                   this, &TrendGraphWidget::settingsRequested);

    if (!_series.isEmpty()) {
        menu.addSeparator();

        QList<TrendSeries> ordered = _series.values();
        std::sort(ordered.begin(), ordered.end(),
                  [](const TrendSeries &lhs, const TrendSeries &rhs) {
                      return lhs.label().localeAwareCompare(rhs.label()) < 0;
                  });

        for (const TrendSeries &series : std::as_const(ordered)) {
            const QString nodeId = series.nodeId();
            menu.addAction(swatchIcon(series.color()),
                           tr("Remove %1").arg(series.label()), this,
                           [this, nodeId]() { removeNode(nodeId); });
        }

        menu.addAction(AppIcons::themed(QStringLiteral("remove")), tr("Remove All"),
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
    }
}

///
/// \brief Routes drags over the chart view (and its viewport) to this widget.
///
/// The chart view is a scroll area that accepts drops itself, so drags landing on
/// it never reach the widget's own drag handlers; this filter intercepts them.
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
