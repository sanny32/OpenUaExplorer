// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file addressspaceplugin.cpp
/// \brief Implements the address-space browse API and logging.
///

#include "addressspaceplugin.h"

#include <QLoggingCategory>

#include "opcua/opcuaclientservice.h"
#include "opcua/standardnodeid.h"
#include "plugincontext.h"

namespace {
Q_LOGGING_CATEGORY(lcAddressSpace, "ouaexp.AddressSpace")
}

AddressSpacePlugin::AddressSpacePlugin(QObject *parent)
    : Plugin(parent)
{
}

///
/// \brief Returns the plugin name shown in the startup log.
///
QString AddressSpacePlugin::name() const
{
    return tr("Address Space Plugin");
}

///
/// \brief Returns the AddressSpace logging category.
///
const QLoggingCategory &AddressSpacePlugin::logCategory() const
{
    return lcAddressSpace();
}

///
/// \brief Observes browse completions to log them and republish the children.
/// \param context Host context providing the client service.
///
void AddressSpacePlugin::initialize(PluginContext &context)
{
    _clientService = context.clientService();
    connect(_clientService, &OpcUaClientService::browseFinished,
            this, &AddressSpacePlugin::handleBrowseFinished);
}

///
/// \brief Browses the children of a node.
/// \param nodeId Node to browse.
///
void AddressSpacePlugin::browse(const QString &nodeId)
{
    _clientService->browse(nodeId);
}

///
/// \brief Browses a node, defaulting to the Objects folder when none is given.
/// \param nodeId Node to browse, or empty for the Objects folder.
///
void AddressSpacePlugin::refresh(const QString &nodeId)
{
    _clientService->browse(nodeId.isEmpty()
        ? QString::fromLatin1(StandardNodeId::ObjectsFolder) : nodeId);
}

///
/// \brief Logs and republishes the outcome of a browse.
/// \param parentNodeId Browsed node.
/// \param children Browse result.
/// \param error Browse error, empty on success.
///
void AddressSpacePlugin::handleBrowseFinished(const QString &parentNodeId,
                                              const QVector<OpcUaNodeInfo> &children,
                                              const QString &error)
{
    if (error.isEmpty())
        qCInfo(lcAddressSpace).noquote() << tr("Browse on node '%1' succeeded.").arg(parentNodeId);
    else
        qCWarning(lcAddressSpace).noquote()
            << tr("Browse on node '%1' failed: %2").arg(parentNodeId, error);
    emit childrenReady(parentNodeId, children, error);
}
