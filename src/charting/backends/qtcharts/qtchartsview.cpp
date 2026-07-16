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
#include <QtCharts/QLegendMarker>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <QApplication>
#include <QCoreApplication>
#include <QDateTime>
#include <QFont>
#include <QFontMetricsF>
#include <QFrame>
#include <QGraphicsEllipseItem>
#include <QGraphicsLayout>
#include <QGuiApplication>
#include <QImage>
#include <QList>
#include <QMargins>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QPixmap>
#include <QPointF>
#include <QRectF>
#include <QScreen>
#include <QWidget>

namespace {

constexpr qreal kDefaultWindowMs = 60000.0;

}

///
/// \brief Card-style value plaque shown as a floating popup above the chart.
///
/// A frameless top-level tooltip window: a rounded card with a soft shadow showing
/// the series swatch and name, a clock glyph with the sample time, a separator, the
/// value in a large weight, and the OPC UA status. Being a top-level window it
/// floats above everything and is never clipped by the chart widget. It is
/// transparent to the mouse so it never disturbs hover tracking, and picks up the
/// chart theme colours without leaking Qt Charts beyond this backend.
///
class ChartCallout : public QWidget
{
public:
    ///
    /// \brief Builds a hidden floating callout owned by the chart view.
    /// \param owner Parent used for lifetime only; the callout is a top-level window.
    ///
    explicit ChartCallout(QWidget *owner)
        : QWidget(owner, Qt::ToolTip | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
    {
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_ShowWithoutActivating);
        setAttribute(Qt::WA_TransparentForMouseEvents);

        const QFont base = QApplication::font();
        _nameFont = base;
        _timeFont = base;
        _valueLabelFont = base;
        _valueFont = base;
        _valueFont.setBold(true);
        _valueFont.setPointSizeF(base.pointSizeF() + 9.0);
        _statusFont = base;
        _valueLabel = QCoreApplication::translate("QtChartsView", "Value");

        hide();
    }

    ///
    /// \brief Sets the card content and recomputes its geometry.
    /// \param swatch Series colour shown beside the name.
    /// \param name Series legend name.
    /// \param time Sample time text.
    /// \param value Sample value text.
    /// \param status Sample OPC UA status text; an empty string hides the status row.
    ///
    void setData(const QColor &swatch, const QString &name, const QString &time,
                 const QString &value, const QString &status)
    {
        _swatch = swatch;
        _name = name;
        _time = time;
        _value = value;
        _status = status;
        relayout();
        resize(int(_rect.width() + 2.0 * kMargin), int(_rect.height() + 2.0 * kMargin));
        update();
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
        _timeColor = blend(text, background, 0.2);
        _border = blend(text, background, 0.82);
        update();
    }

    ///
    /// \brief Sets the semantic colours used to highlight the status text.
    /// \param good Colour for a Good status.
    /// \param uncertain Colour for an Uncertain status.
    /// \param bad Colour for a Bad status.
    ///
    void setStatusColors(const QColor &good, const QColor &uncertain, const QColor &bad)
    {
        if (good.isValid())
            _statusGood = good;
        if (uncertain.isValid())
            _statusUncertain = uncertain;
        if (bad.isValid())
            _statusBad = bad;
        update();
    }

