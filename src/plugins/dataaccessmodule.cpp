// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dataaccessmodule.cpp
/// \brief Implements the data-access read and monitoring API and logging.
///

#include "dataaccessmodule.h"

#include <QLoggingCategory>

#include "opcua/opcuaclientservice.h"
#include "servicecontext.h"

namespace {
Q_LOGGING_CATEGORY(lcDataAccess, "ouaexp.DataAccess")
}

DataAccessModule::DataAccessModule(QObject *parent)
    : ServiceModule(parent)
{
}

///
/// \brief Returns the module name shown in the startup log.
///
QString DataAccessModule::name() const
{
    return tr("Data Access Module");
}

///
/// \brief Returns the DataAccess logging category.
///
const QLoggingCategory &DataAccessModule::logCategory() const
{
    return lcDataAccess();
}

///
/// \brief Observes value reads and monitoring completions to republish and log them.
/// \param context Host context providing the client service.
///
void DataAccessModule::initialize(ServiceContext &context)
{
    _clientService = context.clientService();
    connect(_clientService, &OpcUaClientService::dataValuesReady,
            this, &DataAccessModule::handleValuesReady);
    connect(_clientService, &OpcUaClientService::monitoringFinished,
            this, &DataAccessModule::handleMonitoringFinished);
}

///
/// \brief Reads the values of several nodes.
/// \param nodeIds Nodes to read.
///
void DataAccessModule::read(const QStringList &nodeIds)
{
    _clientService->readValues(nodeIds);
}

///
/// \brief Logs and starts monitoring for a node.
/// \param nodeId Node to monitor.
/// \param publishingInterval Publishing interval in milliseconds.
///
void DataAccessModule::subscribe(const QString &nodeId, double publishingInterval)
{
    qCInfo(lcDataAccess).noquote()
        << tr("Creating monitored item for node '%1' (publishingInterval=%2 ms).")
               .arg(nodeId).arg(publishingInterval);
    _clientService->subscribe(nodeId, publishingInterval);
}

///
/// \brief Logs and cancels monitoring for a node.
/// \param nodeId Node to stop monitoring.
///
void DataAccessModule::unsubscribe(const QString &nodeId)
{
    qCInfo(lcDataAccess).noquote() << tr("Disabling monitoring for node '%1'.").arg(nodeId);
    _clientService->unsubscribe(nodeId);
}

///
/// \brief Republishes read or monitored values without logging streamed updates.
/// \param values Latest values.
/// \param error Read error, empty on success.
///
void DataAccessModule::handleValuesReady(const QVector<OpcUaDataValue> &values, const QString &error)
{
    emit valuesReady(values, error);
}

///
/// \brief Logs and republishes the outcome of a monitoring request.
/// \param nodeId Affected node.
/// \param subscribed True for subscribe and false for unsubscribe.
/// \param success Whether the request succeeded.
/// \param error Error description, empty on success.
///
void DataAccessModule::handleMonitoringFinished(const QString &nodeId, bool subscribed,
                                                bool success, const QString &error)
{
    if (success) {
        qCInfo(lcDataAccess).noquote()
            << (subscribed ? tr("Monitoring enabled for node '%1'.").arg(nodeId)
                           : tr("Monitoring disabled for node '%1'.").arg(nodeId));
    } else {
        qCWarning(lcDataAccess).noquote()
            << (subscribed ? tr("Failed to enable monitoring for node '%1': %2").arg(nodeId, error)
                           : tr("Failed to disable monitoring for node '%1': %2").arg(nodeId, error));
    }
    emit monitoringFinished(nodeId, subscribed, success, error);
}
