// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file eventswidget.h
/// \brief Declares the OPC UA events tab widget.
///

#pragma once

#include <QWidget>

#include "appsettings.h"
#include "opcua/opcuatypes.h"

namespace Ui {
class EventsWidget;
}

class EventsModel;

///
/// \brief Tab widget that displays OPC UA event entries.
///
class EventsWidget : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the events widget and its table view.
    /// \param parent Parent widget.
    ///
    explicit EventsWidget(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the widget and its generated UI.
    ///
    ~EventsWidget() override;

    ///
    /// \brief Targets a node as the event source and enables subscribing.
    /// \param nodeId Node to monitor for events.
    /// \param displayName Human-readable name shown in the source field.
    /// \param displayPath Human-readable path shown in the source field.
    ///
    void setEventSource(const QString &nodeId, const QString &displayName,
                        const QString &displayPath = {});

    ///
    /// \brief Requests event monitoring for the current source node.
    ///
    void requestEventMonitoring();

    ///
    /// \brief Builds the default CSV export file name for displayed events.
    /// \return Suggested CSV file name.
    ///
    QString suggestedEventsCsvFileName() const;

    ///
    /// \brief Appends received events to the table.
    /// \param events Events to display.
    ///
    void appendEvents(const QVector<OpcUaEvent> &events);

    ///
    /// \brief Updates the toolbar button states for a node's monitoring outcome.
    /// \param nodeId Affected node.
    /// \param subscribed Whether the node is now monitored for events.
    ///
    void setEventMonitoringState(const QString &nodeId, bool subscribed);

    ///
    /// \brief Removes all displayed events.
    ///
    void clear();

    ///
    /// \brief Persists the events table header state.
    /// \param settings Settings store to write to.
    ///
    void saveViewState(AppSettings &settings) const;

    ///
    /// \brief Restores the events table header state.
    /// \param settings Settings store to read from.
    ///
    void restoreViewState(AppSettings &settings);

signals:
    ///
    /// \brief Emitted when the user requests event monitoring for the source node.
    /// \param nodeId Node to monitor for events.
    /// \param publishingInterval Publishing interval in milliseconds.
    ///
    void eventSubscribeRequested(QString nodeId, double publishingInterval);

    ///
    /// \brief Emitted when the user requests to stop event monitoring.
    /// \param nodeId Node to stop monitoring.
    ///
    void eventUnsubscribeRequested(QString nodeId);

private:
    void setupEventsView();
    void configureToolbar();
    void updateActionButtons();
    void clearEventSource();
    void exportEventsToCsv();

    Ui::EventsWidget *ui;
    EventsModel      *_eventsModel;
    bool              _subscribed = false;
};
