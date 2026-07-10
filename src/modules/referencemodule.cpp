// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file referencemodule.cpp
/// \brief Implements the reference browse API and logging.
///

#include "referencemodule.h"

#include <QLoggingCategory>

#include "opcua/opcuaclientservice.h"
#include "servicecontext.h"

namespace {
Q_LOGGING_CATEGORY(lcReference, "ouaexp.Reference")
}

ReferenceModule::ReferenceModule(QObject *parent)
    : ServiceModule(parent)
{
}

///
/// \brief Returns the module name shown in the startup log.
///
QString ReferenceModule::name() const
{
    return tr("Reference Module");
}

///
/// \brief Returns the Reference logging category.
///
const QLoggingCategory &ReferenceModule::logCategory() const
{
    return lcReference();
}

///
/// \brief Observes reference browse completions to log them and republish the references.
/// \param context Host context providing the client service.
///
void ReferenceModule::initialize(ServiceContext &context)
{
    _clientService = context.clientService();
    connect(_clientService, &OpcUaClientService::referencesBrowseFinished,
            this, &ReferenceModule::handleReferencesFinished);
}

///
/// \brief Browses the forward references of a node.
/// \param nodeId Node to browse.
///
void ReferenceModule::browseReferences(const QString &nodeId)
{
    _clientService->browseReferences(nodeId);
}

///
/// \brief Logs and republishes the outcome of a reference browse.
/// \param sourceNodeId Browsed node.
/// \param references Reference browse result.
/// \param error Browse error, empty on success.
///
void ReferenceModule::handleReferencesFinished(const QString &sourceNodeId,
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
