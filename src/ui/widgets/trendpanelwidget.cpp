// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file trendpanelwidget.cpp
/// \brief Implements the trend panel widget.
///

#include "trendpanelwidget.h"
#include "ui_trendpanelwidget.h"

#include <QAbstractButton>
#include <QDataStream>
#include <QIODevice>
#include <QTabBar>

#include "themedtoolbutton.h"
#include "trendgraphwidget.h"

namespace {

const QString kModeStateKey = QStringLiteral("trendPanelState");

}

///
/// \brief Builds the trend panel, its first chart tab, and tab-strip wiring.
/// \param parent Parent widget.
///
TrendPanelWidget::TrendPanelWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TrendPanelWidget)
{
    ui->setupUi(this);

    _expandedMinHeight = minimumHeight();

    ui->trendTabs->setTabsClosable(true);

    _collapseButton = new ThemedToolButton(ui->trendTabs);
    _collapseButton->setObjectName(QStringLiteral("collapseButton"));
    _collapseButton->setSquareIconOnly(true);
    _collapseButton->setAutoRaise(true);
    _collapseButton->setIcon(QStringLiteral("chevron-down"));
    _collapseButton->setToolTip(tr("Collapse trend panel"));
    ui->trendTabs->setCornerWidget(_collapseButton, Qt::TopRightCorner);
    connect(_collapseButton, &QAbstractButton::clicked, this,
            [this]() { setCollapsed(!_collapsed); });

    _addTab = new QWidget(ui->trendTabs);
    ui->trendTabs->addTab(_addTab, QStringLiteral("+"));
    if (auto *bar = ui->trendTabs->findChild<QTabBar *>()) {
        const int addIndex = ui->trendTabs->indexOf(_addTab);
        bar->setTabButton(addIndex, QTabBar::RightSide, nullptr);
        bar->setTabButton(addIndex, QTabBar::LeftSide, nullptr);
    }

    addChartTab();

    connect(ui->trendTabs, &QTabWidget::currentChanged, this,
            &TrendPanelWidget::handleTabChanged);
    connect(ui->trendTabs, &QTabWidget::tabCloseRequested, this,
            &TrendPanelWidget::handleTabCloseRequested);
}

///
/// \brief Destroys the panel and its generated UI.
///
TrendPanelWidget::~TrendPanelWidget()
{
    delete ui;
}

///
/// \brief Adds a chart tab before the trailing add-tab and selects it.
/// \return The new chart widget.
///
TrendGraphWidget *TrendPanelWidget::addChartTab()
{
    auto *chart = new TrendGraphWidget(ui->trendTabs);
    chart->setTimestampMode(_timestampMode);
    connect(chart, &TrendGraphWidget::subscribeRequested, this,
            &TrendPanelWidget::onChartSubscribe);
    connect(chart, &TrendGraphWidget::unsubscribeRequested, this,
            &TrendPanelWidget::onChartUnsubscribe);
    connect(chart, &TrendGraphWidget::historyReadRequested, this,
            &TrendPanelWidget::historyReadRequested);

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
    if (_suppressTabChange)
        return;
    if (ui->trendTabs->widget(index) == _addTab)
        addChartTab();
}

///
/// \brief Closes a chart tab, recreating a fresh tab when the last one closes.
/// \param index Tab to close.
///
void TrendPanelWidget::handleTabCloseRequested(int index)
{
    auto *chart = qobject_cast<TrendGraphWidget *>(ui->trendTabs->widget(index));
    if (!chart)
        return;

    const int chartCount = charts().size();

    _suppressTabChange = true;
    chart->clear();
    ui->trendTabs->removeTab(index);
    chart->deleteLater();
    _suppressTabChange = false;

    if (chartCount <= 1) {
        _chartCounter = 0;
        addChartTab();
    } else if (ui->trendTabs->currentWidget() == _addTab) {
        const int addIndex = ui->trendTabs->indexOf(_addTab);
        if (addIndex > 0)
            ui->trendTabs->setCurrentIndex(addIndex - 1);
    }
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
    setCollapsed(false);
    TrendGraphWidget *chart = currentChart();
    if (!chart)
        chart = addChartTab();
    if (chart->hasNode(nodeId))
        return;
    chart->addNode(nodeId, displayName, displayPath);
}

