// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file trendseries.cpp
/// \brief Implements the numeric trend series.
///

#include "trendseries.h"

#include <utility>

#include <QMetaType>

///
/// \brief Constructs a series for a node.
/// \param nodeId Node NodeId (series identity).
/// \param displayName Human-readable node name.
/// \param displayPath Human-readable node path.
///
TrendSeries::TrendSeries(QString nodeId, QString displayName, QString displayPath)
    : _nodeId(std::move(nodeId))
    , _displayName(std::move(displayName))
    , _displayPath(std::move(displayPath))
{
}

///
/// \brief Returns the NodeId identifying this series.
/// \return Node NodeId.
///
QString TrendSeries::nodeId() const
{
    return _nodeId;
}

///
/// \brief Returns the legend label (path preferred over name, then NodeId).
/// \return Display label.
///
QString TrendSeries::label() const
{
    if (!_displayPath.isEmpty())
        return _displayPath;
    if (!_displayName.isEmpty())
        return _displayName;
    return _nodeId;
}

///
/// \brief Returns the legend label for the requested naming mode.
/// \param mode Identifier to prefer when naming the series.
/// \return Display label, falling back to another form and then the NodeId.
///
QString TrendSeries::seriesLabel(TrendLabelMode mode) const
{
    switch (mode) {
    case TrendLabelMode::NodeId:
        return _nodeId;
    case TrendLabelMode::Path:
        if (!_displayPath.isEmpty())
            return _displayPath;
        if (!_displayName.isEmpty())
            return _displayName;
        return _nodeId;
    case TrendLabelMode::DisplayName:
        break;
    }
    if (!_displayName.isEmpty())
        return _displayName;
    return _nodeId;
}

///
/// \brief Returns the assigned line colour.
/// \return Series colour, invalid until assigned.
///
QColor TrendSeries::color() const
{
    return _color;
}

///
/// \brief Assigns the line colour.
/// \param color Colour to use.
///
void TrendSeries::setColor(const QColor &color)
{
    _color = color;
}

///
/// \brief Reports whether the series is drawn.
/// \return True when visible.
///
bool TrendSeries::isVisible() const
{
    return _visible;
}

///
/// \brief Sets whether the series is drawn.
/// \param visible True to draw the series.
///
void TrendSeries::setVisible(bool visible)
{
    _visible = visible;
}

///
/// \brief Returns the buffered points (x = ms since epoch UTC, y = value).
/// \return Points in insertion order.
///
const QVector<QPointF> &TrendSeries::points() const
{
    return _points;
}

///
/// \brief Returns the OPC UA status text aligned with points().
/// \return Status strings in point order.
///
const QVector<QString> &TrendSeries::statuses() const
{
    return _statuses;
}

///
/// \brief Coerces an OPC UA value to a chartable number.
/// \param value Value to coerce.
/// \param out Destination for the numeric result.
/// \return True for numeric and boolean values, false otherwise.
///
bool TrendSeries::toNumeric(const QVariant &value, double *out)
{
    switch (value.typeId()) {
    case QMetaType::Bool:
        *out = value.toBool() ? 1.0 : 0.0;
        return true;
    case QMetaType::Short:
    case QMetaType::UShort:
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::Long:
    case QMetaType::ULong:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
    case QMetaType::Float:
    case QMetaType::Double: {
        bool ok = false;
        const double number = value.toDouble(&ok);
        if (ok)
            *out = number;
        return ok;
    }
    default:
        return false;
    }
}

///
/// \brief Appends a streamed value if it is numeric, trimming to the cap.
/// \param value Streamed data value.
/// \return True when the value was numeric and appended.
///
bool TrendSeries::appendLive(const OpcUaDataValue &value)
{
    double number = 0.0;
    if (!toNumeric(value.value, &number))
        return false;

    const QDateTime timestamp = value.sourceTimestamp.isValid()
        ? value.sourceTimestamp
        : value.serverTimestamp;
    const qreal x = timestamp.isValid()
        ? static_cast<qreal>(timestamp.toMSecsSinceEpoch())
        : static_cast<qreal>(QDateTime::currentMSecsSinceEpoch());

    _points.append(QPointF(x, number));
    _statuses.append(value.status);
    trim();
    return true;
}

///
/// \brief Replaces the buffer with numeric history samples in time order.
/// \param values History samples.
///
void TrendSeries::setHistory(const QVector<OpcUaHistoryValue> &values)
{
    _points.clear();
    _statuses.clear();
    _points.reserve(values.size());
    _statuses.reserve(values.size());
    for (const OpcUaHistoryValue &sample : values) {
        double number = 0.0;
        if (!toNumeric(sample.value, &number))
            continue;
        const QDateTime timestamp = sample.sourceTimestamp.isValid()
            ? sample.sourceTimestamp
            : sample.serverTimestamp;
        if (!timestamp.isValid())
            continue;
        _points.append(QPointF(static_cast<qreal>(timestamp.toMSecsSinceEpoch()), number));
        _statuses.append(sample.status);
    }
    trim();
}

///
/// \brief Backfills with history while keeping newer live points.
/// \param values History samples in time order.
///
void TrendSeries::backfillHistory(const QVector<OpcUaHistoryValue> &values)
{
    QVector<QPointF> historyPoints;
    QVector<QString> historyStatuses;
    historyPoints.reserve(values.size());
    historyStatuses.reserve(values.size());
    for (const OpcUaHistoryValue &sample : values) {
        double number = 0.0;
        if (!toNumeric(sample.value, &number))
            continue;
        const QDateTime timestamp = sample.sourceTimestamp.isValid()
            ? sample.sourceTimestamp
            : sample.serverTimestamp;
        if (!timestamp.isValid())
            continue;
        historyPoints.append(QPointF(static_cast<qreal>(timestamp.toMSecsSinceEpoch()), number));
        historyStatuses.append(sample.status);
    }

    if (historyPoints.isEmpty())
        return;

    const qreal boundaryX = historyPoints.constLast().x();
    for (int i = 0; i < _points.size(); ++i) {
        if (_points.at(i).x() > boundaryX) {
            historyPoints.append(_points.at(i));
            historyStatuses.append(i < _statuses.size() ? _statuses.at(i) : QString());
        }
    }
    _points = std::move(historyPoints);
    _statuses = std::move(historyStatuses);
    trim();
}

///
/// \brief Removes all buffered points.
///
void TrendSeries::clear()
{
    _points.clear();
    _statuses.clear();
}

///
/// \brief Caps the number of retained points; older points are dropped.
/// \param maxPoints Maximum points to keep (minimum 1).
///
void TrendSeries::setMaxPoints(int maxPoints)
{
    _maxPoints = qMax(1, maxPoints);
    trim();
}

///
/// \brief Drops the oldest points once the buffer exceeds the cap.
///
void TrendSeries::trim()
{
    if (_points.size() <= _maxPoints)
        return;
    const int excess = _points.size() - _maxPoints;
    _points.remove(0, excess);
    _statuses.remove(0, qMin(excess, _statuses.size()));
}
