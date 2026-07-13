// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file nodemonitordialog.cpp
/// \brief Implements the single-node live monitor dialog.
///

#include <QDateTime>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QHideEvent>
#include <QMenu>
#include <QMimeData>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QTimer>

#include "appcolors.h"
#include "appicons.h"
#include "charttypes.h"
#include "chartviewfactory.h"
#include "formatters/attributeformatter.h"
#include "ichartview.h"
#include "models/addressspacemimedata.h"
#include "nodemonitordialog.h"
#include "opcua/opcuabackend.h"
#include "ui_nodemonitordialog.h"
#include "widgets/subscriptioncombobox.h"
#include "widgets/themedpushbutton.h"

namespace {

constexpr qint64 kLiveWindowMs = 60000;
constexpr int kLiveTickMs = 1000;
constexpr int kMaxSamples = 4000;

const QColor kSeriesColor = QColor(0x00, 0xb4, 0x46);

///
/// \brief Classifies an OPC UA status string into a quality colour.
/// \param status Status display text, such as "Good" or "BadNodeIdUnknown".
/// \return Success, warning, error or subtitle colour matching the quality.
///
QColor qualityColor(const QString &status)
{
    if (status.startsWith(QLatin1String("Good")))
        return AppColors::statusSuccess();
    if (status.startsWith(QLatin1String("Uncertain")))
        return AppColors::statusWarning();
    if (status.startsWith(QLatin1String("Bad")))
        return AppColors::statusError();
    return AppColors::subtitleText();
}

} // namespace

///
/// \brief Builds the dialog and wires it to the backend.
/// \param service OPC UA backend used to subscribe and read the node.
/// \param parent Parent widget.
///
NodeMonitorDialog::NodeMonitorDialog(OpcUaBackend *service, QWidget *parent)
    : AppBaseDialog(parent)
    , ui(new Ui::NodeMonitorDialog)
    , _service(service)
    , _chart(ChartViewFactory::createChartView(this))
{
    ui->setupUi(this);
    setWindowFlag(Qt::WindowStaysOnTopHint, true);

    _series.setMaxPoints(kMaxSamples);

    QWidget *chartWidget = _chart->widget();
    ui->chartLayout->addWidget(chartWidget);
    chartWidget->installEventFilter(this);
    const QList<QWidget *> chartChildren = chartWidget->findChildren<QWidget *>();
    for (QWidget *child : chartChildren)
        child->installEventFilter(this);

    _settingsButton = new ThemedPushButton(ui->chartContainer);
    _settingsButton->setIconName(QStringLiteral("settings"));
    _settingsButton->setToolTip(tr("Chart settings"));
    _settingsButton->setCursor(Qt::PointingHandCursor);
    _settingsButton->setFixedSize(28, 28);
    _settingsButton->raise();
    ui->chartContainer->installEventFilter(this);

    ui->alwaysOnTopCheck->setChecked(true);
    ui->qualityLabel->setAttribute(Qt::WA_StyledBackground, true);
    ui->closeButton->setColors(
        { AppColors::accent(), AppColors::accentHover(), AppColors::accentPressed() });

    connect(ui->pauseButton, &QPushButton::toggled,
            this, &NodeMonitorDialog::setLivePaused);
    connect(ui->alwaysOnTopCheck, &QCheckBox::toggled,
            this, &NodeMonitorDialog::setAlwaysOnTop);
    connect(ui->subscriptionCombo, &SubscriptionComboBox::subscriptionSelected,
            this, [this](const QString &) { applySelectedSubscription(); });
    connect(ui->subscriptionCombo, &SubscriptionComboBox::subscriptionCreationRequested,
            this, &NodeMonitorDialog::subscriptionCreationRequested);
    connect(_settingsButton, &QPushButton::clicked,
            this, &NodeMonitorDialog::showSettingsMenu);
    connect(ui->closeButton, &QPushButton::clicked,
            this, &QDialog::close);

    connect(_service, &OpcUaBackend::dataValuesReady,
            this, &NodeMonitorDialog::handleDataValues);
    connect(_service, &OpcUaBackend::nodeDetailsReady,
            this, &NodeMonitorDialog::handleNodeDetails);

    _liveTimer = new QTimer(this);
    _liveTimer->setInterval(kLiveTickMs);
    connect(_liveTimer, &QTimer::timeout, this, [this]() {
        if (_livePaused || _nodeId.isEmpty())
            return;
        applyWindow();
        if (_autoScaleY)
            _chart->autoScaleY();
    });

    applyTheme();
    applyChartOptions();
    setAcceptDrops(true);
    showPlaceholder();
    applyWindow();
}

