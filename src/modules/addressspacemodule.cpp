// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file addressspacemodule.cpp
/// \brief Implements the address-space browse API and logging.
///

#include "addressspacemodule.h"

#include <QLoggingCategory>

#include "opcua/opcuaclientservice.h"
#include "opcua/standardnodeid.h"
#include "servicecontext.h"

namespace {
Q_LOGGING_CATEGORY(lcAddressSpace, "ouaexp.AddressSpace")
}

AddressSpaceModule::AddressSpaceModule(QObject *parent)
    : ServiceModule(parent)
{
}

///
/// \brief Returns the module name shown in the startup log.
///
QString AddressSpaceModule::name() const
{
    return tr("Address Space Module");
}

///
/// \brief Returns the AddressSpace logging category.
///
const QLoggingCategory &AddressSpaceModule::logCategory() const
{
    return lcAddressSpace();
}

///
/// \brief Observes browse completions to log them and republish the children.
/// \param context Host context providing the client service.
///
void AddressSpaceModule::initialize(ServiceContext &context)
{
    _clientService = context.clientService();
    connect(_clientService, &OpcUaClientService::browseFinished,
            this, &AddressSpaceModule::handleBrowseFinished);
    connect(_clientService, &OpcUaClientService::nodeSearchProgress,
            this, &AddressSpaceModule::searchProgress);
    connect(_clientService, &OpcUaClientService::nodeSearchFinished,
            this, &AddressSpaceModule::handleSearchFinished);
}

///
/// \brief Browses the children of a node.
/// \param nodeId Node to browse.
///
void AddressSpaceModule::browse(const QString &nodeId)
{
    _clientService->browse(nodeId);
}

///
/// \brief Browses a node, defaulting to the Objects folder when none is given.
/// \param nodeId Node to browse, or empty for the Objects folder.
///
void AddressSpaceModule::refresh(const QString &nodeId)
{
    _clientService->browse(nodeId.isEmpty()
        ? QString::fromLatin1(StandardNodeId::ObjectsFolder) : nodeId);
}

///
/// \brief Searches a subtree on the server for a node whose display name matches.
/// \param startNodeId Node whose subtree is searched.
/// \param pattern Case-insensitive substring matched against display names.
///
void AddressSpaceModule::search(const QString &startNodeId, const QString &pattern)
{
    qCInfo(lcAddressSpace).noquote()
        << tr("Searching node '%1' for '%2'.").arg(startNodeId, pattern);
    _clientService->searchNode(startNodeId, pattern);
}

///
/// \brief Cancels an in-progress node search, if any.
///
void AddressSpaceModule::cancelSearch()
{
    _clientService->cancelNodeSearch();
}

///
/// \brief Logs and republishes the outcome of a search.
/// \param ancestorNodeIds Node ids from the start node down to the match's parent.
/// \param nodeId Matched NodeId, empty when nothing matched.
/// \param error Search error, empty on success.
///
void AddressSpaceModule::handleSearchFinished(const QStringList &ancestorNodeIds,
                                              const QString &nodeId,
                                              const QString &error)
{
    if (!error.isEmpty())
        qCWarning(lcAddressSpace).noquote() << tr("Search failed: %1").arg(error);
    else if (nodeId.isEmpty())
        qCInfo(lcAddressSpace).noquote() << tr("Search found no matching node.");
    else
        qCInfo(lcAddressSpace).noquote() << tr("Search found node '%1'.").arg(nodeId);
    emit searchFinished(ancestorNodeIds, nodeId, error);
}

///
/// \brief Logs and republishes the outcome of a browse.
/// \param parentNodeId Browsed node.
/// \param children Browse result.
/// \param error Browse error, empty on success.
///
void AddressSpaceModule::handleBrowseFinished(const QString &parentNodeId,
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