    ///
    /// \brief Positions the card beside a data point and shows it, kept on screen.
    /// \param anchorGlobal The hovered sample in global screen coordinates.
    ///
    /// The card sits to the right of the point with its vertical centre aligned to
    /// it, flipping to the left and clamping to the current screen so it stays fully
    /// visible even when the point is near an edge.
    ///
    void showAt(const QPoint &anchorGlobal)
    {
        const int cardW = int(_rect.width());
        const int cardH = int(_rect.height());

        QRect screen;
        if (const QScreen *s = QGuiApplication::screenAt(anchorGlobal))
            screen = s->availableGeometry();
        else if (const QScreen *s = QGuiApplication::primaryScreen())
            screen = s->availableGeometry();

        int cardX = anchorGlobal.x() + 16;
        if (!screen.isNull() && cardX + cardW > screen.right())
            cardX = anchorGlobal.x() - 16 - cardW;
        int cardY = anchorGlobal.y() - cardH / 2;

        if (!screen.isNull()) {
            cardX = qBound(screen.left(), cardX, qMax(screen.left(), screen.right() - cardW));
            cardY = qBound(screen.top(), cardY, qMax(screen.top(), screen.bottom() - cardH));
        }

        move(cardX - int(kMargin), cardY - int(kMargin));
        show();
        raise();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.translate(kMargin, kMargin);

        painter.setPen(Qt::NoPen);
        for (int i = 7; i >= 1; --i) {
            painter.setBrush(QColor(0, 0, 0, 5));
            painter.drawRoundedRect(_rect.adjusted(-i, -i + 3.0, i, i + 3.0),
                                    kRadius + i, kRadius + i);
        }

        painter.setBrush(_background);
        painter.setPen(QPen(_border, 1.0));
        painter.drawRoundedRect(_rect, kRadius, kRadius);

        const QFontMetricsF nameMetrics(_nameFont);
        const qreal headerH = qMax(kSwatch, nameMetrics.height());
        const QRectF swatchRect(kPad, _headerY + (headerH - kSwatch) / 2.0, kSwatch, kSwatch);
        painter.setPen(Qt::NoPen);
        painter.setBrush(_swatch);
        painter.drawRoundedRect(swatchRect, 3.0, 3.0);

        const qreal textX = kPad + kSwatch + kIconGap;
        painter.setFont(_nameFont);
        painter.setPen(_textColor);
        painter.drawText(QRectF(textX, _headerY, _rect.width() - textX - kPad, headerH),
                         Qt::AlignLeft | Qt::AlignVCenter, _name);

        const QFontMetricsF timeMetrics(_timeFont);
        const qreal timeH = qMax(kIcon, timeMetrics.height());
        paintClock(&painter, QRectF(kPad, _timeY + (timeH - kIcon) / 2.0, kIcon, kIcon));
        painter.setFont(_timeFont);
        painter.setPen(_timeColor);
        painter.drawText(QRectF(textX, _timeY, _rect.width() - textX - kPad, timeH),
                         Qt::AlignLeft | Qt::AlignVCenter, _time);

        painter.setPen(QPen(_border, 1.0));
        painter.drawLine(QPointF(kPad, _sepY), QPointF(_rect.width() - kPad, _sepY));

        const QFontMetricsF labelMetrics(_valueLabelFont);
        painter.setFont(_valueLabelFont);
        painter.setPen(_muted);
        painter.drawText(QRectF(kPad, _valueLabelY, _rect.width() - kPad * 2.0, labelMetrics.height()),
                         Qt::AlignLeft | Qt::AlignVCenter, _valueLabel);

        const QFontMetricsF valueMetrics(_valueFont);
        const QFontMetricsF statusMetrics(_statusFont);
        const qreal badgeH = statusMetrics.height() + 2.0 * kBadgePadY;
        const qreal valueRowH = _status.isEmpty()
                                    ? valueMetrics.height()
                                    : qMax(valueMetrics.height(), badgeH);
        painter.setFont(_valueFont);
        painter.setPen(_textColor);
        painter.drawText(QRectF(kPad, _valueY, _rect.width() - kPad * 2.0, valueRowH),
                         Qt::AlignLeft | Qt::AlignVCenter, _value);

        if (!_status.isEmpty()) {
            const QColor accent = statusColor();
            const qreal badgeW = statusMetrics.horizontalAdvance(_status) + 2.0 * kBadgePadX;
            const qreal badgeX = kPad + valueMetrics.horizontalAdvance(_value) + kValueBadgeGap;
            const QRectF badgeRect(badgeX, _valueY + (valueRowH - badgeH) / 2.0, badgeW, badgeH);

            painter.setPen(Qt::NoPen);
            painter.setBrush(blend(accent, _background, 0.82));
            painter.drawRoundedRect(badgeRect, kBadgeRadius, kBadgeRadius);

            painter.setFont(_statusFont);
            painter.setPen(accent);
            painter.drawText(badgeRect, Qt::AlignCenter, _status);
        }
    }

private:
    static constexpr qreal kPad = 16.0;
    static constexpr qreal kSwatch = 14.0;
    static constexpr qreal kIcon = 15.0;
    static constexpr qreal kIconGap = 10.0;
    static constexpr qreal kRadius = 12.0;
    static constexpr qreal kMargin = 14.0;
    static constexpr qreal kMinContentWidth = 150.0;
    static constexpr qreal kBadgePadX = 12.0;
    static constexpr qreal kBadgePadY = 5.0;
    static constexpr qreal kBadgeRadius = 9.0;
    static constexpr qreal kValueBadgeGap = 12.0;

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
        const QFontMetricsF statusMetrics(_statusFont);

