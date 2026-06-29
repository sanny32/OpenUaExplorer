// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file qtchartsview.h
/// \brief Declares the Qt Charts implementation of the chart view interface.
///
/// This is one of the few files permitted to depend on Qt Charts. The rest of
/// the application sees only IChartView.
///

#pragma once

#include <QHash>

#include "ichartview.h"

class QChart;
class QChartView;
class QLineSeries;
class QValueAxis;
class QDateTimeAxis;

///
/// \brief Renders trend series with Qt Charts behind the IChartView API.
///
class QtChartsView : public IChartView
{
public:
    ///
    /// \brief Builds the chart, its view widget, and the time/value axes.
    /// \param parent Parent for the view widget; may be nullptr.
    ///
    explicit QtChartsView(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the chart view; the embedded widget owns the chart.
    ///
    ~QtChartsView() override;

    QWidget *widget() override;
    void addSeries(const ChartSeriesId &id, const QString &name, const QColor &color) override;
    void removeSeries(const ChartSeriesId &id) override;
    void clearSeries(const ChartSeriesId &id) override;
    void clearAll() override;
    void appendPoint(const ChartSeriesId &id, qreal xMsEpoch, qreal y) override;
    void setPoints(const ChartSeriesId &id, const QVector<ChartPoint> &points) override;
    void setTimeWindow(qreal startMsEpoch, qreal endMsEpoch) override;
    void autoScaleY() override;
    void fit() override;
    void setLegendVisible(bool visible) override;
    void setGridVisible(bool visible) override;
    void setSmoothLines(bool smooth) override;
    void setSeriesVisible(const ChartSeriesId &id, bool visible) override;
    void setSeriesColor(const ChartSeriesId &id, const QColor &color) override;
    void setTheme(const ChartTheme &theme) override;
    QImage renderToImage(const QSize &size) override;

private:
    QColor nextPaletteColor();

    QChartView *_view = nullptr;
    QChart *_chart = nullptr;
    QValueAxis *_axisY = nullptr;
    QDateTimeAxis *_axisX = nullptr;
    QHash<ChartSeriesId, QLineSeries *> _series;
    ChartTheme _theme;
    int _paletteIndex = 0;
};
