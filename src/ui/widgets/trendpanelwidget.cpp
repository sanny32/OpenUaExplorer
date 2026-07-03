// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file trendpanelwidget.cpp
/// \brief Implements the trend panel widget.
///

#include "trendpanelwidget.h"
#include "ui_trendpanelwidget.h"

#include <algorithm>
#include <limits>

#include <QAbstractButton>
#include <QDataStream>
#include <QEvent>
#include <QIODevice>
#include <QResizeEvent>
#include <QTabBar>

#include "themedtoolbutton.h"
#include "trendgraphwidget.h"

namespace {

const QString kModeStateKey = QStringLiteral("trendPanelState");

///
/// \brief Returns the fastest publishing interval any chart asked for a node.
/// \param subscribers Charts monitoring the node, each with its requested interval.
/// \return Smallest requested interval in milliseconds.
///
double fastestInterval(const QHash<TrendGraphWidget *, double> &subscribers)
{
    double interval = std::numeric_limits<double>::max();
    for (double requested : subscribers)
        interval = std::min(interval, requested);
    return interval;
}

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

    // The collapse button is positioned manually over the tab strip rather than
    // via QTabWidget::setCornerWidget: QlementineStyle places corner widgets one
    // row too low (below the tab bar instead of centred within it).
    _collapseButton = new ThemedToolButton(ui->trendTabs);
    _collapseButton->setObjectName(QStringLiteral("collapseButton"));
    _collapseButton->setSquareIconOnly(true);
    _collapseButton->setAutoRaise(true);
    _collapseButton->setIcon(QStringLiteral("chevron-down"));
    _collapseButton->setToolTip(tr("Collapse trend panel"));
    ui->trendTabs->installEventFilter(this);
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
    chart->setSubscriptions(_subscriptions);
    connect(chart, &TrendGraphWidget::subscribeRequested, this,
            &TrendPanelWidget::onChartSubscribe);
    connect(chart, &TrendGraphWidget::unsubscribeRequested, this,
            &TrendPanelWidget::onChartUnsubscribe);
    connect(chart, &TrendGraphWidget::historyReadRequested, this,
            &TrendPanelWidget::historyReadRequested);
    connect(chart, &TrendGraphWidget::subscriptionCreationRequested, this,
            &TrendPanelWidget::subscriptionCreationRequested);

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
/// \brief Records a chart's requested interval, monitoring at the fastest one.
///
/// A node charted from several tabs is monitored once. Each chart may ask for a
/// different publishing interval, so the panel tracks every requester and forwards
/// the fastest interval, re-requesting whenever that effective interval changes.
///
/// \param nodeId Node a chart wants monitored.
/// \param publishingInterval Publishing interval the chart requests, in milliseconds.
///
void TrendPanelWidget::onChartSubscribe(const QString &nodeId, double publishingInterval)
{
    auto *chart = qobject_cast<TrendGraphWidget *>(sender());
    if (!chart)
        return;

    QHash<TrendGraphWidget *, double> &subscribers = _nodeSubscribers[nodeId];
    const bool firstSubscriber = subscribers.isEmpty();
    const double previousInterval = firstSubscriber ? 0.0 : fastestInterval(subscribers);
    subscribers.insert(chart, publishingInterval);

    const double interval = fastestInterval(subscribers);
    if (firstSubscriber || !qFuzzyCompare(interval, previousInterval))
        emit subscribeRequested(nodeId, interval);
}

///
/// \brief Drops a chart's request, stopping or re-negotiating the node's interval.
/// \param nodeId Node a chart no longer wants monitored.
///
void TrendPanelWidget::onChartUnsubscribe(const QString &nodeId)
{
    auto *chart = qobject_cast<TrendGraphWidget *>(sender());
    if (!chart)
        return;

    auto it = _nodeSubscribers.find(nodeId);
    if (it == _nodeSubscribers.end())
        return;

    const double previousInterval = fastestInterval(it.value());
    it.value().remove(chart);
    if (it.value().isEmpty()) {
        _nodeSubscribers.erase(it);
        emit unsubscribeRequested(nodeId);
        return;
    }

    const double interval = fastestInterval(it.value());
    if (!qFuzzyCompare(interval, previousInterval))
        emit subscribeRequested(nodeId, interval);
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
/// \brief Updates the subscriptions offered by every chart's settings.
/// \param subscriptions Current subscriptions in row order.
///
void TrendPanelWidget::setSubscriptions(const QVector<SubscriptionItem> &subscriptions)
{
    _subscriptions = subscriptions;
    for (TrendGraphWidget *chart : charts())
        chart->setSubscriptions(subscriptions);
}

///
/// \brief Repoints charts referencing a renamed subscription.
/// \param oldName Previous subscription name.
/// \param newName New subscription name.
///
void TrendPanelWidget::applySubscriptionRename(const QString &oldName, const QString &newName)
{
    for (TrendGraphWidget *chart : charts())
        chart->applySubscriptionRename(oldName, newName);
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
/// \brief Pins the collapse button to the top-right of the tab strip, centred on
///        the tab bar.
///
void TrendPanelWidget::positionCollapseButton()
{
    if (!_collapseButton)
        return;

    const QTabBar *bar = ui->trendTabs->tabBar();
    const int barHeight = bar ? bar->height() : _collapseButton->sizeHint().height();
    const QSize hint = _collapseButton->sizeHint();
    const int margin = 6;
    const int x = ui->trendTabs->width() - hint.width() - margin;
    const int y = qMax(0, (barHeight - hint.height()) / 2);

    _collapseButton->setGeometry(x, y, hint.width(), hint.height());
    _collapseButton->raise();
    _collapseButton->show();
}

///
/// \brief Keeps the collapse button pinned to the tab strip on tab-widget resize
///        and show events.
/// \param watched Object delivering the event.
/// \param event Event being filtered.
/// \return True when the event is consumed.
///
bool TrendPanelWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->trendTabs
        && (event->type() == QEvent::Resize || event->type() == QEvent::Show
            || event->type() == QEvent::LayoutRequest)) {
        positionCollapseButton();
    }
    return QWidget::eventFilter(watched, event);
}

///
/// \brief Clears every chart, stops streaming, and forgets pending reads.
///
void TrendPanelWidget::clearRuntimeData()
{
    for (TrendGraphWidget *chart : charts())
        chart->clear();
    _nodeSubscribers.clear();
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