        const bool hasStatus = !_status.isEmpty();
        const qreal headerW = kSwatch + kIconGap + nameMetrics.horizontalAdvance(_name);
        const qreal timeW = kIcon + kIconGap + timeMetrics.horizontalAdvance(_time);
        const qreal labelW = labelMetrics.horizontalAdvance(_valueLabel);
        qreal valueRowW = valueMetrics.horizontalAdvance(_value);
        if (hasStatus) {
            valueRowW += kValueBadgeGap
                         + statusMetrics.horizontalAdvance(_status) + 2.0 * kBadgePadX;
        }
        qreal contentW = qMax(qMax(headerW, timeW), qMax(labelW, valueRowW));
        contentW = qMax(contentW, kMinContentWidth);

        const qreal headerH = qMax(kSwatch, nameMetrics.height());
        const qreal timeH = qMax(kIcon, timeMetrics.height());

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
        qreal valueRowH = valueMetrics.height();
        if (hasStatus)
            valueRowH = qMax(valueRowH, statusMetrics.height() + 2.0 * kBadgePadY);
        y += valueRowH;
        y += kPad;

        _rect = QRectF(0.0, 0.0, kPad * 2.0 + contentW, y);
    }

    ///
    /// \brief Picks the highlight colour for the current status by its name.
    ///
    /// OPC UA status names begin with their severity ("Bad...", "Uncertain...",
    /// otherwise Good), so the leading word classifies the sample without needing
    /// the numeric code here.
    ///
    QColor statusColor() const
    {
        if (_status.startsWith(QLatin1String("Bad"), Qt::CaseInsensitive))
            return _statusBad;
        if (_status.startsWith(QLatin1String("Uncertain"), Qt::CaseInsensitive))
            return _statusUncertain;
        return _statusGood;
    }

    void paintClock(QPainter *painter, const QRectF &area) const
    {
        const qreal diameter = qMin(area.width(), area.height()) - 2.0;
        const QRectF dial(area.center().x() - diameter / 2.0,
                          area.center().y() - diameter / 2.0, diameter, diameter);
        painter->setPen(QPen(_timeColor, 1.4));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(dial);
        const QPointF centre = dial.center();
        painter->drawLine(centre, centre + QPointF(0.0, -diameter * 0.30));
        painter->drawLine(centre, centre + QPointF(diameter * 0.22, diameter * 0.06));
    }

    QColor _swatch;
    QString _name;
    QString _time;
    QString _value;
    QString _status;
    QString _valueLabel;
    QFont _nameFont;
    QFont _timeFont;
    QFont _valueLabelFont;
    QFont _valueFont;
    QFont _statusFont;
    QRectF _rect;
    qreal _headerY = 0.0;
    qreal _timeY = 0.0;
    qreal _sepY = 0.0;
    qreal _valueLabelY = 0.0;
    qreal _valueY = 0.0;
    QColor _background = QColor(0xff, 0xff, 0xff);
    QColor _textColor = QColor(0x20, 0x20, 0x20);
    QColor _muted = QColor(0x90, 0x90, 0x90);
    QColor _timeColor = QColor(0x50, 0x50, 0x50);
    QColor _border = QColor(0xe0, 0xe0, 0xe0);
    QColor _statusGood = QColor(0x2e, 0x9e, 0x44);
    QColor _statusUncertain = QColor(0xc0, 0x7d, 0x00);
    QColor _statusBad = QColor(0xd1, 0x34, 0x38);
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

    if (QGraphicsLayout *legendLayout = _chart->legend()->layout())
        legendLayout->setContentsMargins(6.0, 0.0, 6.0, 0.0);

    _chart->setMargins(QMargins(4, 0, 4, 4));

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

    _callout = new ChartCallout(_view);

    _marker = new QGraphicsEllipseItem(-5.0, -5.0, 10.0, 10.0, _chart);
    _marker->setZValue(11.0);
    _marker->hide();
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
/// The hovered point a line series reports is interpolated along the segment
/// under the cursor, not a stored sample, so the plaque snaps to the nearest
/// actual sample by time. That keeps the value, timestamp and status consistent,
/// all sourced from the same sample.
///
void QtChartsView::showCallout(QLineSeries *series, const ChartSeriesId &id,
                               const QPointF &point, bool state)
{
    const QList<QPointF> points = series->points();
    if (!state || !_hoverValueVisible || points.isEmpty()) {
        hideCallout();
        return;
    }

    int index = 0;
    qreal bestDistance = std::numeric_limits<qreal>::max();
    for (int i = 0; i < points.size(); ++i) {
        const qreal distance = qAbs(points.at(i).x() - point.x());
        if (distance < bestDistance) {
            bestDistance = distance;
            index = i;
        }
    }

    const QPointF sample = points.at(index);
    const QVector<QString> &statuses = _statuses.value(id);
    const QString status = index < statuses.size() ? statuses.at(index) : QString();

    const QDateTime time = QDateTime::fromMSecsSinceEpoch(qint64(sample.x()));
    _callout->setData(series->color(), series->name(),
                      time.toString(QStringLiteral("HH:mm:ss")),
                      QString::number(sample.y(), 'g', 6), status);

    const QPointF itemPos = _chart->mapToPosition(sample, series);
    const QColor border = _theme.background.isValid() ? _theme.background : QColor(Qt::white);
    _marker->setBrush(series->color());
    _marker->setPen(QPen(border, 2.0));
    _marker->setPos(itemPos);
    _marker->show();

    const QPoint globalPos =
        _view->viewport()->mapToGlobal(_view->mapFromScene(_chart->mapToScene(itemPos)));
    _callout->showAt(globalPos);
}

