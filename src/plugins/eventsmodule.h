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
/// \brief Provides the event-monitoring API: subscribe to and stream OPC UA events.
///
/// subscribeEvents() is the single logged choke point for event monitoring; the events
/// widget invokes it directly. Streamed event notifications that arrive through
/// eventsReady() are intentionally not logged.
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

signals:
    ///
    /// \brief Emitted when events arrive for a monitored node.
    /// \param nodeId Monitored node that produced the events.
    /// \param events Received events.
    /// \param error Error description, empty on success.
    ///
    void eventsReady(QString nodeId, QVector<OpcUaEvent> events, QString error);

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
    void handleEventMonitoringFinished(const QString &nodeId, bool subscribed,
                                       bool success, const QString &error);

    OpcUaClientService *_clientService = nullptr;
};
