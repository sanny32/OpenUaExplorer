// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file eventsmodule.cpp
/// \brief Implements the OPC UA event-monitoring API and logging.
///

#include "eventsmodule.h"

#include <QLoggingCategory>

#include "opcua/opcuaclientservice.h"
#include "servicecontext.h"

namespace {
Q_LOGGING_CATEGORY(lcEvents, "ouaexp.Events")
}

EventsModule::EventsModule(QObject *parent)
    : ServiceModule(parent)
{
}

///
/// \brief Returns the module name shown in the startup log.
///
QString EventsModule::name() const
{
    return tr("Events Module");
}

///
/// \brief Returns the Events logging category.
///
const QLoggingCategory &EventsModule::logCategory() const
{
    return lcEvents();
}

///
/// \brief Observes event notifications and monitoring completions to republish and log them.
/// \param context Host context providing the client service.
///
void EventsModule::initialize(ServiceContext &context)
{
    _clientService = context.clientService();
    connect(_clientService, &OpcUaClientService::eventsReady,
            this, &EventsModule::handleEventsReady);
    connect(_clientService, &OpcUaClientService::eventMonitoringFinished,
            this, &EventsModule::handleEventMonitoringFinished);
}

///
/// \brief Logs and starts event monitoring for a node.
/// \param nodeId Node to monitor for events.
/// \param publishingInterval Publishing interval in milliseconds.
///
void EventsModule::subscribeEvents(const QString &nodeId, double publishingInterval)
{
    qCInfo(lcEvents).noquote()
        << tr("Creating event monitored item for node '%1' (publishingInterval=%2 ms).")
               .arg(nodeId).arg(publishingInterval);
    _clientService->subscribeEvents(nodeId, publishingInterval);
}

///
/// \brief Logs and cancels event monitoring for a node.
/// \param nodeId Node to stop monitoring.
///
void EventsModule::unsubscribeEvents(const QString &nodeId)
{
    qCInfo(lcEvents).noquote() << tr("Disabling event monitoring for node '%1'.").arg(nodeId);
    _clientService->unsubscribeEvents(nodeId);
}

///
/// \brief Republishes received events without logging individual notifications.
/// \param nodeId Monitored node that produced the events.
/// \param events Received events.
/// \param error Error description, empty on success.
///
void EventsModule::handleEventsReady(const QString &nodeId, const QVector<OpcUaEvent> &events,
                                     const QString &error)
{
    emit eventsReady(nodeId, events, error);
}

///
/// \brief Logs and republishes the outcome of an event-monitoring request.
/// \param nodeId Affected node.
/// \param subscribed True for subscribe and false for unsubscribe.
/// \param success Whether the request succeeded.
/// \param error Error description, empty on success.
///
void EventsModule::handleEventMonitoringFinished(const QString &nodeId, bool subscribed,
                                                 bool success, const QString &error)
{
    if (success) {
        qCInfo(lcEvents).noquote()
            << (subscribed ? tr("Event monitoring enabled for node '%1'.").arg(nodeId)
                           : tr("Event monitoring disabled for node '%1'.").arg(nodeId));
    } else {
        qCWarning(lcEvents).noquote()
            << (subscribed
                    ? tr("Failed to enable event monitoring for node '%1': %2").arg(nodeId, error)
                    : tr("Failed to disable event monitoring for node '%1': %2").arg(nodeId, error));
    }
    emit eventMonitoringFinished(nodeId, subscribed, success, error);
}