///
/// \brief Ref-counts a chart's subscribe request, monitoring on first reference.
/// \param nodeId Node a chart wants monitored.
/// \param publishingInterval Publishing interval in milliseconds.
///
void TrendPanelWidget::onChartSubscribe(const QString &nodeId, double publishingInterval)
{
    if (++_subscriptionRefs[nodeId] == 1)
        emit subscribeRequested(nodeId, publishingInterval);
}

///
/// \brief Ref-counts a chart's unsubscribe request, stopping on the last reference.
/// \param nodeId Node a chart no longer wants monitored.
///
void TrendPanelWidget::onChartUnsubscribe(const QString &nodeId)
{
    auto it = _subscriptionRefs.find(nodeId);
    if (it == _subscriptionRefs.end())
        return;
    if (--it.value() <= 0) {
        _subscriptionRefs.erase(it);
        emit unsubscribeRequested(nodeId);
    }
}

///
/// \brief Applies history results to whichever chart requested them.
/// \param nodeId Node whose history arrived.
/// \param error Read error, empty on success.
/// \param values History samples in time order.
/// \return True when a chart had requested this node's history.
///
bool TrendPanelWidget::consumeHistory(const QString &nodeId, const QString &error,
                                      const QVector<OpcUaHistoryValue> &values)
{
    bool handled = false;
    for (TrendGraphWidget *chart : charts()) {
        if (chart->consumeHistory(nodeId, error, values))
            handled = true;
    }
    return handled;
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
    _timestampMode = mode;
    for (TrendGraphWidget *chart : charts())
        chart->setTimestampMode(mode);
}

///
/// \brief Reports whether the panel is collapsed to its tab strip.
/// \return True when only the tab strip is visible.
///
bool TrendPanelWidget::isCollapsed() const
{
    return _collapsed;
}

///
/// \brief Collapses the panel to its tab strip or restores its full height.
/// \param collapsed True to collapse, false to restore.
///
void TrendPanelWidget::setCollapsed(bool collapsed)
{
    if (_collapsed == collapsed)
        return;
    _collapsed = collapsed;

    if (_collapsed) {
        const int header = collapsedHeight();
        setMinimumHeight(header);
        setMaximumHeight(header);
    } else {
        setMinimumHeight(_expandedMinHeight);
        setMaximumHeight(QWIDGETSIZE_MAX);
    }

    _collapseButton->setIcon(_collapsed ? QStringLiteral("chevron-up")
                                        : QStringLiteral("chevron-down"));
    _collapseButton->setToolTip(_collapsed ? tr("Expand trend panel")
                                           : tr("Collapse trend panel"));
}

///
/// \brief Computes the panel height that keeps only the tab strip visible.
/// \return Collapsed panel height in pixels.
///
int TrendPanelWidget::collapsedHeight() const
{
    int header = ui->trendTabs->tabBar()->sizeHint().height();
    if (_collapseButton)
        header = qMax(header, _collapseButton->sizeHint().height());
    return header;
}

///
/// \brief Clears every chart, stops streaming, and forgets pending reads.
///
void TrendPanelWidget::clearRuntimeData()
{
    for (TrendGraphWidget *chart : charts())
        chart->clear();
    _subscriptionRefs.clear();
}

///
/// \brief Persists the active chart's mode.
/// \param settings Settings store to write to.
///
void TrendPanelWidget::saveViewState(AppSettings &settings) const
{
    TrendGraphWidget *chart = currentChart();
    if (!chart)
        return;
    QByteArray state;
    QDataStream stream(&state, QIODevice::WriteOnly);
    stream << chart->modeState() << chart->windowState() << _collapsed;
    settings.setViewState(kModeStateKey, state);
}

///
/// \brief Restores the active chart's mode.
/// \param settings Settings store to read from.
///
void TrendPanelWidget::restoreViewState(AppSettings &settings)
{
    const QByteArray state = settings.viewState(kModeStateKey);
    if (state.isEmpty())
        return;

    QDataStream stream(state);
    int mode = 0;
    qint64 windowMs = 60000;
    stream >> mode >> windowMs;
    if (stream.status() != QDataStream::Ok)
        return;

    if (TrendGraphWidget *chart = currentChart())
        chart->applyModeState(mode, windowMs);

    bool collapsed = false;
    stream >> collapsed;
    if (stream.status() == QDataStream::Ok)
        setCollapsed(collapsed);
}
