// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file eventsmodule.h
/// \brief Declares the module that exposes the OPC UA event-monitoring API.
///

#pragma once

#include "opcua/opcuatypes.h"
#include "servicemodule.h"

class OpcUaClientService;

///
/// \brief Provides the event API: monitor live events and read event history.
///
/// subscribeEvents() and readHistory() are the logged choke points for event operations.
/// Streamed event notifications that arrive through eventsReady() are intentionally not logged.
///
class EventsModule : public ServiceModule
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs the events module.
    /// \param parent Owning QObject.
    ///
    explicit EventsModule(QObject *parent = nullptr);

    QString name() const override;
    const QLoggingCategory &logCategory() const override;
    void initialize(ServiceContext &context) override;

public slots:
    ///
    /// \brief Enables event monitoring for a node.
    /// \param nodeId Node to monitor for events.
    /// \param publishingInterval Publishing interval in milliseconds.
    ///
    void subscribeEvents(const QString &nodeId, double publishingInterval = 1000.0);

    ///
    /// \brief Disables event monitoring for a node.
    /// \param nodeId Node to stop monitoring.
    ///
    void unsubscribeEvents(const QString &nodeId);

    ///
    /// \brief Reads historical events for a single node over a time range.
    /// \param nodeId Node whose event history is read.
    /// \param start Inclusive range start.
    /// \param end Inclusive range end.
    /// \param maxValues Maximum events to return, or 0 for no limit.
    ///
    void readHistory(const QString &nodeId, const QDateTime &start, const QDateTime &end,
                     quint32 maxValues);

signals:
    ///
    /// \brief Emitted when events arrive for a monitored node.
    /// \param nodeId Monitored node that produced the events.
    /// \param events Received events.
    /// \param error Error description, empty on success.
    ///
    void eventsReady(QString nodeId, QVector<OpcUaEvent> events, QString error);

    ///
    /// \brief Emitted when an event history read finishes.
    /// \param nodeId Node whose event history was read.
    /// \param events Historical events in server order.
    /// \param error Read error, empty on success.
    ///
    void eventsHistoryReady(QString nodeId, QVector<OpcUaEvent> events, QString error);

    ///
    /// \brief Emitted when an event-monitoring request finishes.
    /// \param nodeId Affected node.
    /// \param subscribed True for subscribe and false for unsubscribe.
    /// \param success Whether the request succeeded.
    /// \param error Error description, empty on success.
    ///
    void eventMonitoringFinished(QString nodeId, bool subscribed, bool success, QString error);

private:
    void handleEventsReady(const QString &nodeId, const QVector<OpcUaEvent> &events,
                           const QString &error);
    void handleEventsHistoryReady(const QString &nodeId, const QVector<OpcUaEvent> &events,
                                  const QString &error);
    void handleEventMonitoringFinished(const QString &nodeId, bool subscribed,
                                       bool success, const QString &error);

    OpcUaClientService *_clientService = nullptr;
};
