// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file attributeplugin.cpp
/// \brief Implements the attribute read/write API and logging.
///

#include "attributeplugin.h"

#include <QLoggingCategory>

#include "opcua/opcuaclientservice.h"
#include "plugincontext.h"

namespace {
Q_LOGGING_CATEGORY(lcAttribute, "ouaexp.Attribute")
}

AttributePlugin::AttributePlugin(QObject *parent)
    : Plugin(parent)
{
}

///
/// \brief Returns the plugin name shown in the startup log.
///
QString AttributePlugin::name() const
{
    return tr("Attribute Plugin");
}

///
/// \brief Returns the Attribute logging category.
///
const QLoggingCategory &AttributePlugin::logCategory() const
{
    return lcAttribute();
}

///
/// \brief Observes read and write completions to log them and republish the results.
/// \param context Host context providing the client service.
///
void AttributePlugin::initialize(PluginContext &context)
{
    _clientService = context.clientService();
    connect(_clientService, &OpcUaClientService::nodeDetailsReady,
            this, &AttributePlugin::handleNodeDetailsReady);
    connect(_clientService, &OpcUaClientService::writeFinished,
            this, &AttributePlugin::handleWriteFinished);
}

///
/// \brief Reads a node's attribute set.
/// \param nodeId Node to read.
///
void AttributePlugin::read(const QString &nodeId)
{
    _clientService->readNode(nodeId);
}

///
/// \brief Writes a node's value.
/// \param nodeId Node to write.
/// \param value Value to write.
/// \param valueType OPC UA type of the value.
///
void AttributePlugin::write(const QString &nodeId, const QVariant &value, int valueType)
{
    _clientService->writeValue(nodeId, value, valueType);
}

///
/// \brief Logs and republishes the outcome of an attribute read.
/// \param details Read node details.
/// \param error Read error, empty on success.
///
void AttributePlugin::handleNodeDetailsReady(const OpcUaNodeDetails &details, const QString &error)
{
    if (error.isEmpty())
        qCInfo(lcAttribute).noquote()
            << tr("Read attributes of node '%1' succeeded.").arg(details.nodeId);
    else
        qCWarning(lcAttribute).noquote() << tr("Read attributes failed: %1").arg(error);
    emit attributesReady(details, error);
}

///
/// \brief Logs and republishes the outcome of a value write.
/// \param nodeId Written node.
/// \param success Whether the write succeeded.
/// \param error Write error, empty on success.
///
void AttributePlugin::handleWriteFinished(const QString &nodeId, bool success, const QString &error)
{
    if (success)
        qCInfo(lcAttribute).noquote() << tr("Write to node '%1' succeeded.").arg(nodeId);
    else
        qCWarning(lcAttribute).noquote()
            << tr("Write to node '%1' failed: %2").arg(nodeId, error);
    emit writeFinished(nodeId, success, error);
}
