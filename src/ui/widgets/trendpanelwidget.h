// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file trendpanelwidget.h
/// \brief Declares the trend panel widget.
///

#pragma once

#include <QHash>
#include <QSet>
#include <QString>
#include <QVector>
#include <QWidget>

#include "appsettings.h"
#include "opcua/opcuatypes.h"

namespace Ui {
class TrendPanelWidget;
}

class TrendGraphWidget;
class QButtonGroup;
class QTimer;

///
/// \brief Hosts trend charts and drives their live/historical data flow.
///
/// The toolbar's Live / 1m / 10m / 1h / 1d buttons choose the mode: Live streams
/// subscribed values into a rolling window, while the range buttons read history
/// for that window. Each tab holds one chart that may carry several series.
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
    /// \brief Persists the active mode.
    /// \param settings Settings store to write to.
    ///
    void saveViewState(AppSettings &settings) const;

    ///
    /// \brief Restores the active mode.
    /// \param settings Settings store to read from.
    ///
    void restoreViewState(AppSettings &settings);

public slots:
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

private:
    enum class Mode {
        Live,
        History
    };

    TrendGraphWidget *addChartTab();
    TrendGraphWidget *currentChart() const;
    QList<TrendGraphWidget *> charts() const;
    QStringList allNodeIds() const;
    void configureToolbar();
    void handleTabChanged(int index);
    void enterLiveMode();
    void enterHistoryMode(qint64 windowMs);
    void reconcileSubscriptions();
    void applyWindow();
    void onNodeAdded(const QString &nodeId);
    void onNodeRemoved(const QString &nodeId);
    void subscribeNode(const QString &nodeId);
    void unsubscribeNode(const QString &nodeId);
    void requestHistory(const QString &nodeId);
    void exportCurrentChart();

    Ui::TrendPanelWidget *ui;
    QButtonGroup *_modeGroup = nullptr;
    QTimer *_liveTimer = nullptr;
    QWidget *_addTab = nullptr;
    Mode _mode = Mode::Live;
    qint64 _windowMs = 60000;
    int _chartCounter = 0;
    QSet<QString> _subscribed;
    QHash<QString, bool> _pendingHistory;
    bool _restoring = false;
};
