// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file qtchartsview.cpp
/// \brief Implements the Qt Charts chart view.
///

#include "qtchartsview.h"

#include <limits>
#include <utility>

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QLegend>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <QDateTime>
#include <QImage>
#include <QList>
#include <QMargins>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QPointF>

namespace {

constexpr qreal kDefaultWindowMs = 60000.0;

}

///
/// \brief Builds the chart, its view widget, and the time/value axes.
/// \param parent Parent for the view widget; may be nullptr.
///
QtChartsView::QtChartsView(QWidget *parent)
    : _chart(new QChart)
    , _axisY(new QValueAxis)
    , _axisX(new QDateTimeAxis)
{
    _chart->legend()->setVisible(true);
    _chart->legend()->setAlignment(Qt::AlignTop);
    _chart->setMargins(QMargins(4, 4, 4, 4));

    _axisX->setFormat(QStringLiteral("HH:mm:ss"));
    _axisX->setTickCount(4);
    _chart->addAxis(_axisX, Qt::AlignBottom);
    _chart->addAxis(_axisY, Qt::AlignLeft);

    const qreal now = static_cast<qreal>(QDateTime::currentMSecsSinceEpoch());
    _axisX->setRange(QDateTime::fromMSecsSinceEpoch(qint64(now - kDefaultWindowMs)),
                     QDateTime::fromMSecsSinceEpoch(qint64(now)));
    _axisY->setRange(0.0, 1.0);

    _view = new QChartView(_chart, parent);
    _view->setRenderHint(QPainter::Antialiasing, true);
}

///
/// \brief Destroys the chart view; the embedded widget owns the chart.
///
QtChartsView::~QtChartsView() = default;

QWidget *QtChartsView::widget()
{
    return _view;
}

///
/// \brief Picks the next palette colour, cycling when the palette is exhausted.
/// \return Colour for a new series.
///
QColor QtChartsView::nextPaletteColor()
{
    if (_theme.seriesPalette.isEmpty())
        return QColor(0x00, 0xb4, 0x46);
    const QColor color = _theme.seriesPalette.at(_paletteIndex % _theme.seriesPalette.size());
    ++_paletteIndex;
    return color;
}

void QtChartsView::addSeries(const ChartSeriesId &id, const QString &name, const QColor &color)
{
    if (_series.contains(id))
        return;

    auto *series = new QLineSeries(_chart);
    series->setName(name);
    series->setColor(color.isValid() ? color : nextPaletteColor());
    series->setPen(QPen(series->color(), 1.6));

    _chart->addSeries(series);
    series->attachAxis(_axisX);
    series->attachAxis(_axisY);
    _series.insert(id, series);
}

void QtChartsView::removeSeries(const ChartSeriesId &id)
{
    QLineSeries *series = _series.take(id);
    if (!series)
        return;
    _chart->removeSeries(series);
    delete series;
}

void QtChartsView::clearSeries(const ChartSeriesId &id)
{
    if (QLineSeries *series = _series.value(id))
        series->clear();
}

void QtChartsView::clearAll()
{
    for (QLineSeries *series : std::as_const(_series)) {
        _chart->removeSeries(series);
        delete series;
    }
    _series.clear();
    _paletteIndex = 0;
}

void QtChartsView::appendPoint(const ChartSeriesId &id, qreal xMsEpoch, qreal y)
{
    if (QLineSeries *series = _series.value(id))
        series->append(xMsEpoch, y);
}

void QtChartsView::setPoints(const ChartSeriesId &id, const QVector<ChartPoint> &points)
{
    QLineSeries *series = _series.value(id);
    if (!series)
        return;

    QList<QPointF> replacement;
    replacement.reserve(points.size());
    for (const ChartPoint &point : points)
        replacement.append(QPointF(point.xMsEpoch, point.y));
    series->replace(replacement);
}

void QtChartsView::setTimeWindow(qreal startMsEpoch, qreal endMsEpoch)
{
    _axisX->setRange(QDateTime::fromMSecsSinceEpoch(qint64(startMsEpoch)),
                     QDateTime::fromMSecsSinceEpoch(qint64(endMsEpoch)));
}

void QtChartsView::autoScaleY()
{
    qreal minY = std::numeric_limits<qreal>::max();
    qreal maxY = std::numeric_limits<qreal>::lowest();
    bool any = false;
    for (QLineSeries *series : std::as_const(_series)) {
        const QList<QPointF> points = series->points();
        for (const QPointF &point : points) {
            minY = qMin(minY, point.y());
            maxY = qMax(maxY, point.y());
            any = true;
        }
    }
    if (!any)
        return;
    if (qFuzzyCompare(minY, maxY)) {
        minY -= 1.0;
        maxY += 1.0;
    }
    const qreal margin = (maxY - minY) * 0.05;
    _axisY->setRange(minY - margin, maxY + margin);
}

void QtChartsView::fit()
{
    qreal minX = std::numeric_limits<qreal>::max();
    qreal maxX = std::numeric_limits<qreal>::lowest();
    bool any = false;
    for (QLineSeries *series : std::as_const(_series)) {
        const QList<QPointF> points = series->points();
        for (const QPointF &point : points) {
            minX = qMin(minX, point.x());
            maxX = qMax(maxX, point.x());
            any = true;
        }
    }
    if (any) {
        if (qFuzzyCompare(minX, maxX))
            maxX = minX + kDefaultWindowMs;
        setTimeWindow(minX, maxX);
    }
    autoScaleY();
}

void QtChartsView::setTheme(const ChartTheme &theme)
{
    _theme = theme;

    _chart->setBackgroundBrush(theme.background);
    _chart->setPlotAreaBackgroundBrush(theme.background);
    _chart->setPlotAreaBackgroundVisible(true);
    _chart->legend()->setLabelColor(theme.legendText);

    _axisY->setGridLineColor(theme.grid);
    _axisY->setLinePenColor(theme.axis);
    _axisY->setLabelsColor(theme.text);
    _axisX->setGridLineColor(theme.grid);
    _axisX->setLinePenColor(theme.axis);
    _axisX->setLabelsColor(theme.text);
}

QImage QtChartsView::renderToImage(const QSize &size)
{
    const QPixmap pixmap = _view->grab();
    QImage image = pixmap.toImage();
    if (size.isValid() && !size.isEmpty())
        image = image.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return image;
}
