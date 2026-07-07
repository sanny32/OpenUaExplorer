// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file trendsettings.h
/// \brief Declares the value types exchanged between a trend chart and its settings dialog.
///

#pragma once

#include <QColor>
#include <QString>
#include <QtGlobal>

#include "models/trendseries.h"

///
/// \brief Chooses how a series' samples are connected between points.
///
enum class TrendLineType {
    Line, ///< Straight segments interpolating between samples.
    Step  ///< Hold-last-value steps, matching on-change reporting.
};

///
/// \brief Per-chart display and range options edited in the trend settings dialog.
///
/// These carry the scalar choices shown in the dialog. Per-series visibility and
/// colour are exchanged separately through TrendSeriesInfo because they vary in
/// count with the charted nodes.
///
struct TrendDisplaySettings
{
    /// \brief Rescale the value axis to the data as new points arrive.
    bool autoScale = true;
    /// \brief Show the chart legend.
    bool showLegend = true;
    /// \brief Show the axis grid lines.
    bool showGrid = true;
    /// \brief Render series lines with antialiasing.
    bool smoothLines = true;
    /// \brief How samples are connected: straight segments or hold-last-value steps.
    TrendLineType lineType = TrendLineType::Step;
    /// \brief Draw a marker at each sample point.
    bool showPoints = false;
    /// \brief Show the value plaque when hovering a series line.
    bool showValueTooltip = true;
    /// \brief Identifier used to name series in the legend and hover plaque.
    TrendLabelMode labelMode = TrendLabelMode::DisplayName;
    /// \brief Keep the visible window pinned to "now" while streaming live.
    bool autoScrollLive = true;
    /// \brief Name of the subscription whose publishing interval drives live updates.
    QString liveSubscription = QStringLiteral("Default");
    /// \brief Active mode: 0 for Live, 1 for History.
    int mode = 0;
    /// \brief History window length in milliseconds.
    qint64 windowMs = 60000;
};

///
/// \brief Identity, label, colour and visibility of one charted series.
///
struct TrendSeriesInfo
{
    /// \brief Node NodeId identifying the series.
    QString nodeId;
    /// \brief Human-readable legend label.
    QString label;
    /// \brief Current line colour.
    QColor color;
    /// \brief Whether the series is drawn.
    bool visible = true;
};
