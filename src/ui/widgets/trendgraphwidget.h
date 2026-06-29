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

#include "appsettings.h"
#include "models/trendseries.h"
#include "opcua/opcuatypes.h"

class IChartView;

///
/// \brief Plots one or more OPC UA variable nodes as time-series line charts.
///
/// Rendering is delegated to an IChartView created by the charting factory, so
/// this widget never depends on a concrete charting backend. It owns the per-node
/// TrendSeries buffers, accepts dropped nodes, and re-feeds points when the
/// timestamp display mode changes.
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
    /// \brief Removes all series and their points.
    ///
    void clear();

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

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void applyTheme();
    void refeedSeries(const TrendSeries &series);
    qreal toChartX(qreal epochMs) const;
    QColor paletteColor(int index) const;

    std::unique_ptr<IChartView> _chart;
    QHash<QString, TrendSeries> _series;
    AppSettings::TimestampMode _timestampMode = AppSettings::TimestampMode::LocalTime;
};