///
/// \brief Destroys the dialog and its generated UI.
///
NodeMonitorDialog::~NodeMonitorDialog()
{
    delete ui;
}

///
/// \brief Points the monitor at a variable node, replacing any previous target.
/// \param node Variable node to monitor.
///
void NodeMonitorDialog::setTarget(const OpcUaNodeInfo &node)
{
    if (node.nodeId.isEmpty() || !OpcUa::isVariable(node.nodeClass))
        return;

    ui->pauseButton->setChecked(false);
    unsubscribeCurrent();
    if (!_nodeId.isEmpty())
        _chart->removeSeries(_nodeId);

    _nodeId = node.nodeId;
    _displayName = node.displayName.isEmpty()
        ? (node.browseName.isEmpty() ? node.nodeId : node.browseName)
        : node.displayName;
    _displayPath = node.displayPath;
    _typeText.clear();
    _hasChartPoint = false;
    _lastChartY = 0.0;
    _series = TrendSeries(node.nodeId, _displayName, node.displayPath);
    _series.setMaxPoints(kMaxSamples);

    _chart->addSeries(_nodeId, _displayName, kSeriesColor);

    setWindowTitle(tr("Node Monitor — %1").arg(_displayName));
    ui->nameLabel->setText(_displayName);
    ui->nodeIdLabel->setText(tr("NodeId: %1").arg(_nodeId));
    ui->valueLabel->setText(QStringLiteral("—"));
    ui->typeLabel->clear();
    clearQualityBadge();
    ui->sourceTimeLabel->clear();
    ui->serverTimeLabel->clear();

    _service->readNode(_nodeId);
    subscribeCurrent();
    applyWindow();
}

///
/// \brief Returns the NodeId of the monitored node, or empty when none is set.
/// \return Monitored NodeId.
///
QString NodeMonitorDialog::nodeId() const
{
    return _nodeId;
}

///
/// \brief Subscribes to the current node's Value at the default interval.
///
void NodeMonitorDialog::subscribeCurrent()
{
    if (_nodeId.isEmpty() || _subscribed)
        return;
    _subscribed = true;
    _service->subscribe(_nodeId, _publishingInterval);
}

///
/// \brief Applies the selected subscription, re-negotiating an active subscription.
///
void NodeMonitorDialog::applySelectedSubscription()
{
    const double interval = ui->subscriptionCombo->currentInterval();
    if (qFuzzyCompare(interval, _publishingInterval))
        return;
    _publishingInterval = interval;
    if (_subscribed && !_nodeId.isEmpty())
        _service->subscribe(_nodeId, _publishingInterval);
}

///
/// \brief Populates the subscription combo from the application's subscriptions.
/// \param subscriptions Configured subscriptions, in display order.
///
void NodeMonitorDialog::setSubscriptions(const QVector<SubscriptionItem> &subscriptions)
{
    ui->subscriptionCombo->setSubscriptions(subscriptions);
    applySelectedSubscription();
}

