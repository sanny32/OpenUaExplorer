// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file trendpanelwidget.cpp
/// \brief Implements the trend panel widget.
///

#include "trendpanelwidget.h"
#include "ui_trendpanelwidget.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QDataStream>
#include <QDateTime>
#include <QFileDialog>
#include <QImage>
#include <QMessageBox>
#include <QTimer>

#include "trendgraphwidget.h"

namespace {

constexpr qint64 kLiveWindowMs = 60000;
constexpr int kLiveTickMs = 1000;
constexpr double kPublishingIntervalMs = 1000.0;
const QString kModeStateKey = QStringLiteral("trendPanelState");

}

///
/// \brief Builds the trend panel, its first chart tab, and toolbar wiring.
/// \param parent Parent widget.
///
TrendPanelWidget::TrendPanelWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TrendPanelWidget)
{
    ui->setupUi(this);

    _liveTimer = new QTimer(this);
    _liveTimer->setInterval(kLiveTickMs);
    connect(_liveTimer, &QTimer::timeout, this, [this]() {
        applyWindow();
        for (TrendGraphWidget *chart : charts())
            chart->autoScale();
    });

    _addTab = new QWidget(ui->trendTabs);
    ui->trendTabs->addTab(_addTab, QStringLiteral("+"));

    configureToolbar();
    addChartTab();

    connect(ui->trendTabs, &QTabWidget::currentChanged, this,
            &TrendPanelWidget::handleTabChanged);

    enterLiveMode();
}

///
/// \brief Destroys the panel and its generated UI.
///
TrendPanelWidget::~TrendPanelWidget()
{
    delete ui;
}

///
/// \brief Wires the toolbar buttons, their icons, and the mode button group.
///
void TrendPanelWidget::configureToolbar()
{
    ui->fitButton->setIcon(QStringLiteral("fit"));
    ui->exportButton->setIcon(QStringLiteral("export"));
    ui->settingsButton->setIcon(QStringLiteral("settings"));

    _modeGroup = new QButtonGroup(this);
    _modeGroup->setExclusive(true);
    _modeGroup->addButton(ui->liveButton);
    _modeGroup->addButton(ui->oneMinuteButton);
    _modeGroup->addButton(ui->tenMinutesButton);
    _modeGroup->addButton(ui->oneHourButton);
    _modeGroup->addButton(ui->oneDayButton);

    connect(ui->liveButton, &QAbstractButton::clicked, this,
            [this]() { enterLiveMode(); });
    connect(ui->oneMinuteButton, &QAbstractButton::clicked, this,
            [this]() { enterHistoryMode(60000); });
    connect(ui->tenMinutesButton, &QAbstractButton::clicked, this,
            [this]() { enterHistoryMode(600000); });
    connect(ui->oneHourButton, &QAbstractButton::clicked, this,
            [this]() { enterHistoryMode(3600000); });
    connect(ui->oneDayButton, &QAbstractButton::clicked, this,
            [this]() { enterHistoryMode(86400000); });

    connect(ui->autoScaleButton, &QAbstractButton::clicked, this, [this]() {
        if (TrendGraphWidget *chart = currentChart())
            chart->autoScale();
    });
    connect(ui->fitButton, &QAbstractButton::clicked, this, [this]() {
        if (TrendGraphWidget *chart = currentChart())
            chart->fit();
    });
    connect(ui->exportButton, &QAbstractButton::clicked, this,
            &TrendPanelWidget::exportCurrentChart);
}

///
/// \brief Adds a chart tab before the trailing add-tab and selects it.
/// \return The new chart widget.
///
TrendGraphWidget *TrendPanelWidget::addChartTab()
{
    auto *chart = new TrendGraphWidget(ui->trendTabs);
    connect(chart, &TrendGraphWidget::nodeAdded, this,
            [this](const QString &nodeId) { onNodeAdded(nodeId); });
    connect(chart, &TrendGraphWidget::nodeRemoved, this,
            [this](const QString &nodeId) { onNodeRemoved(nodeId); });

    const int insertAt = ui->trendTabs->indexOf(_addTab);
    const QString title = QStringLiteral("Trend %1").arg(++_chartCounter);
    ui->trendTabs->insertTab(insertAt < 0 ? ui->trendTabs->count() : insertAt, chart, title);
    ui->trendTabs->setCurrentWidget(chart);
    return chart;
}

///
/// \brief Creates a new chart when the add-tab is chosen.
/// \param index Newly selected tab index.
///
void TrendPanelWidget::handleTabChanged(int index)
{
    if (ui->trendTabs->widget(index) == _addTab)
        addChartTab();
}

