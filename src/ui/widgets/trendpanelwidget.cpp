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
#include <QColor>
#include <QDataStream>
#include <QEvent>
#include <QIODevice>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QResizeEvent>
#include <QShortcut>
#include <QTabBar>

#include "models/trendseries.h"
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
    updateCollapseButton();
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

    _tabNameEditor = new QLineEdit(ui->trendTabs->tabBar());
    _tabNameEditor->setObjectName(QStringLiteral("tabNameEditor"));
    _tabNameEditor->hide();
    _tabNameEditor->installEventFilter(this);
    ui->trendTabs->tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->trendTabs->tabBar()->setFocusPolicy(Qt::StrongFocus);
    auto *renameShortcut = new QShortcut(QKeySequence(Qt::Key_F2), ui->trendTabs);
    renameShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    connect(ui->trendTabs, &QTabWidget::currentChanged, this,
            &TrendPanelWidget::handleTabChanged);
    connect(ui->trendTabs, &QTabWidget::tabCloseRequested, this,
            &TrendPanelWidget::handleTabCloseRequested);
    connect(ui->trendTabs, &QTabWidget::tabBarDoubleClicked, this,
            &TrendPanelWidget::renameTab);
    connect(ui->trendTabs->tabBar(), &QWidget::customContextMenuRequested, this,
            &TrendPanelWidget::showTabContextMenu);
    connect(_tabNameEditor, &QLineEdit::editingFinished, this,
            &TrendPanelWidget::commitTabRename);
    connect(renameShortcut, &QShortcut::activated, this,
            &TrendPanelWidget::renameCurrentTab);
}

///
/// \brief Destroys the panel and its generated UI, dropping an open rename first so the
///        editor's editingFinished on teardown never reaches a deleted ui.
///
TrendPanelWidget::~TrendPanelWidget()
{
    cancelTabRename();
    delete ui;
}

///
/// \brief Retranslates the generated UI on a language change.
/// \param event Change event being handled.
///
void TrendPanelWidget::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        updateCollapseButton();
        retranslateTabTitles();
    }
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
    const int number = ++_chartCounter;
    const int index = ui->trendTabs->insertTab(insertAt < 0 ? ui->trendTabs->count() : insertAt,
                                               chart, defaultTabTitle(number));
    ui->trendTabs->tabBar()->setTabData(index, number);
    ui->trendTabs->setCurrentWidget(chart);
    return chart;
}

///
/// \brief Returns the title a chart tab carries until the user renames it.
/// \param number Sequence number of the chart.
/// \return Title in the current language.
///
QString TrendPanelWidget::defaultTabTitle(int number) const
{
    return tr("Trend %1").arg(number);
}

///
/// \brief Reports whether a chart tab still carries the title it was created with.
/// \param index Tab index.
/// \return True when the user never renamed the tab.
///
bool TrendPanelWidget::hasDefaultTitle(int index) const
{
    return ui->trendTabs->tabBar()->tabData(index).isValid();
}

///
/// \brief Reports whether a saved title is the tab's default one, in English or the current language.
/// \param index Tab index.
/// \param title Saved title.
/// \return True when the title was not chosen by the user.
///
bool TrendPanelWidget::isDefaultTabTitle(int index, const QString &title) const
{
    const QVariant number = ui->trendTabs->tabBar()->tabData(index);
    if (!number.isValid())
        return false;
    return title == defaultTabTitle(number.toInt())
        || title == QStringLiteral("Trend %1").arg(number.toInt());
}

///
/// \brief Gives a chart tab a user-chosen title, which no longer follows the language.
/// \param index Tab index.
/// \param title New title.
///
void TrendPanelWidget::setCustomTabTitle(int index, const QString &title)
{
    ui->trendTabs->setTabText(index, title);
    ui->trendTabs->tabBar()->setTabData(index, QVariant());
}

