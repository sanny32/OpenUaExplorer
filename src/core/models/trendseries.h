// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file trendseries.h
/// \brief Declares a numeric trend series backing one charted node.
///

#pragma once

#include <QColor>
#include <QPointF>
#include <QString>
#include <QVector>

#include "opcua/opcuatypes.h"

///
/// \brief Holds the buffered numeric samples for one charted OPC UA node.
///
/// Points store the source timestamp as milliseconds since the Unix epoch (UTC)
/// in x and the coerced numeric value in y, independent of any charting backend.
/// Non-numeric values are rejected so only plottable nodes form a series.
///
class TrendSeries
{
public:
    ///
    /// \brief Constructs an empty, unnamed series.
    ///
    TrendSeries() = default;

    ///
    /// \brief Constructs a series for a node.
    /// \param nodeId Node NodeId (series identity).
    /// \param displayName Human-readable node name.
    /// \param displayPath Human-readable node path.
    ///
    TrendSeries(QString nodeId, QString displayName, QString displayPath);

    ///
    /// \brief Returns the NodeId identifying this series.
    /// \return Node NodeId.
    ///
    QString nodeId() const;

    ///
    /// \brief Returns the legend label (path preferred over name, then NodeId).
    /// \return Display label.
    ///
    QString label() const;

    ///
    /// \brief Returns the assigned line colour.
    /// \return Series colour, invalid until assigned.
    ///
    QColor color() const;

    ///
    /// \brief Assigns the line colour.
    /// \param color Colour to use.
    ///
    void setColor(const QColor &color);

    ///
    /// \brief Returns the buffered points (x = ms since epoch UTC, y = value).
    /// \return Points in insertion order.
    ///
    const QVector<QPointF> &points() const;

    ///
    /// \brief Appends a streamed value if it is numeric, trimming to the cap.
    /// \param value Streamed data value.
    /// \return True when the value was numeric and appended.
    ///
    bool appendLive(const OpcUaDataValue &value);

    ///
    /// \brief Replaces the buffer with numeric history samples in time order.
    /// \param values History samples.
    ///
    void setHistory(const QVector<OpcUaHistoryValue> &values);

    ///
    /// \brief Removes all buffered points.
    ///
    void clear();

    ///
    /// \brief Caps the number of retained points; older points are dropped.
    /// \param maxPoints Maximum points to keep (minimum 1).
    ///
    void setMaxPoints(int maxPoints);

    ///
    /// \brief Coerces an OPC UA value to a chartable number.
    /// \param value Value to coerce.
    /// \param out Destination for the numeric result.
    /// \return True for numeric and boolean values, false otherwise.
    ///
    static bool toNumeric(const QVariant &value, double *out);

private:
    void trim();

    QString _nodeId;
    QString _displayName;
    QString _displayPath;
    QColor _color;
    QVector<QPointF> _points;
    int _maxPoints = 50000;
};
