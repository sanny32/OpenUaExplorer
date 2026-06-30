// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file trendgraphwidget.h
/// \brief Declares the trend graph widget that plots live and historical values.
///

#pragma once

#include <memory>

#include <QHash>
#include <QString>
#include <QVector>
#include <QWidget>

#include <QSet>

#include "appsettings.h"
#include "models/trendseries.h"
#include "opcua/opcuatypes.h"
#include "trendsettings.h"

namespace Ui {
class TrendGraphWidget;
}

class IChartView;
class QTimer;
class QMimeData;
class QContextMenuEvent;

///
/// \brief Plots one or more OPC UA variable nodes as time-series line charts.
///
/// Rendering is delegated to an IChartView created by the charting factory, so
/// this widget never depends on a concrete charting backend. It owns the per-node
/// TrendSeries buffers, accepts dropped nodes, and re-feeds points when the
/// timestamp display mode changes.
///
/// Each chart carries its own Live / 1m / 10m / 1h / 1d toolbar and therefore its
/// own mode: Live streams subscribed values into a rolling window, while a range
/// reads history for that window. The widget emits subscribe / unsubscribe /
/// history requests for its own nodes; the hosting panel ref-counts them across
/// tabs before forwarding them to the session.
///
class TrendGraphWidget : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the chart view and lays it out.
    /// \param parent Parent widget.
    ///
    explicit TrendGraphWidget(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the widget and its chart view.
    ///
    ~TrendGraphWidget() override;

    ///
    /// \brief Adds a node series if not already present.
    /// \param nodeId Node NodeId.
    /// \param displayName Human-readable node name.
    /// \param displayPath Human-readable node path.
    /// \return True when a new series was added.
    ///
    bool addNode(const QString &nodeId, const QString &displayName,
                 const QString &displayPath = {});

    ///
    /// \brief Removes a node series.
    /// \param nodeId Node to remove.
    ///
    void removeNode(const QString &nodeId);

    ///
    /// \brief Reports whether a node is charted here.
    /// \param nodeId Node to test.
    /// \return True when the node has a series.
    ///
    bool hasNode(const QString &nodeId) const;

    ///
    /// \brief Returns the NodeIds of all charted series.
    /// \return Charted NodeIds.
    ///
    QStringList chartedNodeIds() const;

    ///
    /// \brief Appends streamed values to any matching series.
    /// \param values Latest data-access values.
    ///
    void applyLiveValues(const QVector<OpcUaDataValue> &values);

    ///
    /// \brief Replaces a series' points with history samples.
    /// \param nodeId Node whose history arrived.
    /// \param values History samples in time order.
    ///
    void applyHistory(const QString &nodeId, const QVector<OpcUaHistoryValue> &values);

    ///
    /// \brief Sets the visible time window in milliseconds since the epoch.
    /// \param startMsEpoch Window start.
    /// \param endMsEpoch Window end.
    ///
    void setTimeWindow(qreal startMsEpoch, qreal endMsEpoch);

    ///
    /// \brief Scales the value axis to the data.
    ///
    void autoScale();

    ///
    /// \brief Fits both axes to the full data extent.
    ///
    void fit();

    ///
    /// \brief Renders the chart to an image for export.
    /// \param size Target image size.
    /// \return Rendered image.
    ///
    QImage renderToImage(const QSize &size) const;

    ///
    /// \brief Applies history results if this chart requested them for the node.
    /// \param nodeId Node whose history arrived.
    /// \param error Read error, empty on success.
    /// \param values History samples in time order.
    /// \return True when this chart had requested the node's history.
    ///
    bool consumeHistory(const QString &nodeId, const QString &error,
                        const QVector<OpcUaHistoryValue> &values);

    ///
    /// \brief Removes all series and their points, unsubscribing live nodes.
    ///
    void clear();

    ///
    /// \brief Returns the active mode as an int for persistence.
    /// \return 0 for Live, 1 for History.
    ///
    int modeState() const;

    ///
    /// \brief Returns the active window length in milliseconds.
    /// \return Window length.
    ///
    qint64 windowState() const;

    ///
    /// \brief Restores a persisted mode and window.
    /// \param mode 0 for Live, 1 for History.
    /// \param windowMs Window length in milliseconds.
    ///
    void applyModeState(int mode, qint64 windowMs);

    ///
    /// \brief Returns the current display, range and auto-scroll settings.
    /// \return Active settings.
    ///
    TrendDisplaySettings displaySettings() const;

    ///
    /// \brief Applies display, range and auto-scroll settings to the chart.
    /// \param settings Settings to apply.
    ///
    void setDisplaySettings(const TrendDisplaySettings &settings);

    ///
    /// \brief Returns the charted series with their colours and visibility.
    /// \return Series in legend-label order.
    ///
    QVector<TrendSeriesInfo> seriesInfos() const;

    ///
    /// \brief Applies edited per-series colours and visibility.
    /// \param series Series carrying updated colour and visibility.
    ///
    void applySeriesInfos(const QVector<TrendSeriesInfo> &series);

public slots:
    ///
    /// \brief Applies the timestamp display mode and re-feeds the chart.
    /// \param mode Local time or UTC.
    ///
    void setTimestampMode(AppSettings::TimestampMode mode);

signals:
    ///
    /// \brief Emitted after a node series is added.
    /// \param nodeId Added node NodeId.
    /// \param displayName Human-readable node name.
    /// \param displayPath Human-readable node path.
    ///
    void nodeAdded(QString nodeId, QString displayName, QString displayPath);

    ///
    /// \brief Emitted after a node series is removed.
    /// \param nodeId Removed node NodeId.
    ///
    void nodeRemoved(QString nodeId);

    ///
    /// \brief Requests monitoring for a node charted here.
    /// \param nodeId Node to monitor.
    /// \param publishingInterval Publishing interval in milliseconds.
    ///
    void subscribeRequested(QString nodeId, double publishingInterval);

    ///
    /// \brief Requests that monitoring stop for a node charted here.
    /// \param nodeId Node to stop monitoring.
    ///
    void unsubscribeRequested(QString nodeId);

    ///
    /// \brief Requests a raw history read for a node charted here.
    /// \param nodeId Node whose history is read.
    /// \param start Inclusive range start.
    /// \param end Inclusive range end.
    /// \param maxValues Maximum samples to return, or 0 for no limit.
    ///
    void historyReadRequested(QString nodeId, QDateTime start, QDateTime end, quint32 maxValues);

    ///
    /// \brief Requests that the host open settings for this chart.
    ///
    void settingsRequested();

protected:
    void changeEvent(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    bool acceptsNodeDrag(const QMimeData *mimeData) const;
    bool dropNode(const QMimeData *mimeData);
    void showSeriesContextMenu(const QPoint &globalPos);
    void openSettings();
    void applyDisplaySettings();

    enum class Mode {
        Live,
        History
    };

    void connectToolbar();
    void enterLiveMode();
    void setLivePaused(bool paused);
    void enterHistoryMode(qint64 windowMs);
    void refreshHistory();
    void applyWindow();
    void subscribeNode(const QString &nodeId);
    void unsubscribeNode(const QString &nodeId);
    void requestHistory(const QString &nodeId);
    void exportChart();
    void applyTheme();
    void refeedSeries(const TrendSeries &series);
    qreal toChartX(qreal epochMs) const;
    QColor paletteColor(int index) const;

    Ui::TrendGraphWidget *ui;
    std::unique_ptr<IChartView> _chart;
    QHash<QString, TrendSeries> _series;
    AppSettings::TimestampMode _timestampMode = AppSettings::TimestampMode::LocalTime;

    QTimer *_liveTimer = nullptr;
    Mode _mode = Mode::Live;
    bool _livePaused = false;
    qint64 _windowMs = 60000;
    qint64 _windowEndMs = 0;
    TrendDisplaySettings _display;
    QSet<QString> _subscribed;
    QSet<QString> _pendingHistory;
};
