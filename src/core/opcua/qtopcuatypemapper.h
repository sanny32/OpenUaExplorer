// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <functional>

#include <QList>
#include <QString>
#include <QVariant>
#include <QVector>

#include <QOpcUaApplicationDescription>
#include <QOpcUaEndpointDescription>
#include <QOpcUaNode>
#include <QOpcUaReferenceDescription>

#include "opcuatypes.h"

namespace QtOpcUaTypeMapper {

using Translate = std::function<QString(const char *)>;

/// \brief Maps discovered Qt endpoints to transport-neutral endpoint records.
QList<EndpointInfo> endpointInfos(const QVector<QOpcUaEndpointDescription> &endpoints);

/// \brief Maps Qt application descriptions to transport-neutral server records.
QList<ServerInfo> serverInfos(const QVector<QOpcUaApplicationDescription> &servers);

/// \brief Maps Qt browse references to transport-neutral node records.
QVector<OpcUaNodeInfo> nodeInfos(const QVector<QOpcUaReferenceDescription> &references);

/// \brief Returns the complete attribute mask used by node-detail reads.
QOpcUa::NodeAttributes nodeDetailAttributes();

/// \brief Builds formatted node details from attributes cached by a Qt node.
OpcUaNodeDetails nodeDetails(QOpcUaNode *node, const QString &nodeId,
                             QOpcUa::NodeAttributes attributes,
                             const Translate &translate);
                             
/// \brief Resolves this client's session name from SessionDiagnosticsArray.
QString ownSessionName(const QVariant &value, const QString &applicationUri);

} // namespace QtOpcUaTypeMapper