///
/// \brief Re-applies the default titles of the chart tabs the user never renamed.
///
void TrendPanelWidget::retranslateTabTitles()
{
    for (int index = 0; index < ui->trendTabs->count(); ++index) {
        const QVariant number = ui->trendTabs->tabBar()->tabData(index);
        if (number.isValid())
            ui->trendTabs->setTabText(index, defaultTabTitle(number.toInt()));
    }
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

    cancelTabRename();
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
/// \brief Offers rename and close for the chart tab under the cursor.
///
/// Rename is queued until the menu has closed: the menu hands focus back on closing,
/// which would take it from the title editor and end the rename at once.
///
/// \param pos Click position in tab-bar coordinates.
///
void TrendPanelWidget::showTabContextMenu(const QPoint &pos)
{
    QTabBar *bar = ui->trendTabs->tabBar();
    const int index = bar->tabAt(pos);
    if (!qobject_cast<TrendGraphWidget *>(ui->trendTabs->widget(index)))
        return;

    QMenu menu(this);
    connect(menu.addAction(tr("Rename")), &QAction::triggered, this,
            [this, index]() { renameTab(index); }, Qt::QueuedConnection);
    connect(menu.addAction(tr("Close")), &QAction::triggered, this,
            [this, index]() { handleTabCloseRequested(index); });
    menu.exec(bar->mapToGlobal(pos));
}

///
/// \brief Opens an inline editor over a chart tab's title.
/// \param index Tab to rename; the add-tab and invalid indexes are ignored.
///
void TrendPanelWidget::renameTab(int index)
{
    auto *chart = qobject_cast<TrendGraphWidget *>(ui->trendTabs->widget(index));
    if (!chart)
        return;

    _renamedChart = chart;
    _tabNameEditor->setGeometry(ui->trendTabs->tabBar()->tabRect(index));
    _tabNameEditor->setText(ui->trendTabs->tabText(index));
    _tabNameEditor->selectAll();
    _tabNameEditor->show();
    _tabNameEditor->setFocus();
}

///
/// \brief Starts renaming the active chart tab unless a rename is already in progress.
///
void TrendPanelWidget::renameCurrentTab()
{
    if (!_renamedChart)
        renameTab(ui->trendTabs->currentIndex());
}

///
/// \brief Applies the edited title, keeping the old one when the edit is blank.
///
void TrendPanelWidget::commitTabRename()
{
    if (!_renamedChart)
        return;

    const int index = ui->trendTabs->indexOf(_renamedChart);
    const QString title = _tabNameEditor->text().trimmed();
    cancelTabRename();
    if (index >= 0 && !title.isEmpty() && title != ui->trendTabs->tabText(index))
        setCustomTabTitle(index, title);
}

///
/// \brief Closes the inline title editor without applying it; the target is cleared
///        first because hiding the focused editor emits editingFinished.
///
void TrendPanelWidget::cancelTabRename()
{
    _renamedChart = nullptr;
    _tabNameEditor->hide();
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
/// \brief Adds a node series to the active chart when its DataType can be charted.
/// \param details Variable node details.
///
void TrendPanelWidget::addNode(const OpcUaNodeDetails &details)
{
    if (!TrendSeries::isTrendable(details))
        return;
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
/// \brief Returns the distinct node ids charted across every tab.
/// \return Charted node ids without duplicates.
///
QStringList TrendPanelWidget::chartedNodeIds() const
{
    QStringList result;
    const QList<TrendGraphWidget *> tabs = charts();
    for (TrendGraphWidget *chart : tabs) {
        const QStringList nodeIds = chart->chartedNodeIds();
        for (const QString &nodeId : nodeIds) {
            if (!result.contains(nodeId))
                result.append(nodeId);
        }
    }
    return result;
}

///
/// \brief Captures every chart tab with its display settings and series.
/// \return Trend tabs in tab order, suitable for a saved session.
///
QVector<SessionTrendTab> TrendPanelWidget::captureTrendTabs() const
{
    QVector<SessionTrendTab> result;
    const QList<TrendGraphWidget *> tabs = charts();
    for (TrendGraphWidget *chart : tabs) {
        const TrendDisplaySettings display = chart->displaySettings();
        SessionTrendTab tab;
        const int index = ui->trendTabs->indexOf(chart);
        if (!hasDefaultTitle(index))
            tab.title = ui->trendTabs->tabText(index);
        tab.autoScale = display.autoScale;
        tab.showLegend = display.showLegend;
        tab.showGrid = display.showGrid;
        tab.smoothLines = display.smoothLines;
        tab.lineType = static_cast<int>(display.lineType);
        tab.showPoints = display.showPoints;
        tab.showValueTooltip = display.showValueTooltip;
        tab.labelMode = static_cast<int>(display.labelMode);
        tab.autoScrollLive = display.autoScrollLive;
        tab.liveSubscription = display.liveSubscription;
        tab.mode = display.mode;
        tab.windowMs = display.windowMs;

        const QVector<TrendSeriesInfo> infos = chart->seriesInfos();
        for (const TrendSeriesInfo &info : infos) {
            SessionTrendSeries series;
            series.nodeId = info.nodeId;
            series.displayName = info.displayName;
            series.displayPath = info.displayPath;
            series.color = info.color.isValid() ? info.color.name(QColor::HexArgb) : QString();
            series.visible = info.visible;
            tab.series.append(series);
        }
        result.append(tab);
    }
    return result;
}

///
/// \brief Rebuilds the chart tabs from a saved session's trend layout.
/// \param tabs Trend tabs to recreate; ignored when empty.
///
void TrendPanelWidget::restoreTrendTabs(const QVector<SessionTrendTab> &tabs)
{
    if (tabs.isEmpty())
        return;

    cancelTabRename();
    _suppressTabChange = true;
    const QList<TrendGraphWidget *> existing = charts();
    for (TrendGraphWidget *chart : existing) {
        chart->clear();
        ui->trendTabs->removeTab(ui->trendTabs->indexOf(chart));
        chart->deleteLater();
    }
    // The charts that held the requests are gone, so their bookkeeping must not outlive them.
    _nodeSubscribers.clear();
    _chartCounter = 0;
    _suppressTabChange = false;

    for (const SessionTrendTab &tab : tabs) {
        TrendGraphWidget *chart = addChartTab();
        const int index = ui->trendTabs->indexOf(chart);
        if (!tab.title.isEmpty() && !isDefaultTabTitle(index, tab.title))
            setCustomTabTitle(index, tab.title);
        for (const SessionTrendSeries &series : tab.series)
            chart->addNode(series.nodeId, series.displayName, series.displayPath);

        TrendDisplaySettings display;
        display.autoScale = tab.autoScale;
        display.showLegend = tab.showLegend;
        display.showGrid = tab.showGrid;
        display.smoothLines = tab.smoothLines;
        display.lineType = static_cast<TrendLineType>(tab.lineType);
        display.showPoints = tab.showPoints;
        display.showValueTooltip = tab.showValueTooltip;
        display.labelMode = static_cast<TrendLabelMode>(tab.labelMode);
        display.autoScrollLive = tab.autoScrollLive;
        display.liveSubscription = tab.liveSubscription;
        display.mode = tab.mode;
        display.windowMs = tab.windowMs;
        chart->setDisplaySettings(display);

        QVector<TrendSeriesInfo> infos;
        infos.reserve(tab.series.size());
        for (const SessionTrendSeries &series : tab.series) {
            TrendSeriesInfo info;
            info.nodeId = series.nodeId;
            info.displayName = series.displayName;
            info.displayPath = series.displayPath;
            info.color = series.color.isEmpty() ? QColor() : QColor(series.color);
            info.visible = series.visible;
            infos.append(info);
        }
        chart->applySeriesInfos(infos);
    }

    if (ui->trendTabs->count() > 0)
        ui->trendTabs->setCurrentIndex(0);
    setCollapsed(false);
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

    updateCollapseButton();
}

///
/// \brief Shows the collapse button's icon and tooltip for the action it would take next.
///
void TrendPanelWidget::updateCollapseButton()
{
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
///        and show events, and cancels a tab rename on Escape.
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
    if (watched == _tabNameEditor && event->type() == QEvent::KeyPress
        && static_cast<QKeyEvent *>(event)->key() == Qt::Key_Escape) {
        cancelTabRename();
        return true;
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

#include "moc_trendpanelwidget.cpp"
