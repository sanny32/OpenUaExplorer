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
#include <QFont>
#include <QFontMetrics>
#include <QFrame>
#include <QGraphicsItem>
#include <QImage>
#include <QList>
#include <QMargins>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <QPointF>
#include <QRectF>

namespace {

constexpr qreal kDefaultWindowMs = 60000.0;

}

///
/// \brief Rounded value plaque anchored to a data point on the chart.
///
/// Drawn directly on the chart scene so it follows the plotted point, picks up
/// the chart theme colours, and never leaks Qt Charts beyond this backend.
///
class ChartCallout : public QGraphicsItem
{
public:
    ///
    /// \brief Builds a hidden callout parented to the chart item.
    /// \param chart Chart whose coordinate system anchors the callout.
    ///
    explicit ChartCallout(QChart *chart)
        : QGraphicsItem(chart)
        , _chart(chart)
    {
        setZValue(11.0);
        hide();
    }

    ///
    /// \brief Binds the callout to a series and the hovered value point.
    /// \param series Series the point belongs to (its axes map the position).
    /// \param anchor Hovered value in series coordinates.
    ///
    void setAnchor(QLineSeries *series, const QPointF &anchor)
    {
        _series = series;
        _anchor = anchor;
    }

    ///
    /// \brief Sets the displayed text and recomputes the box geometry.
    /// \param text Multi-line label.
    ///
    void setText(const QString &text)
    {
        _text = text;
        const QFontMetrics metrics(_font);
        _textRect = metrics.boundingRect(QRect(0, 0, 220, 120),
                                         Qt::AlignLeft | Qt::TextWordWrap, _text);
        _textRect.translate(8.0, 6.0);
        prepareGeometryChange();
        _rect = _textRect.adjusted(-8.0, -6.0, 8.0, 6.0);
    }

    ///
    /// \brief Applies theme colours to the plaque.
    /// \param background Box fill colour.
    /// \param text Label colour.
    /// \param border Box outline colour.
    ///
    void setColors(const QColor &background, const QColor &text, const QColor &border)
    {
        _background = background;
        _textColor = text;
        _border = border;
    }

    ///
    /// \brief Repositions the box beside the current anchor point.
    ///
    void updateGeometry()
    {
        prepareGeometryChange();
        setPos(_chart->mapToPosition(_anchor, _series) + QPointF(12.0, -52.0));
    }

    QRectF boundingRect() const override
    {
        const QPointF anchor = mapFromParent(_chart->mapToPosition(_anchor, _series));
        QRectF rect;
        rect.setLeft(qMin(_rect.left(), anchor.x()));
        rect.setRight(qMax(_rect.right(), anchor.x()));
        rect.setTop(qMin(_rect.top(), anchor.y()));
        rect.setBottom(qMax(_rect.bottom(), anchor.y()));
        return rect;
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override
    {
        Q_UNUSED(option)
        Q_UNUSED(widget)

        QPainterPath path;
        path.addRoundedRect(_rect, 5.0, 5.0);

        const QPointF anchor = mapFromParent(_chart->mapToPosition(_anchor, _series));
        if (!_rect.contains(anchor)) {
            const bool above = anchor.y() <= _rect.top();
            const bool below = anchor.y() > _rect.bottom();
            const qreal cornerX = qBound(_rect.left() + 8.0, anchor.x(), _rect.right() - 8.0);
            const qreal edgeY = above ? _rect.top() : _rect.bottom();
            if (above || below) {
                path.moveTo(cornerX - 6.0, edgeY);
                path.lineTo(anchor);
                path.lineTo(cornerX + 6.0, edgeY);
            }
            path = path.simplified();
        }

        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setBrush(_background);
        painter->setPen(QPen(_border));
        painter->drawPath(path);
        painter->setPen(_textColor);
        painter->drawText(_textRect, _text);
    }

private:
    QChart *_chart = nullptr;
    QLineSeries *_series = nullptr;
    QString _text;
    QRectF _textRect;
    QRectF _rect;
    QPointF _anchor;
    QFont _font;
    QColor _background = QColor(0xff, 0xff, 0xff);
    QColor _textColor = QColor(0x20, 0x20, 0x20);
    QColor _border = QColor(0x9e, 0x9e, 0x9e);
};

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
    _view->setFrameShape(QFrame::NoFrame);

    _callout = new ChartCallout(_chart);
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

///
/// \brief Shows or hides the value plaque for a hovered series point.
/// \param series Series under the cursor.
/// \param point Hovered value in series coordinates.
/// \param state True when the cursor enters the line, false when it leaves.
///
void QtChartsView::showCallout(QLineSeries *series, const QPointF &point, bool state)
{
    if (!state || !_hoverValueVisible) {
        _callout->hide();
        return;
    }

    const QDateTime time = QDateTime::fromMSecsSinceEpoch(qint64(point.x()));
    const QString text = QStringLiteral("%1\n%2\n%3")
                             .arg(series->name(),
                                  time.toString(QStringLiteral("HH:mm:ss")),
                                  QString::number(point.y(), 'g', 6));
    _callout->setAnchor(series, point);
    _callout->setText(text);
    _callout->updateGeometry();
    _callout->show();
}

///
/// \brief Adds a line series and wires its hover handler.
/// \param id Series identifier.
/// \param name Legend name.
/// \param color Line colour; an invalid colour picks the next palette entry.
///
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

    series->setPointsVisible(false);
    QObject::connect(series, &QLineSeries::hovered, &_hoverContext,
                     [this, series](const QPointF &point, bool state) {
                         showCallout(series, point, state);
                     });
}