///
/// \brief Captures the monitored node, settings and placement for the session.
/// \return Serializable snapshot of this monitor window.
///
SessionNodeMonitor NodeMonitorDialog::captureSession() const
{
    SessionNodeMonitor state;
    state.nodeId = _nodeId;
    state.displayName = _displayName;
    state.displayPath = _displayPath;
    state.subscriptionName = ui->subscriptionCombo->currentSubscription();
    state.typeText = _typeText;
    state.alwaysOnTop = ui->alwaysOnTopCheck->isChecked();
    state.autoScale = _autoScaleY;
    state.stepLines = _stepLines;
    state.showGrid = _showGrid;
    state.showLegend = _showLegend;
    state.showPoints = _showPoints;
    state.showValueTooltip = _showTooltip;
    state.geometry = geometry();
    return state;
}

///
/// \brief Restores a monitor's settings, placement and target from a session.
/// \param state Snapshot previously produced by captureSession().
///
void NodeMonitorDialog::restoreSession(const SessionNodeMonitor &state)
{
    _autoScaleY = state.autoScale;
    _stepLines = state.stepLines;
    _showGrid = state.showGrid;
    _showLegend = state.showLegend;
    _showPoints = state.showPoints;
    _showTooltip = state.showValueTooltip;
    applyChartOptions();

    {
        const QSignalBlocker blocker(ui->alwaysOnTopCheck);
        ui->alwaysOnTopCheck->setChecked(state.alwaysOnTop);
    }
    setWindowFlag(Qt::WindowStaysOnTopHint, state.alwaysOnTop);

    ui->subscriptionCombo->setCurrentSubscription(state.subscriptionName);
    _publishingInterval = ui->subscriptionCombo->currentInterval();

    if (state.geometry.isValid())
        setGeometry(state.geometry);

    if (!state.nodeId.isEmpty()) {
        OpcUaNodeInfo node;
        node.nodeId = state.nodeId;
        node.displayName = state.displayName;
        node.displayPath = state.displayPath;
        node.nodeClass = OpcUa::Variable;
        setTarget(node);

        if (!state.typeText.isEmpty()) {
            _typeText = state.typeText;
            ui->typeLabel->setText(_typeText);
        }
    }
}

///
/// \brief Stops monitoring the current node when it is being monitored.
///
void NodeMonitorDialog::unsubscribeCurrent()
{
    if (!_subscribed)
        return;
    _subscribed = false;
    _service->unsubscribe(_nodeId);
}

///
/// \brief Re-subscribes when the window becomes visible again.
/// \param event Show event.
///
void NodeMonitorDialog::showEvent(QShowEvent *event)
{
    AppBaseDialog::showEvent(event);
    positionSettingsButton();
    subscribeCurrent();
    if (!_liveTimer->isActive())
        _liveTimer->start();
}

///
/// \brief Stops monitoring while the window is hidden.
/// \param event Hide event.
///
void NodeMonitorDialog::hideEvent(QHideEvent *event)
{
    unsubscribeCurrent();
    _liveTimer->stop();
    AppBaseDialog::hideEvent(event);
}

///
/// \brief Applies streamed values that belong to the monitored node.
/// \param values Latest data-access values.
/// \param error Read or monitoring error, empty on success.
///
void NodeMonitorDialog::handleDataValues(const QVector<OpcUaDataValue> &values,
                                         const QString &error)
{
    Q_UNUSED(error);
    if (_nodeId.isEmpty())
        return;
    for (const OpcUaDataValue &value : values) {
        if (value.nodeId == _nodeId)
            applyValue(value);
    }
}

///
/// \brief Applies the resolved data type from a node read for the monitored node.
/// \param details Read node details.
/// \param error Read error, empty on success.
///
void NodeMonitorDialog::handleNodeDetails(const OpcUaNodeDetails &details, const QString &error)
{
    if (_nodeId.isEmpty() || details.nodeId != _nodeId)
        return;
    if (!error.isEmpty())
        return;

    _typeText = OpcUaFormat::dataTypeDisplay(details.dataTypeId);
    ui->typeLabel->setText(_typeText);

    OpcUaDataValue value;
    value.nodeId = details.nodeId;
    value.value = details.value;
    value.valueType = details.valueType;
    value.dataTypeId = details.dataTypeId;
    value.status = details.status;
    value.sourceTimestamp = details.sourceTimestamp;
    value.serverTimestamp = details.serverTimestamp;
    applyValue(value);
}

