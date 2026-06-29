// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file charttypes.h
/// \brief Defines backend-neutral data objects for the charting API.
///
/// This header must never include any concrete charting backend (Qt Charts,
/// QCustomPlot, ...). It carries only plain value types so that the application
/// can depend on the charting API without pulling in a specific backend.
///

#pragma once

#include <QColor>
#include <QString>
#include <QVector>

///
/// \brief Identifies one series within a chart (the node's NodeId).
///
using ChartSeriesId = QString;

///
/// \brief One plotted sample: an X position in milliseconds since the epoch and a Y value.
///
struct ChartPoint
{
    /// \brief X position in milliseconds since the Unix epoch.
    qreal xMsEpoch = 0.0;
    /// \brief Y value.
    qreal y = 0.0;
};

///
/// \brief Backend-neutral colour set applied to a chart, sourced from AppColors.
///
struct ChartTheme
{
    /// \brief Plot and chart background fill.
    QColor background;
    /// \brief Grid line colour.
    QColor grid;
    /// \brief Axis line and tick colour.
    QColor axis;
    /// \brief Axis label and title text colour.
    QColor text;
    /// \brief Legend text colour.
    QColor legendText;
    /// \brief Palette cycled through when a series is added without an explicit colour.
    QVector<QColor> seriesPalette;
};
