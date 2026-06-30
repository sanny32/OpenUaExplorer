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

#include <QApplication>
#include <QCoreApplication>
#include <QDateTime>
#include <QFont>
#include <QFontMetricsF>
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
/// \brief Card-style value plaque anchored to a data point on the chart.
///
/// Drawn directly on the chart scene so it follows the plotted point: a rounded
/// card with a soft shadow showing the series swatch and name, a clock glyph with
/// the sample time, a separator, and the value in a large weight. Picks up the
/// chart theme colours and never leaks Qt Charts beyond this backend.
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
        const QFont base = QApplication::font();
        _nameFont = base;
        _timeFont = base;
        _valueLabelFont = base;
        _valueFont = base;
        _valueFont.setBold(true);
        _valueFont.setPointSizeF(base.pointSizeF() + 9.0);
        _valueLabel = QCoreApplication::translate("QtChartsView", "Value");

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
    /// \brief Sets the card content and recomputes its geometry.
    /// \param swatch Series colour shown beside the name.
    /// \param name Series legend name.
    /// \param time Sample time text.
    /// \param value Sample value text.
    ///
    void setData(const QColor &swatch, const QString &name, const QString &time,
                 const QString &value)
    {
        _swatch = swatch;
        _name = name;
        _time = time;
        _value = value;
        relayout();
    }

    ///
    /// \brief Applies theme colours to the card and derives muted tones.
    /// \param background Card fill colour.
    /// \param text Primary text colour.
    ///
    void setColors(const QColor &background, const QColor &text)
    {
        _background = background;
        _textColor = text;
        _muted = blend(text, background, 0.45);
        _border = blend(text, background, 0.82);
    }

    ///
    /// \brief Places the card beside the anchor, kept inside the plot area.
    ///
    void updateGeometry()
    {
        prepareGeometryChange();
        const QPointF anchor = _chart->mapToPosition(_anchor, _series);
        const QRectF plot = _chart->plotArea();

        qreal x = anchor.x() + 16.0;
        if (x + _rect.width() > plot.right())
            x = anchor.x() - 16.0 - _rect.width();
        qreal y = anchor.y() - _rect.height() / 2.0;

        x = qBound(plot.left(), x, qMax(plot.left(), plot.right() - _rect.width()));
        y = qBound(plot.top(), y, qMax(plot.top(), plot.bottom() - _rect.height()));
        setPos(x, y);
    }

    QRectF boundingRect() const override
    {
        QRectF rect = _rect.adjusted(-12.0, -10.0, 12.0, 16.0);
        const QPointF anchor = mapFromParent(_chart->mapToPosition(_anchor, _series));
        return rect.united(QRectF(anchor.x() - 8.0, anchor.y() - 8.0, 16.0, 16.0));
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override
    {
        Q_UNUSED(option)
        Q_UNUSED(widget)

        painter->setRenderHint(QPainter::Antialiasing, true);

        const QPointF anchor = mapFromParent(_chart->mapToPosition(_anchor, _series));

        painter->setPen(Qt::NoPen);
        for (int i = 7; i >= 1; --i) {
            painter->setBrush(QColor(0, 0, 0, 5));
            painter->drawRoundedRect(_rect.adjusted(-i, -i + 3.0, i, i + 3.0),
                                     kRadius + i, kRadius + i);
        }

        QPainterPath card;
        card.addRoundedRect(_rect, kRadius, kRadius);
        appendTail(card, anchor);
        card = card.simplified();

        painter->setBrush(_background);
        painter->setPen(QPen(_border, 1.0));
        painter->drawPath(card);

        const QFontMetricsF nameMetrics(_nameFont);
        const qreal headerH = qMax(kSwatch, nameMetrics.height());
        const QRectF swatchRect(kPad, _headerY + (headerH - kSwatch) / 2.0, kSwatch, kSwatch);
        painter->setPen(Qt::NoPen);
        painter->setBrush(_swatch);
        painter->drawRoundedRect(swatchRect, 3.0, 3.0);

        const qreal textX = kPad + kSwatch + kIconGap;
        painter->setFont(_nameFont);
        painter->setPen(_textColor);
        painter->drawText(QRectF(textX, _headerY, _rect.width() - textX - kPad, headerH),
                          Qt::AlignLeft | Qt::AlignVCenter, _name);

        const QFontMetricsF timeMetrics(_timeFont);
        const qreal timeH = qMax(kIcon, timeMetrics.height());
        paintClock(painter, QRectF(kPad, _timeY + (timeH - kIcon) / 2.0, kIcon, kIcon));
        painter->setFont(_timeFont);
        painter->setPen(_muted);
        painter->drawText(QRectF(textX, _timeY, _rect.width() - textX - kPad, timeH),
                          Qt::AlignLeft | Qt::AlignVCenter, _time);

        painter->setPen(QPen(_border, 1.0));
        painter->drawLine(QPointF(kPad, _sepY), QPointF(_rect.width() - kPad, _sepY));

        const QFontMetricsF labelMetrics(_valueLabelFont);
        painter->setFont(_valueLabelFont);
        painter->setPen(_muted);
        painter->drawText(QRectF(kPad, _valueLabelY, _rect.width() - kPad * 2.0, labelMetrics.height()),
                          Qt::AlignLeft | Qt::AlignVCenter, _valueLabel);

        const QFontMetricsF valueMetrics(_valueFont);
        painter->setFont(_valueFont);
        painter->setPen(_textColor);
        painter->drawText(QRectF(kPad, _valueY, _rect.width() - kPad * 2.0, valueMetrics.height()),
                          Qt::AlignLeft | Qt::AlignVCenter, _value);

        painter->setPen(QPen(_background, 2.0));
        painter->setBrush(_swatch);
        painter->drawEllipse(anchor, 5.0, 5.0);
    }

private:
    static constexpr qreal kPad = 16.0;
    static constexpr qreal kSwatch = 14.0;
    static constexpr qreal kIcon = 15.0;
    static constexpr qreal kIconGap = 10.0;
    static constexpr qreal kRadius = 12.0;

    static QColor blend(const QColor &a, const QColor &b, qreal t)
    {
        return QColor(int(a.red() * (1.0 - t) + b.red() * t),
                      int(a.green() * (1.0 - t) + b.green() * t),
                      int(a.blue() * (1.0 - t) + b.blue() * t));
    }

    void relayout()
    {
        const QFontMetricsF nameMetrics(_nameFont);
        const QFontMetricsF timeMetrics(_timeFont);
        const QFontMetricsF labelMetrics(_valueLabelFont);
        const QFontMetricsF valueMetrics(_valueFont);

        const qreal headerW = kSwatch + kIconGap + nameMetrics.horizontalAdvance(_name);
        const qreal timeW = kIcon + kIconGap + timeMetrics.horizontalAdvance(_time);
        const qreal labelW = labelMetrics.horizontalAdvance(_valueLabel);
        const qreal valueW = valueMetrics.horizontalAdvance(_value);
        const qreal contentW = qMax(qMax(headerW, timeW), qMax(labelW, valueW));

        const qreal headerH = qMax(kSwatch, nameMetrics.height());
        const qreal timeH = qMax(kIcon, timeMetrics.height());

        prepareGeometryChange();

        qreal y = kPad;
        _headerY = y;
        y += headerH + 10.0;
        _timeY = y;
        y += timeH + 12.0;
        _sepY = y;
        y += 12.0;
        _valueLabelY = y;
        y += labelMetrics.height() + 4.0;
        _valueY = y;
        y += valueMetrics.height() + kPad;

        _rect = QRectF(0.0, 0.0, kPad * 2.0 + contentW, y);
    }

    void appendTail(QPainterPath &path, const QPointF &anchor) const
    {
        if (_rect.contains(anchor))
            return;
        if (anchor.x() < _rect.left() || anchor.x() > _rect.right()) {
            const qreal edgeX = anchor.x() < _rect.left() ? _rect.left() : _rect.right();
            const qreal baseY = qBound(_rect.top() + 14.0, anchor.y(), _rect.bottom() - 14.0);
            path.moveTo(edgeX, baseY - 8.0);
            path.lineTo(anchor);
            path.lineTo(edgeX, baseY + 8.0);
        } else {
            const qreal edgeY = anchor.y() < _rect.top() ? _rect.top() : _rect.bottom();
            const qreal baseX = qBound(_rect.left() + 14.0, anchor.x(), _rect.right() - 14.0);
            path.moveTo(baseX - 8.0, edgeY);
            path.lineTo(anchor);
            path.lineTo(baseX + 8.0, edgeY);
        }
    }

    void paintClock(QPainter *painter, const QRectF &area) const
    {
        const qreal diameter = qMin(area.width(), area.height()) - 2.0;
        const QRectF dial(area.center().x() - diameter / 2.0,
                          area.center().y() - diameter / 2.0, diameter, diameter);
        painter->setPen(QPen(_muted, 1.4));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(dial);
        const QPointF centre = dial.center();
        painter->drawLine(centre, centre + QPointF(0.0, -diameter * 0.30));
        painter->drawLine(centre, centre + QPointF(diameter * 0.22, diameter * 0.06));
    }

    QChart *_chart = nullptr;
    QLineSeries *_series = nullptr;
    QPointF _anchor;
    QColor _swatch;
    QString _name;
    QString _time;
    QString _value;
    QString _valueLabel;
    QFont _nameFont;
    QFont _timeFont;
    QFont _valueLabelFont;
    QFont _valueFont;
    QRectF _rect;
    qreal _headerY = 0.0;
    qreal _timeY = 0.0;
    qreal _sepY = 0.0;
    qreal _valueLabelY = 0.0;
    qreal _valueY = 0.0;
    QColor _background = QColor(0xff, 0xff, 0xff);
    QColor _textColor = QColor(0x20, 0x20, 0x20);
    QColor _muted = QColor(0x90, 0x90, 0x90);
    QColor _border = QColor(0xe0, 0xe0, 0xe0);
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
    _callout->setAnchor(series, point);
    _callout->setData(series->color(), series->name(),
                      time.toString(QStringLiteral("HH:mm:ss")),
                      QString::number(point.y(), 'g', 6));
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
/// \brief Renames a single series in the legend and hover plaque.
/// \param id Target series.
/// \param name New legend name.
///
void QtChartsView::setSeriesName(const ChartSeriesId &id, const QString &name)
{
    if (QLineSeries *series = _series.value(id))
        series->setName(name);
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

    _callout->setColors(theme.background, theme.text);
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
