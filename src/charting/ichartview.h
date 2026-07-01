// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file ichartview.h
/// \brief Declares the backend-agnostic chart view interface.
///
/// The application talks to charts only through this interface. The concrete
/// rendering backend (currently Qt Charts) lives behind chartViewFactory and
/// can be replaced without touching any consumer. No backend header is included
/// here on purpose.
///

#pragma once

#include <QSize>
#include <QtGlobal>

#include "charttypes.h"

class QWidget;
class QImage;

///
/// \brief Time-series chart abstraction with multiple line series.
///
/// Implementations own a QWidget exposed through widget(); callers embed that
/// widget in their layout. X values are milliseconds since the Unix epoch; the
/// caller decides whether they encode local or UTC wall-clock time.
///
class IChartView
{
public:
    virtual ~IChartView() = default;

    ///
    /// \brief Returns the embeddable view widget owned by the chart.
    /// \return Widget to place into a layout; owned by the chart.
    ///
    virtual QWidget *widget() = 0;

    ///
    /// \brief Adds an empty line series.
    /// \param id Series identifier (node NodeId).
    /// \param name Legend name.
    /// \param color Line colour; an invalid colour picks the next palette entry.
    ///
    virtual void addSeries(const ChartSeriesId &id, const QString &name, const QColor &color) = 0;

    ///
    /// \brief Removes a series and its points.
    /// \param id Series to remove.
    ///
    virtual void removeSeries(const ChartSeriesId &id) = 0;

    ///
    /// \brief Removes all points of a series but keeps the series itself.
    /// \param id Series to clear.
    ///
    virtual void clearSeries(const ChartSeriesId &id) = 0;

    ///
    /// \brief Removes every series and point.
    ///
    virtual void clearAll() = 0;

    ///
    /// \brief Appends one sample to a series (live streaming).
    /// \param id Target series.
    /// \param xMsEpoch X position in milliseconds since the epoch.
    /// \param y Y value.
    /// \param status OPC UA status text for the sample, shown in the hover plaque.
    ///
    virtual void appendPoint(const ChartSeriesId &id, qreal xMsEpoch, qreal y,
                             const QString &status) = 0;

    ///
    /// \brief Replaces all points of a series (history read).
    /// \param id Target series.
    /// \param points Replacement points in time order.
    ///
    virtual void setPoints(const ChartSeriesId &id, const QVector<ChartPoint> &points) = 0;

    ///
    /// \brief Fixes the visible X (time) range.
    /// \param startMsEpoch Range start in milliseconds since the epoch.
    /// \param endMsEpoch Range end in milliseconds since the epoch.
    ///
    virtual void setTimeWindow(qreal startMsEpoch, qreal endMsEpoch) = 0;

    ///
    /// \brief Scales the Y axis to the data range with a small margin.
    ///
    virtual void autoScaleY() = 0;

    ///
    /// \brief Fits both axes to the full data extent.
    ///
    virtual void fit() = 0;

    ///
    /// \brief Shows or hides the chart legend.
    /// \param visible True to show the legend.
    ///
    virtual void setLegendVisible(bool visible) = 0;

    ///
    /// \brief Shows or hides the axis grid lines.
    /// \param visible True to show the grid.
    ///
    virtual void setGridVisible(bool visible) = 0;

    ///
    /// \brief Enables or disables antialiased line rendering.
    /// \param smooth True for smooth (antialiased) lines.
    ///
    virtual void setSmoothLines(bool smooth) = 0;

    ///
    /// \brief Shows or hides the value plaque shown when hovering a series.
    /// \param visible True to display the hover value plaque.
    ///
    virtual void setHoverValueVisible(bool visible) = 0;

    ///
    /// \brief Shows or hides a single series.
    /// \param id Target series.
    /// \param visible True to draw the series.
    ///
    virtual void setSeriesVisible(const ChartSeriesId &id, bool visible) = 0;

    ///
    /// \brief Recolours a single series.
    /// \param id Target series.
    /// \param color New line colour.
    ///
    virtual void setSeriesColor(const ChartSeriesId &id, const QColor &color) = 0;

    ///
    /// \brief Renames a single series in the legend and hover plaque.
    /// \param id Target series.
    /// \param name New legend name.
    ///
    virtual void setSeriesName(const ChartSeriesId &id, const QString &name) = 0;

    ///
    /// \brief Applies the application colour theme to the chart.
    /// \param theme Backend-neutral colour set.
    ///
    virtual void setTheme(const ChartTheme &theme) = 0;

    ///
    /// \brief Renders the current chart to an image for export.
    /// \param size Target image size in pixels.
    /// \return Rendered image.
    ///
    virtual QImage renderToImage(const QSize &size) = 0;
};
