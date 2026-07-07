// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file trendpanelwidget.h
/// \brief Declares the trend panel widget.
///

#pragma once

#include <QHash>
#include <QString>
#include <QVector>
#include <QWidget>

#include "appsettings.h"
#include "models/subscriptionitem.h"
#include "opcua/opcuatypes.h"

namespace Ui {
class TrendPanelWidget;
}

class TrendGraphWidget;
class ThemedToolButton;

///
/// \brief Hosts trend chart tabs and routes their data flow to the session.
///
/// Each tab holds one TrendGraphWidget that owns its own Live / 1m / 10m / 1h / 1d
/// toolbar and mode. The panel manages the tab strip (add, close, last-tab reset),
/// coalesces the charts' subscribe / unsubscribe requests so a node monitored from
/// several tabs is subscribed once at the fastest interval any chart asked for, and
/// fans live values and history out to charts.
///
class TrendPanelWidget : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the trend panel, its first chart tab, and toolbar wiring.
    /// \param parent Parent widget.
    ///
    explicit TrendPanelWidget(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the panel and its generated UI.
    ///
    ~TrendPanelWidget() override;

    ///
    /// \brief Adds a node series to the active chart.
    /// \param details Variable node details.
    ///
    void addNode(const OpcUaNodeDetails &details);

    ///
    /// \brief Adds a node series to the active chart.
    /// \param nodeId Node NodeId.
    /// \param displayName Human-readable node name.
    /// \param displayPath Human-readable node path.
    ///
    void addNode(const QString &nodeId, const QString &displayName,
                 const QString &displayPath = {});

    ///
    /// \brief Returns the distinct node ids charted across every tab.
    /// \return Charted node ids without duplicates.
    ///
    QStringList chartedNodeIds() const;

    ///
    /// \brief Applies history results if the panel requested them for the node.
    /// \param nodeId Node whose history arrived.
    /// \param error Read error, empty on success.
    /// \param values History samples in time order.
    /// \return True when the panel had requested this node's history.
    ///
    bool consumeHistory(const QString &nodeId, const QString &error,
                        const QVector<OpcUaHistoryValue> &values);

    ///
    /// \brief Clears every chart, stops streaming, and forgets pending reads.
    ///
    void clearRuntimeData();

    ///
    /// \brief Reports whether the panel is collapsed to its tab strip.
    /// \return True when only the tab strip is visible.
    ///
    bool isCollapsed() const;

    ///
    /// \brief Collapses the panel to its tab strip or restores its full height.
    /// \param collapsed True to collapse, false to restore.
    ///
    void setCollapsed(bool collapsed);

    ///
    /// \brief Persists the active mode.
    /// \param settings Settings store to write to.
    ///
    void saveViewState(AppSettings &settings) const;

    ///
    /// \brief Restores the active mode.
    /// \param settings Settings store to read from.
    ///
    void restoreViewState(AppSettings &settings);

protected:
    ///
    /// \brief Keeps the collapse button pinned to the tab strip on tab-widget
    ///        resize and show events.
    /// \param watched Object delivering the event.
    /// \param event Event being filtered.
    /// \return True when the event is consumed.
    ///
    bool eventFilter(QObject *watched, QEvent *event) override;

public slots:
    ///
    /// \brief Updates the subscriptions offered by every chart's settings.
    /// \param subscriptions Current subscriptions in row order.
    ///
    void setSubscriptions(const QVector<SubscriptionItem> &subscriptions);

    ///
    /// \brief Repoints charts referencing a renamed subscription.
    /// \param oldName Previous subscription name.
    /// \param newName New subscription name.
    ///
    void applySubscriptionRename(const QString &oldName, const QString &newName);

    ///
    /// \brief Forwards streamed values to the charts.
    /// \param values Latest data-access values.
    ///
    void applyLiveValues(const QVector<OpcUaDataValue> &values);

    ///
    /// \brief Applies the timestamp display mode to the charts.
    /// \param mode Local time or UTC.
    ///
    void setTimestampMode(AppSettings::TimestampMode mode);

signals:
    ///
    /// \brief Requests monitoring for a charted node.
    /// \param nodeId Node to monitor.
    /// \param publishingInterval Publishing interval in milliseconds.
    ///
    void subscribeRequested(QString nodeId, double publishingInterval);

    ///
    /// \brief Requests that monitoring stop for a charted node.
    /// \param nodeId Node to stop monitoring.
    ///
    void unsubscribeRequested(QString nodeId);

    ///
    /// \brief Requests a raw history read for a charted node.
    /// \param nodeId Node whose history is read.
    /// \param start Inclusive range start.
    /// \param end Inclusive range end.
    /// \param maxValues Maximum samples to return, or 0 for no limit.
    ///
    void historyReadRequested(QString nodeId, QDateTime start, QDateTime end, quint32 maxValues);

    ///
    /// \brief Requests that a new subscription be created in the shared list.
    /// \param name New subscription name.
    /// \param publishingInterval Publishing interval in milliseconds.
    ///
    void subscriptionCreationRequested(QString name, double publishingInterval);

private:
    TrendGraphWidget *addChartTab();
    TrendGraphWidget *currentChart() const;
    QList<TrendGraphWidget *> charts() const;
    void handleTabChanged(int index);
    void handleTabCloseRequested(int index);
    void onChartSubscribe(const QString &nodeId, double publishingInterval);
    void onChartUnsubscribe(const QString &nodeId);
    int collapsedHeight() const;
    void positionCollapseButton();

    Ui::TrendPanelWidget *ui;
    QWidget *_addTab = nullptr;
    ThemedToolButton *_collapseButton = nullptr;
    int _chartCounter = 0;
    bool _suppressTabChange = false;
    bool _collapsed = false;
    int _expandedMinHeight = 0;
    AppSettings::TimestampMode _timestampMode = AppSettings::TimestampMode::LocalTime;
    QVector<SubscriptionItem> _subscriptions;
    QHash<QString, QHash<TrendGraphWidget *, double>> _nodeSubscribers;
};
