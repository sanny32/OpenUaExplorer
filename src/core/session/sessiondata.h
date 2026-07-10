// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file sessiondata.h
/// \brief Declares the serializable working-session payload.
///

#pragma once

#include <QRect>
#include <QString>
#include <QStringList>
#include <QVector>

#include "models/subscriptionitem.h"
#include "opcua/connectionprofile.h"

///
/// \brief One monitored data-access node restored with a working session.
///
struct SessionNode
{
    /// \brief NodeId string.
    QString nodeId;

    /// \brief Assigned subscription name, empty when the node is not monitored.
    QString subscriptionName;
};

///
/// \brief One charted series restored with a working session's trend tab.
///
struct SessionTrendSeries
{
    /// \brief NodeId string.
    QString nodeId;

    /// \brief Human-readable node name.
    QString displayName;

    /// \brief Human-readable node path.
    QString displayPath;

    /// \brief Line colour as "#AARRGGBB", empty for the automatic palette colour.
    QString color;

    /// \brief Whether the series is drawn.
    bool visible = true;
};

///
/// \brief One trend chart tab restored with a working session.
///
/// Mirrors the scalar options of the trend settings dialog together with the
/// tab's charted series, so a saved session reproduces the trend layout, not
/// just the set of charted nodes.
///
struct SessionTrendTab
{
    /// \brief Rescale the value axis to the data as new points arrive.
    bool autoScale = true;

    /// \brief Show the chart legend.
    bool showLegend = true;

    /// \brief Show the axis grid lines.
    bool showGrid = true;

    /// \brief Render series lines with antialiasing.
    bool smoothLines = true;

    /// \brief How samples are connected: 0 straight segments, 1 hold-last-value steps.
    int lineType = 1;

    /// \brief Draw a marker at each sample point.
    bool showPoints = false;

    /// \brief Show the value plaque when hovering a series line.
    bool showValueTooltip = true;

    /// \brief Series naming mode: 0 display name, 1 NodeId, 2 path.
    int labelMode = 0;

    /// \brief Keep the visible window pinned to "now" while streaming live.
    bool autoScrollLive = true;

    /// \brief Name of the subscription whose publishing interval drives live updates.
    QString liveSubscription = QStringLiteral("Default");

    /// \brief Active mode: 0 for Live, 1 for History.
    int mode = 0;

    /// \brief History window length in milliseconds.
    qint64 windowMs = 60000;

    /// \brief Charted series in legend-label order.
    QVector<SessionTrendSeries> series;
};

///
/// \brief One open node-monitor window restored with a working session.
///
/// Captures the monitored node together with the window's chart display toggles,
/// subscription choice and placement, so a saved session reopens each monitor as
/// it was left, not just the fact that a monitor was open.
///
struct SessionNodeMonitor
{
    /// \brief Monitored variable NodeId string.
    QString nodeId;

    /// \brief Human-readable node name shown in the header and legend.
    QString displayName;

    /// \brief Human-readable node path.
    QString displayPath;

    /// \brief Selected subscription name driving the live update interval.
    QString subscriptionName;

    /// \brief Resolved data-type display text, shown until a fresh read refreshes it.
    QString typeText;

    /// \brief Keep the window above others.
    bool alwaysOnTop = true;

    /// \brief Rescale the value axis to the data as new points arrive.
    bool autoScale = true;

    /// \brief Connect samples as hold-last-value steps instead of straight segments.
    bool stepLines = true;

    /// \brief Show the axis grid lines.
    bool showGrid = true;

    /// \brief Show the chart legend.
    bool showLegend = true;

    /// \brief Draw a marker at each sample point.
    bool showPoints = false;

    /// \brief Show the value plaque when hovering the series line.
    bool showValueTooltip = true;

    /// \brief Window geometry on screen, invalid when it was never placed.
    QRect geometry;
};

///
/// \brief Snapshot of a working session: connection plus its monitoring workspace.
///
/// Secrets are never part of this payload; the connection profile references its
/// credentials by profile id through the keychain, or the user is prompted on load.
///
struct SessionData
{
    /// \brief Active connection profile, excluding any secrets.
    ConnectionProfile profile;

    /// \brief User-created subscriptions to recreate on load.
    QVector<SubscriptionItem> subscriptions;

    /// \brief Monitored data-access nodes with their subscription assignment.
    QVector<SessionNode> dataAccessNodes;

    /// \brief Charted trend node ids (legacy flat layout, read for older files).
    QStringList trendNodes;

    /// \brief Trend chart tabs with their display settings and series.
    QVector<SessionTrendTab> trendTabs;

    /// \brief Open node-monitor windows with their settings.
    QVector<SessionNodeMonitor> nodeMonitors;

    /// \brief Expanded address-space node ids, parents before children.
    QStringList expandedNodes;

    /// \brief Node id selected in the address-space tree, if any.
    QString selectedNode;
};
