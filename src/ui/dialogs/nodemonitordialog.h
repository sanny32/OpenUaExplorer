// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file nodemonitordialog.h
/// \brief Declares the single-node live monitor dialog.
///

#pragma once

#include <memory>

#include <QString>

#include "dialogs/appbasedialog.h"
#include "models/subscriptionitem.h"
#include "models/trendseries.h"
#include "opcua/opcuatypes.h"
#include "session/sessiondata.h"

namespace Ui {
class NodeMonitorDialog;
}

class OpcUaBackend;
class IChartView;
class ThemedPushButton;
class QMimeData;
class QTimer;

///
/// \brief Watches one OPC UA variable node live in a modeless, always-on-top window.
///
/// Shows the current value, resolved data type, quality, and source/server
/// timestamps of a single node, updated over a 1000 ms subscription, and plots a
/// rolling trend of the last minute through an IChartView. A node can be targeted
/// on construction (from the tree context menu) or dropped onto the window.
///
/// Rendering is delegated to the charting factory so the dialog never depends on a
/// concrete charting backend. The window keeps its own subscription: it monitors on
/// show and stops monitoring on hide or when retargeted.
///
class NodeMonitorDialog : public AppBaseDialog
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the dialog and wires it to the backend.
    /// \param service OPC UA backend used to subscribe and read the node.
    /// \param parent Parent widget.
    ///
    explicit NodeMonitorDialog(OpcUaBackend *service, QWidget *parent = nullptr);

    ///
    /// \brief Destroys the dialog and its generated UI.
    ///
    ~NodeMonitorDialog() override;

    ///
    /// \brief Points the monitor at a variable node, replacing any previous target.
    /// \param node Variable node to monitor.
    ///
    void setTarget(const OpcUaNodeInfo &node);

    ///
    /// \brief Returns the NodeId of the monitored node, or empty when none is set.
    /// \return Monitored NodeId.
    ///
    QString nodeId() const;

    ///
    /// \brief Populates the subscription combo from the application's subscriptions.
    /// \param subscriptions Configured subscriptions, in display order.
    ///
    void setSubscriptions(const QVector<SubscriptionItem> &subscriptions);

    ///
    /// \brief Captures the monitored node, settings and placement for the session.
    /// \return Serializable snapshot of this monitor window.
    ///
    SessionNodeMonitor captureSession() const;

    ///
    /// \brief Restores a monitor's settings, placement and target from a session.
    /// \param state Snapshot previously produced by captureSession().
    ///
    void restoreSession(const SessionNodeMonitor &state);

signals:
    ///
    /// \brief Requests that a new subscription be created in the shared list.
    /// \param name New subscription name.
    /// \param publishingInterval Publishing interval in milliseconds.
    ///
    void subscriptionCreationRequested(QString name, double publishingInterval);

protected:
    void changeEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void setLivePaused(bool paused);
    void setAlwaysOnTop(bool onTop);
    void applySelectedSubscription();

private:
    void handleDataValues(const QVector<OpcUaDataValue> &values, const QString &error);
    void handleNodeDetails(const OpcUaNodeDetails &details, const QString &error);
    void applyValue(const OpcUaDataValue &value);
    void appendStep(qreal x, qreal y, const QString &status);
    void refeedChart();
    void applyChartOptions();
    void showSettingsMenu();
    void positionSettingsButton();
    void updateQualityBadge(const QString &status);
    void clearQualityBadge();
    void applyWindow();
    void applyTheme();
    void subscribeCurrent();
    void unsubscribeCurrent();
    void showPlaceholder();
    bool acceptsNodeDrag(const QMimeData *mimeData) const;
    bool dropNode(const QMimeData *mimeData);

    Ui::NodeMonitorDialog *ui;
    OpcUaBackend *_service;
    std::unique_ptr<IChartView> _chart;
    ThemedPushButton *_settingsButton = nullptr;
    TrendSeries _series;
    QTimer *_liveTimer = nullptr;
    QString _nodeId;
    QString _displayName;
    QString _displayPath;
    QString _typeText;
    bool _subscribed = false;
    bool _livePaused = false;
    bool _hasChartPoint = false;
    double _lastChartY = 0.0;
    double _publishingInterval = 1000.0;
    qint64 _windowMs = 60000;

    bool _autoScaleY = true;
    bool _stepLines = true;
    bool _showGrid = true;
    bool _showLegend = true;
    bool _showPoints = false;
    bool _showTooltip = true;
};