///
/// \brief Buffers a sample into the chart and, unless paused, updates the display.
///
/// Samples are always appended so the trend stays complete across a pause; while
/// paused the value, timestamps, quality and viewport stay frozen and the buffered
/// points become visible when live updates resume.
///
/// \param value Streamed or read data value for the monitored node.
///
void NodeMonitorDialog::applyValue(const OpcUaDataValue &value)
{
    const int before = _series.points().size();
    const bool appended = _series.appendLive(value) && _series.points().size() != before;
    if (appended) {
        const QPointF &point = _series.points().constLast();
        appendStep(point.x(), point.y(), value.status);
    }

    if (_livePaused)
        return;

    ui->valueLabel->setText(OpcUaFormat::displayValue(value.value));
    updateQualityBadge(value.status);

    ui->sourceTimeLabel->setText(OpcUaFormat::isoTimestampWithZone(value.sourceTimestamp));
    ui->serverTimeLabel->setText(OpcUaFormat::isoTimestampWithZone(value.serverTimestamp));

    if (_typeText.isEmpty() && !value.dataTypeId.isEmpty()) {
        _typeText = OpcUaFormat::dataTypeDisplay(value.dataTypeId);
        ui->typeLabel->setText(_typeText);
    }

    if (appended) {
        applyWindow();
        if (_autoScaleY)
            _chart->autoScaleY();
    }
}

///
/// \brief Appends a sample as a hold-last-value step (flat, then a vertical jump).
///
/// OPC UA monitored items report on value change, so the value is constant between
/// samples; drawing a horizontal segment to the new time and then a vertical step
/// renders that faithfully instead of a sloped line between sparse samples.
///
/// \param x Sample time in milliseconds since the epoch.
/// \param y Sample value.
/// \param status OPC UA status text for the sample.
///
void NodeMonitorDialog::appendStep(qreal x, qreal y, const QString &status)
{
    if (_stepLines && _hasChartPoint)
        _chart->appendPoint(_nodeId, x, _lastChartY, status);
    _chart->appendPoint(_nodeId, x, y, status);
    _lastChartY = y;
    _hasChartPoint = true;
}

///
/// \brief Rebuilds the chart from the buffer honouring the current step-line mode.
///
void NodeMonitorDialog::refeedChart()
{
    const QVector<QPointF> &points = _series.points();
    const QVector<QString> &statuses = _series.statuses();

    QVector<ChartPoint> mapped;
    mapped.reserve(points.size() * (_stepLines ? 2 : 1));
    for (int i = 0; i < points.size(); ++i) {
        const QString status = i < statuses.size() ? statuses.at(i) : QString();
        if (_stepLines && i > 0)
            mapped.append(ChartPoint{ points.at(i).x(), points.at(i - 1).y(), status });
        mapped.append(ChartPoint{ points.at(i).x(), points.at(i).y(), status });
    }
    _chart->setPoints(_nodeId, mapped);

    _hasChartPoint = !points.isEmpty();
    if (_hasChartPoint)
        _lastChartY = points.constLast().y();
}

///
/// \brief Pushes the current display toggles to the chart.
///
void NodeMonitorDialog::applyChartOptions()
{
    _chart->setGridVisible(_showGrid);
    _chart->setLegendVisible(_showLegend);
    _chart->setPointsVisible(_showPoints);
    _chart->setHoverValueVisible(_showTooltip);
    _chart->setSmoothLines(false);
}