///
/// \brief Returns the chart in the active tab, or nullptr for the add-tab.
/// \return Active chart widget or nullptr.
///
TrendGraphWidget *TrendPanelWidget::currentChart() const
{
    return qobject_cast<TrendGraphWidget *>(ui->trendTabs->currentWidget());
}

///
/// \brief Collects every chart tab.
/// \return Chart widgets in tab order.
///
QList<TrendGraphWidget *> TrendPanelWidget::charts() const
{
    QList<TrendGraphWidget *> result;
    for (int i = 0; i < ui->trendTabs->count(); ++i) {
        if (auto *chart = qobject_cast<TrendGraphWidget *>(ui->trendTabs->widget(i)))
            result.append(chart);
    }
    return result;
}

///
/// \brief Collects the NodeIds charted across all tabs.
/// \return Charted NodeIds.
///
QStringList TrendPanelWidget::allNodeIds() const
{
    QStringList ids;
    for (TrendGraphWidget *chart : charts())
        ids += chart->chartedNodeIds();
    return ids;
}

///
/// \brief Adds a node series to the active chart.
/// \param details Variable node details.
///
void TrendPanelWidget::addNode(const OpcUaNodeDetails &details)
{
    addNode(details.nodeId, details.displayName);
}

///
/// \brief Adds a node series to the active chart.
/// \param nodeId Node NodeId.
/// \param displayName Human-readable node name.
/// \param displayPath Human-readable node path.
///
void TrendPanelWidget::addNode(const QString &nodeId, const QString &displayName,
                               const QString &displayPath)
{
    if (nodeId.isEmpty())
        return;
    TrendGraphWidget *chart = currentChart();
    if (!chart)
        chart = addChartTab();
    if (chart->hasNode(nodeId))
        return;
    chart->addNode(nodeId, displayName, displayPath);
}

///
/// \brief Switches to live streaming and subscribes the charted nodes.
///
void TrendPanelWidget::enterLiveMode()
{
    _mode = Mode::Live;
    _windowMs = kLiveWindowMs;
    ui->liveButton->setChecked(true);
    reconcileSubscriptions();
    applyWindow();
    if (!_liveTimer->isActive())
        _liveTimer->start();
}

///
/// \brief Switches to historical reads over a rolling window.
/// \param windowMs Window length in milliseconds.
///
void TrendPanelWidget::enterHistoryMode(qint64 windowMs)
{
    _mode = Mode::History;
    _windowMs = windowMs;
    _liveTimer->stop();
    reconcileSubscriptions();
    applyWindow();
    const QStringList ids = allNodeIds();
    for (const QString &nodeId : ids)
        requestHistory(nodeId);
}

///
/// \brief Brings live subscriptions in line with the active mode.
///
void TrendPanelWidget::reconcileSubscriptions()
{
    QSet<QString> desired;
    if (_mode == Mode::Live) {
        const QStringList ids = allNodeIds();
        desired = QSet<QString>(ids.cbegin(), ids.cend());
    }

    const QSet<QString> current = _subscribed;
    for (const QString &nodeId : current) {
        if (!desired.contains(nodeId))
            unsubscribeNode(nodeId);
    }
    for (const QString &nodeId : desired) {
        if (!_subscribed.contains(nodeId))
            subscribeNode(nodeId);
    }
}

///
/// \brief Applies the rolling time window to every chart.
///
void TrendPanelWidget::applyWindow()
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qreal start = static_cast<qreal>(now - _windowMs);
    const qreal end = static_cast<qreal>(now);
    for (TrendGraphWidget *chart : charts())
        chart->setTimeWindow(start, end);
}

///
/// \brief Reacts to a node added to a chart by subscribing or reading history.
/// \param nodeId Added node NodeId.
///
void TrendPanelWidget::onNodeAdded(const QString &nodeId)
{
    if (_restoring)
        return;
    if (_mode == Mode::Live)
        subscribeNode(nodeId);
    else
        requestHistory(nodeId);
}

///
/// \brief Reacts to a node removed from a chart.
/// \param nodeId Removed node NodeId.
///
void TrendPanelWidget::onNodeRemoved(const QString &nodeId)
{
    if (!allNodeIds().contains(nodeId))
        unsubscribeNode(nodeId);
    _pendingHistory.remove(nodeId);
}

