// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file referenceplugin.cpp
/// \brief Implements the reference browse API and logging.
///

#include "referenceplugin.h"

#include <QLoggingCategory>

#include "opcua/opcuaclientservice.h"
#include "plugincontext.h"

namespace {
Q_LOGGING_CATEGORY(lcReference, "ouaexp.Reference")
}

ReferencePlugin::ReferencePlugin(QObject *parent)
    : Plugin(parent)
{
}

///
/// \brief Returns the plugin name shown in the startup log.
///
QString ReferencePlugin::name() const
{
    return tr("Reference Plugin");
}

///
/// \brief Returns the Reference logging category.
///
const QLoggingCategory &ReferencePlugin::logCategory() const
{
    return lcReference();
}

///
/// \brief Observes reference browse completions to log them and republish the references.
/// \param context Host context providing the client service.
///
void ReferencePlugin::initialize(PluginContext &context)
{
    _clientService = context.clientService();
    connect(_clientService, &OpcUaClientService::referencesBrowseFinished,
            this, &ReferencePlugin::handleReferencesFinished);
}

///
/// \brief Browses the forward references of a node.
/// \param nodeId Node to browse.
///
void ReferencePlugin::browseReferences(const QString &nodeId)
{
    _clientService->browseReferences(nodeId);
}

///
/// \brief Logs and republishes the outcome of a reference browse.
/// \param sourceNodeId Browsed node.
/// \param references Reference browse result.
/// \param error Browse error, empty on success.
///
void ReferencePlugin::handleReferencesFinished(const QString &sourceNodeId,
                                               const QVector<OpcUaNodeInfo> &references,
                                               const QString &error)
{
    if (error.isEmpty())
        qCInfo(lcReference).noquote()
            << tr("Browse references on node '%1' succeeded.").arg(sourceNodeId);
    else
        qCWarning(lcReference).noquote()
            << tr("Browse references on node '%1' failed: %2").arg(sourceNodeId, error);
    emit referencesReady(sourceNodeId, references, error);
}