///
/// \brief Shows the chart display-options menu under the settings button.
///
void NodeMonitorDialog::showSettingsMenu()
{
    QMenu menu(this);

    QAction *autoScale = menu.addAction(tr("Auto Scale"));
    autoScale->setCheckable(true);
    autoScale->setChecked(_autoScaleY);
    connect(autoScale, &QAction::toggled, this, [this](bool on) {
        _autoScaleY = on;
        if (on && !_livePaused)
            _chart->autoScaleY();
    });

    QAction *stepLines = menu.addAction(tr("Step Lines"));
    stepLines->setCheckable(true);
    stepLines->setChecked(_stepLines);
    connect(stepLines, &QAction::toggled, this, [this](bool on) {
        _stepLines = on;
        refeedChart();
        if (!_livePaused && _autoScaleY)
            _chart->autoScaleY();
    });

    menu.addSeparator();

    QAction *grid = menu.addAction(tr("Show Grid"));
    grid->setCheckable(true);
    grid->setChecked(_showGrid);
    connect(grid, &QAction::toggled, this, [this](bool on) {
        _showGrid = on;
        _chart->setGridVisible(on);
    });

    QAction *legend = menu.addAction(tr("Show Legend"));
    legend->setCheckable(true);
    legend->setChecked(_showLegend);
    connect(legend, &QAction::toggled, this, [this](bool on) {
        _showLegend = on;
        _chart->setLegendVisible(on);
    });

    QAction *points = menu.addAction(tr("Show Points"));
    points->setCheckable(true);
    points->setChecked(_showPoints);
    connect(points, &QAction::toggled, this, [this](bool on) {
        _showPoints = on;
        _chart->setPointsVisible(on);
    });

    QAction *tooltip = menu.addAction(tr("Show Value Tooltip"));
    tooltip->setCheckable(true);
    tooltip->setChecked(_showTooltip);
    connect(tooltip, &QAction::toggled, this, [this](bool on) {
        _showTooltip = on;
        _chart->setHoverValueVisible(on);
    });

    menu.exec(_settingsButton->mapToGlobal(QPoint(0, _settingsButton->height())));
}

///
/// \brief Keeps the settings button pinned to the chart's top-right corner.
///
void NodeMonitorDialog::positionSettingsButton()
{
    if (!_settingsButton)
        return;
    const int margin = 8;
    _settingsButton->move(ui->chartContainer->width() - _settingsButton->width() - margin, margin);
    _settingsButton->raise();
}

///
/// \brief Shows the status code as a coloured pill badge matching its quality.
/// \param status Status display text, or empty to clear the badge.
///
void NodeMonitorDialog::updateQualityBadge(const QString &status)
{
    if (status.isEmpty()) {
        clearQualityBadge();
        return;
    }

    const QColor foreground = qualityColor(status);
    QColor background = foreground;
    background.setAlpha(38);

    ui->qualityLabel->setText(status);
    ui->qualityLabel->setStyleSheet(QStringLiteral(
        "QLabel { background-color: %1; color: %2;"
        " border-radius: 9px; padding: 2px 12px; font-weight: 600; }")
        .arg(AppColors::toCss(background), AppColors::toCss(foreground)));
}

///
/// \brief Clears the status badge text and styling.
///
void NodeMonitorDialog::clearQualityBadge()
{
    ui->qualityLabel->clear();
    ui->qualityLabel->setStyleSheet(QString());
}

///
/// \brief Applies the rolling one-minute window ending at the current time.
///
void NodeMonitorDialog::applyWindow()
{
    const qint64 end = QDateTime::currentMSecsSinceEpoch();
    _chart->setTimeWindow(static_cast<qreal>(end - _windowMs), static_cast<qreal>(end));
}

///
/// \brief Freezes or resumes the live view while samples keep buffering.
///
/// While paused the value, timestamps, quality and viewport stay frozen but incoming
/// samples are still recorded; resuming snaps the window back to now and reveals the
/// full trend collected during the pause, without gaps.
///
/// \param paused True to freeze the live view, false to resume.
///
void NodeMonitorDialog::setLivePaused(bool paused)
{
    _livePaused = paused;
    ui->pauseButton->setText(paused ? tr("Resume") : tr("Pause"));
    ui->pauseButton->setIconName(paused ? QStringLiteral("resume") : QStringLiteral("pause"));
    if (!paused) {
        applyWindow();
        if (_autoScaleY)
            _chart->autoScaleY();
    }
}

