// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file trendgraphwidget.cpp
/// \brief Implements the trend graph widget.
///

#include "trendgraphwidget.h"

#include <utility>

#include <QDateTime>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QVBoxLayout>

#include "application.h"
#include "appcolors.h"
#include "apptheme.h"
#include "charttypes.h"
#include "chartviewfactory.h"
#include "ichartview.h"
#include "models/addressspacemimedata.h"

namespace {

const QVector<QColor> kSeriesPalette = {
    QColor(0x00, 0xb4, 0x46),
    QColor(0x25, 0x63, 0xeb),
    QColor(0xc0, 0x7d, 0x00),
    QColor(0xd1, 0x34, 0x38),
    QColor(0x8b, 0x5c, 0xf6),
    QColor(0x08, 0x91, 0xb2),
    QColor(0xdb, 0x27, 0x77),
    QColor(0x65, 0xa3, 0x0d),
};

}

///
/// \brief Builds the chart view and lays it out.
/// \param parent Parent widget.
///
TrendGraphWidget::TrendGraphWidget(QWidget *parent)
    : QWidget(parent)
    , _chart(ChartViewFactory::createChartView(this))
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAcceptDrops(true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(_chart->widget());

    applyTheme();

    if (auto *app = qobject_cast<Application *>(qApp)) {
        connect(&app->theme(), &AppTheme::colorSchemeChanged, this,
                &TrendGraphWidget::applyTheme);
    }
}

///
/// \brief Destroys the widget and its chart view.
///
TrendGraphWidget::~TrendGraphWidget() = default;

///
/// \brief Returns a distinct palette colour for a series index.
/// \param index Series index.
/// \return Cycled palette colour.
///
QColor TrendGraphWidget::paletteColor(int index) const
{
    return kSeriesPalette.at(index % kSeriesPalette.size());
}

bool TrendGraphWidget::addNode(const QString &nodeId, const QString &displayName,
                               const QString &displayPath)
{
    if (nodeId.isEmpty() || _series.contains(nodeId))
        return false;

    TrendSeries series(nodeId, displayName, displayPath);
    const QColor color = paletteColor(_series.size());
    series.setColor(color);
    _series.insert(nodeId, series);

    _chart->addSeries(nodeId, series.label(), color);

    emit nodeAdded(nodeId, displayName, displayPath);
    return true;
}

void TrendGraphWidget::removeNode(const QString &nodeId)
{
    if (_series.remove(nodeId) == 0)
        return;
    _chart->removeSeries(nodeId);
    emit nodeRemoved(nodeId);
}

bool TrendGraphWidget::hasNode(const QString &nodeId) const
{
    return _series.contains(nodeId);
}

QStringList TrendGraphWidget::chartedNodeIds() const
{
    return _series.keys();
}

void TrendGraphWidget::applyLiveValues(const QVector<OpcUaDataValue> &values)
{
    for (const OpcUaDataValue &value : values) {
        auto it = _series.find(value.nodeId);
        if (it == _series.end())
            continue;
        const int before = it->points().size();
        if (!it->appendLive(value))
            continue;
        if (it->points().size() == before)
            continue;
        const QPointF &point = it->points().constLast();
        _chart->appendPoint(value.nodeId, toChartX(point.x()), point.y());
    }
}

void TrendGraphWidget::applyHistory(const QString &nodeId,
                                    const QVector<OpcUaHistoryValue> &values)
{
    auto it = _series.find(nodeId);
    if (it == _series.end())
        return;
    it->setHistory(values);
    refeedSeries(*it);
}

void TrendGraphWidget::setTimeWindow(qreal startMsEpoch, qreal endMsEpoch)
{
    _chart->setTimeWindow(toChartX(startMsEpoch), toChartX(endMsEpoch));
}

void TrendGraphWidget::autoScale()
{
    _chart->autoScaleY();
}

void TrendGraphWidget::fit()
{
    _chart->fit();
}

QImage TrendGraphWidget::renderToImage(const QSize &size) const
{
    return _chart->renderToImage(size);
}

void TrendGraphWidget::clear()
{
    _series.clear();
    _chart->clearAll();
}

///
/// \brief Applies the timestamp display mode and re-feeds the chart.
/// \param mode Local time or UTC.
///
void TrendGraphWidget::setTimestampMode(AppSettings::TimestampMode mode)
{
    if (_timestampMode == mode)
        return;
    _timestampMode = mode;
    for (const TrendSeries &series : std::as_const(_series))
        refeedSeries(series);
}

///
/// \brief Maps an epoch timestamp to the chart's local-time X axis.
/// \param epochMs Source timestamp in milliseconds since the epoch (UTC).
/// \return X position so the local-time axis renders the chosen wall clock.
///
qreal TrendGraphWidget::toChartX(qreal epochMs) const
{
    if (_timestampMode != AppSettings::TimestampMode::Utc)
        return epochMs;
    const qint64 offsetMs = qint64(QDateTime::currentDateTime().offsetFromUtc()) * 1000;
    return epochMs - static_cast<qreal>(offsetMs);
}

///
/// \brief Pushes a series' buffered points into the chart with the active mode.
/// \param series Series whose points are re-fed.
///
void TrendGraphWidget::refeedSeries(const TrendSeries &series)
{
    const QVector<QPointF> &points = series.points();
    QVector<ChartPoint> mapped;
    mapped.reserve(points.size());
    for (const QPointF &point : points)
        mapped.append(ChartPoint{toChartX(point.x()), point.y()});
    _chart->setPoints(series.nodeId(), mapped);
}

///
/// \brief Builds a chart theme from AppColors and applies it.
///
void TrendGraphWidget::applyTheme()
{
    ChartTheme theme;
    theme.background = palette().color(QPalette::Base);
    theme.grid = AppColors::noticeNeutralBorder();
    theme.axis = AppColors::caption();
    theme.text = AppColors::subtitleText();
    theme.legendText = AppColors::titleText();
    theme.seriesPalette = kSeriesPalette;
    _chart->setTheme(theme);
}

///
/// \brief Accepts a drag that carries a droppable variable node.
///
void TrendGraphWidget::dragEnterEvent(QDragEnterEvent *event)
{
    dragMoveEvent(event);
}

///
/// \brief Accepts the drag as a copy while it carries a variable node.
///
void TrendGraphWidget::dragMoveEvent(QDragMoveEvent *event)
{
    OpcUaNodeInfo node;
    if (AddressSpaceMime::decodeNode(event->mimeData(), &node)
        && !node.nodeId.isEmpty() && OpcUa::isVariable(node.nodeClass)) {
        event->setDropAction(Qt::CopyAction);
        event->accept();
    } else {
        event->ignore();
    }
}

///
/// \brief Adds the dropped variable node as a new series.
///
void TrendGraphWidget::dropEvent(QDropEvent *event)
{
    OpcUaNodeInfo node;
    if (!AddressSpaceMime::decodeNode(event->mimeData(), &node)
        || node.nodeId.isEmpty() || !OpcUa::isVariable(node.nodeClass)) {
        event->ignore();
        return;
    }
    const QString label = node.displayName.isEmpty()
        ? (node.browseName.isEmpty() ? node.nodeId : node.browseName)
        : node.displayName;
    addNode(node.nodeId, label, node.displayPath);
    event->setDropAction(Qt::CopyAction);
    event->accept();
}