///
/// \brief Removes a series, its points, and any visible callout.
/// \param id Series to remove.
///
void QtChartsView::removeSeries(const ChartSeriesId &id)
{
    QLineSeries *series = _series.take(id);
    if (!series)
        return;
    _callout->hide();
    _chart->removeSeries(series);
    delete series;
}

///
/// \brief Removes a series' points but keeps the series itself.
/// \param id Series to clear.
///
void QtChartsView::clearSeries(const ChartSeriesId &id)
{
    if (QLineSeries *series = _series.value(id))
        series->clear();
}

///
/// \brief Removes every series, point, and any visible callout.
///
void QtChartsView::clearAll()
{
    _callout->hide();
    for (QLineSeries *series : std::as_const(_series)) {
        _chart->removeSeries(series);
        delete series;
    }
    _series.clear();
    _paletteIndex = 0;
}

///
/// \brief Appends one streamed sample to a series.
/// \param id Target series.
/// \param xMsEpoch X position in milliseconds since the epoch.
/// \param y Y value.
///
void QtChartsView::appendPoint(const ChartSeriesId &id, qreal xMsEpoch, qreal y)
{
    if (QLineSeries *series = _series.value(id))
        series->append(xMsEpoch, y);
}

///
/// \brief Replaces all points of a series with history samples.
/// \param id Target series.
/// \param points Replacement points in time order.
///
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

///
/// \brief Fixes the visible X (time) range.
/// \param startMsEpoch Range start in milliseconds since the epoch.
/// \param endMsEpoch Range end in milliseconds since the epoch.
///
void QtChartsView::setTimeWindow(qreal startMsEpoch, qreal endMsEpoch)
{
    _axisX->setRange(QDateTime::fromMSecsSinceEpoch(qint64(startMsEpoch)),
                     QDateTime::fromMSecsSinceEpoch(qint64(endMsEpoch)));
}

///
/// \brief Scales the Y axis to the visible data with a small margin.
///
void QtChartsView::autoScaleY()
{
    qreal minY = std::numeric_limits<qreal>::max();
    qreal maxY = std::numeric_limits<qreal>::lowest();
    bool any = false;
    for (QLineSeries *series : std::as_const(_series)) {
        if (!series->isVisible())
            continue;
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

///
/// \brief Fits both axes to the full extent of the visible data.
///
void QtChartsView::fit()
{
    qreal minX = std::numeric_limits<qreal>::max();
    qreal maxX = std::numeric_limits<qreal>::lowest();
    bool any = false;
    for (QLineSeries *series : std::as_const(_series)) {
        if (!series->isVisible())
            continue;
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

///
/// \brief Shows or hides the chart legend.
/// \param visible True to show the legend.
///
void QtChartsView::setLegendVisible(bool visible)
{
    _chart->legend()->setVisible(visible);
}

///
/// \brief Shows or hides the axis grid lines.
/// \param visible True to show the grid.
///
void QtChartsView::setGridVisible(bool visible)
{
    _axisX->setGridLineVisible(visible);
    _axisY->setGridLineVisible(visible);
}

///
/// \brief Enables or disables antialiased line rendering.
/// \param smooth True for smooth (antialiased) lines.
///
void QtChartsView::setSmoothLines(bool smooth)
{
    _view->setRenderHint(QPainter::Antialiasing, smooth);
}

///
/// \brief Shows or hides the hover value plaque, hiding it immediately when off.
/// \param visible True to display the value plaque on hover.
///
void QtChartsView::setHoverValueVisible(bool visible)
{
    _hoverValueVisible = visible;
    if (!visible)
        _callout->hide();
}

///
/// \brief Shows or hides a single series.
/// \param id Target series.
/// \param visible True to draw the series.
///
void QtChartsView::setSeriesVisible(const ChartSeriesId &id, bool visible)
{
    if (QLineSeries *series = _series.value(id))
        series->setVisible(visible);
}

///
/// \brief Recolours a single series' line and pen.
/// \param id Target series.
/// \param color New line colour.
///
void QtChartsView::setSeriesColor(const ChartSeriesId &id, const QColor &color)
{
    QLineSeries *series = _series.value(id);
    if (!series || !color.isValid())
        return;
    series->setColor(color);
    series->setPen(QPen(color, 1.6));
}

///
/// \brief Applies the colour theme to the chart, axes, and callout.
/// \param theme Backend-neutral colour set.
///
void QtChartsView::setTheme(const ChartTheme &theme)
{
    _theme = theme;

    _chart->setBackgroundBrush(theme.background);
    _chart->setPlotAreaBackgroundBrush(theme.background);
    _chart->setPlotAreaBackgroundVisible(true);
    _view->setBackgroundBrush(theme.background);
    _chart->legend()->setLabelColor(theme.legendText);

    _axisY->setGridLineColor(theme.grid);
    _axisY->setLinePenColor(theme.axis);
    _axisY->setLabelsColor(theme.text);
    _axisX->setGridLineColor(theme.grid);
    _axisX->setLinePenColor(theme.axis);
    _axisX->setLabelsColor(theme.text);

    _callout->setColors(theme.background, theme.text, theme.axis);
}

///
/// \brief Renders the current chart to an image for export.
/// \param size Target image size; an empty size keeps the grabbed size.
/// \return Rendered image.
///
QImage QtChartsView::renderToImage(const QSize &size)
{
    const QPixmap pixmap = _view->grab();
    QImage image = pixmap.toImage();
    if (size.isValid() && !size.isEmpty())
        image = image.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return image;
}