///
/// \brief Toggles the always-on-top window hint.
/// \param onTop True to keep the window above others.
///
void NodeMonitorDialog::setAlwaysOnTop(bool onTop)
{
    if (windowFlags().testFlag(Qt::WindowStaysOnTopHint) == onTop)
        return;
    setWindowFlag(Qt::WindowStaysOnTopHint, onTop);
    show();
}

///
/// \brief Resets the header to the empty prompt shown before a node is targeted.
///
void NodeMonitorDialog::showPlaceholder()
{
    ui->nameLabel->setText(tr("No node"));
    ui->nodeIdLabel->setText(tr("Drag a variable node here to monitor it"));
    ui->valueLabel->setText(QStringLiteral("—"));
    ui->typeLabel->clear();
    clearQualityBadge();
    ui->sourceTimeLabel->clear();
    ui->serverTimeLabel->clear();
}

///
/// \brief Builds a chart theme from AppColors and applies it.
///
void NodeMonitorDialog::applyTheme()
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
    theme.seriesPalette = { kSeriesColor };
    _chart->setTheme(theme);

    ui->typeLabel->setStyleSheet(
        QStringLiteral("color: %1;").arg(AppColors::toCss(AppColors::titleText())));
}

///
/// \brief Re-applies the chart theme after the palette has switched.
/// \param event Change event.
///
void NodeMonitorDialog::changeEvent(QEvent *event)
{
    AppBaseDialog::changeEvent(event);
    if (event->type() == QEvent::PaletteChange
        || event->type() == QEvent::ApplicationPaletteChange) {
        applyTheme();
    }
}

///
/// \brief Reports whether a drag carries a droppable variable node.
/// \param mimeData Drag MIME data.
/// \return True when the drag holds a variable node.
///
bool NodeMonitorDialog::acceptsNodeDrag(const QMimeData *mimeData) const
{
    OpcUaNodeInfo node;
    return AddressSpaceMime::decodeNode(mimeData, &node)
        && !node.nodeId.isEmpty() && OpcUa::isVariable(node.nodeClass);
}

///
/// \brief Targets the monitor at a dropped variable node.
/// \param mimeData Drop MIME data.
/// \return True when a variable node was accepted.
///
bool NodeMonitorDialog::dropNode(const QMimeData *mimeData)
{
    OpcUaNodeInfo node;
    if (!AddressSpaceMime::decodeNode(mimeData, &node)
        || node.nodeId.isEmpty() || !OpcUa::isVariable(node.nodeClass)) {
        return false;
    }
    setTarget(node);
    return true;
}

///
/// \brief Routes drags over the chart view (and its viewport) to this widget.
/// \param watched Watched object.
/// \param event Delivered event.
/// \return True when the drag or drop event was handled here.
///
bool NodeMonitorDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->chartContainer && event->type() == QEvent::Resize)
        positionSettingsButton();

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
    default:
        break;
    }
    return AppBaseDialog::eventFilter(watched, event);
}

///
/// \brief Accepts a drag that carries a droppable variable node.
/// \param event Drag-enter event.
///
void NodeMonitorDialog::dragEnterEvent(QDragEnterEvent *event)
{
    dragMoveEvent(event);
}

///
/// \brief Accepts the drag as a copy while it carries a variable node.
/// \param event Drag-move event.
///
void NodeMonitorDialog::dragMoveEvent(QDragMoveEvent *event)
{
    if (acceptsNodeDrag(event->mimeData())) {
        event->setDropAction(Qt::CopyAction);
        event->accept();
    } else {
        event->ignore();
    }
}

///
/// \brief Targets the monitor at the dropped variable node.
/// \param event Drop event.
///
void NodeMonitorDialog::dropEvent(QDropEvent *event)
{
    if (dropNode(event->mimeData())) {
        event->setDropAction(Qt::CopyAction);
        event->accept();
    } else {
        event->ignore();
    }
}