///
/// \brief Subscribes a node once, emitting the request on first reference.
/// \param nodeId Node to monitor.
///
void TrendPanelWidget::subscribeNode(const QString &nodeId)
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
void TrendPanelWidget::unsubscribeNode(const QString &nodeId)
{
    if (_subscribed.remove(nodeId))
        emit unsubscribeRequested(nodeId);
}

///
/// \brief Requests a history read for a node over the active window.
/// \param nodeId Node whose history is read.
///
void TrendPanelWidget::requestHistory(const QString &nodeId)
{
    const QDateTime end = QDateTime::currentDateTime();
    const QDateTime start = end.addMSecs(-_windowMs);
    _pendingHistory.insert(nodeId, true);
    emit historyReadRequested(nodeId, start, end, 0);
}

///
/// \brief Applies history results if the panel requested them for the node.
/// \param nodeId Node whose history arrived.
/// \param error Read error, empty on success.
/// \param values History samples in time order.
/// \return True when the panel had requested this node's history.
///
bool TrendPanelWidget::consumeHistory(const QString &nodeId, const QString &error,
                                      const QVector<OpcUaHistoryValue> &values)
{
    if (!_pendingHistory.contains(nodeId))
        return false;
    _pendingHistory.remove(nodeId);
    if (error.isEmpty()) {
        for (TrendGraphWidget *chart : charts()) {
            if (chart->hasNode(nodeId)) {
                chart->applyHistory(nodeId, values);
                chart->autoScale();
            }
        }
    }
    return true;
}

///
/// \brief Forwards streamed values to the charts.
/// \param values Latest data-access values.
///
void TrendPanelWidget::applyLiveValues(const QVector<OpcUaDataValue> &values)
{
    for (TrendGraphWidget *chart : charts())
        chart->applyLiveValues(values);
}

///
/// \brief Applies the timestamp display mode to the charts.
/// \param mode Local time or UTC.
///
void TrendPanelWidget::setTimestampMode(AppSettings::TimestampMode mode)
{
    for (TrendGraphWidget *chart : charts())
        chart->setTimestampMode(mode);
    applyWindow();
}

///
/// \brief Clears every chart, stops streaming, and forgets pending reads.
///
void TrendPanelWidget::clearRuntimeData()
{
    const QSet<QString> current = _subscribed;
    for (const QString &nodeId : current)
        unsubscribeNode(nodeId);
    _pendingHistory.clear();
    for (TrendGraphWidget *chart : charts())
        chart->clear();
}

///
/// \brief Persists the active mode.
/// \param settings Settings store to write to.
///
void TrendPanelWidget::saveViewState(AppSettings &settings) const
{
    QByteArray state;
    QDataStream stream(&state, QIODevice::WriteOnly);
    stream << static_cast<int>(_mode) << static_cast<qint64>(_windowMs);
    settings.setViewState(kModeStateKey, state);
}

///
/// \brief Restores the active mode.
/// \param settings Settings store to read from.
///
void TrendPanelWidget::restoreViewState(AppSettings &settings)
{
    const QByteArray state = settings.viewState(kModeStateKey);
    if (state.isEmpty())
        return;

    QDataStream stream(state);
    int mode = static_cast<int>(Mode::Live);
    qint64 windowMs = kLiveWindowMs;
    stream >> mode >> windowMs;
    if (stream.status() != QDataStream::Ok)
        return;

    _restoring = true;
    if (static_cast<Mode>(mode) == Mode::History) {
        switch (windowMs) {
        case 600000:   ui->tenMinutesButton->setChecked(true); break;
        case 3600000:  ui->oneHourButton->setChecked(true); break;
        case 86400000: ui->oneDayButton->setChecked(true); break;
        default:       ui->oneMinuteButton->setChecked(true); break;
        }
        enterHistoryMode(windowMs);
    } else {
        enterLiveMode();
    }
    _restoring = false;
}

///
/// \brief Saves the active chart as a PNG image.
///
void TrendPanelWidget::exportCurrentChart()
{
    TrendGraphWidget *chart = currentChart();
    if (!chart)
        return;

    const QString fileName = QFileDialog::getSaveFileName(
        this, tr("Export Trend"), QStringLiteral("trend.png"),
        tr("PNG Image (*.png);;All Files (*)"));
    if (fileName.isEmpty())
        return;

    const QImage image = chart->renderToImage(chart->size() * 2);
    if (!image.save(fileName)) {
        QMessageBox::warning(this, tr("Export Trend"),
                             tr("Could not save '%1'.").arg(fileName));
    }
}
