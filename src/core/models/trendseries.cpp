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

QString TrendSeries::nodeId() const
{
    return _nodeId;
}

QString TrendSeries::label() const
{
    if (!_displayPath.isEmpty())
        return _displayPath;
    if (!_displayName.isEmpty())
        return _displayName;
    return _nodeId;
}

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

QColor TrendSeries::color() const
{
    return _color;
}

void TrendSeries::setColor(const QColor &color)
{
    _color = color;
}

bool TrendSeries::isVisible() const
{
    return _visible;
}

void TrendSeries::setVisible(bool visible)
{
    _visible = visible;
}

const QVector<QPointF> &TrendSeries::points() const
{
    return _points;
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
    _points.reserve(values.size());
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
    }
    trim();
}

void TrendSeries::clear()
{
    _points.clear();
}

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
    _points.remove(0, _points.size() - _maxPoints);
}