///
/// \brief Hides the value plaque and its on-chart point marker together.
///
void QtChartsView::hideCallout()
{
    _callout->hide();
    _marker->hide();
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

    series->setPointsVisible(_pointsVisible);
    QObject::connect(series, &QLineSeries::hovered, &_hoverContext,
                     [this, series, id](const QPointF &point, bool state) {
                         showCallout(series, id, point, state);
                     });

    styleLegendMarkers();
}

///
/// \brief Gives every legend marker a theme-matching outline.
///
/// Qt draws the legend swatch with a default dark border that stays dark in the
/// dark theme; recolouring the marker pen to the legend text colour keeps the
/// outline readable on both light and dark backgrounds.
///
void QtChartsView::styleLegendMarkers()
{
    if (!_theme.legendText.isValid())
        return;
    const QList<QLegendMarker *> markers = _chart->legend()->markers();
    for (QLegendMarker *marker : markers)
        marker->setPen(QPen(_theme.legendText, 1.0));
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
    _statuses.remove(id);
    hideCallout();
    _chart->removeSeries(series);
    delete series;
}

///
/// \brief Removes a series' points but keeps the series itself.
/// \param id Series to clear.
///
void QtChartsView::clearSeries(const ChartSeriesId &id)
{
    if (QLineSeries *series = _series.value(id)) {
        series->clear();
        _statuses[id].clear();
    }
}

///
/// \brief Removes every series, point, and any visible callout.
///
void QtChartsView::clearAll()
{
    hideCallout();
    for (QLineSeries *series : std::as_const(_series)) {
        _chart->removeSeries(series);
        delete series;
    }
    _series.clear();
    _statuses.clear();
    _paletteIndex = 0;
}

///
/// \brief Appends one streamed sample to a series.
/// \param id Target series.
/// \param xMsEpoch X position in milliseconds since the epoch.
/// \param y Y value.
///
void QtChartsView::appendPoint(const ChartSeriesId &id, qreal xMsEpoch, qreal y,
                               const QString &status)
{
    if (QLineSeries *series = _series.value(id)) {
        series->append(xMsEpoch, y);
        _statuses[id].append(status);
    }
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
    QVector<QString> statuses;
    replacement.reserve(points.size());
    statuses.reserve(points.size());
    for (const ChartPoint &point : points) {
        replacement.append(QPointF(point.xMsEpoch, point.y));
        statuses.append(point.status);
    }
    series->replace(replacement);
    _statuses[id] = std::move(statuses);
}

///
/// \brief Fixes the visible X (time) range.
/// \param startMsEpoch Range start in milliseconds since the epoch.
/// \param endMsEpoch Range end in milliseconds since the epoch.
///
void QtChartsView::setTimeWindow(qreal startMsEpoch, qreal endMsEpoch)
{
    hideCallout();
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
/// \brief Shows or hides a marker at each sample point on every series.
/// \param visible True to draw a marker at every point.
///
void QtChartsView::setPointsVisible(bool visible)
{
    _pointsVisible = visible;
    for (QLineSeries *series : std::as_const(_series))
        series->setPointsVisible(visible);
}

///
/// \brief Shows or hides the hover value plaque, hiding it immediately when off.
/// \param visible True to display the value plaque on hover.
///
void QtChartsView::setHoverValueVisible(bool visible)
{
    _hoverValueVisible = visible;
    if (!visible)
        hideCallout();
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
    styleLegendMarkers();

    _axisY->setGridLineColor(theme.grid);
    _axisY->setLinePenColor(theme.axis);
    _axisY->setLabelsColor(theme.text);
    _axisX->setGridLineColor(theme.grid);
    _axisX->setLinePenColor(theme.axis);
    _axisX->setLabelsColor(theme.text);

    _callout->setColors(theme.background, theme.text);
    _callout->setStatusColors(theme.statusGood, theme.statusUncertain, theme.statusBad);
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
