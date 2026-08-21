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

class QOpcUaGenericStructHandler;

namespace QtOpcUaTypeMapper {

using Translate = std::function<QString(const char *)>;

/// \brief Replaces the opaque structures in a value by their decoded fields.
///
/// Values the handler cannot decode, and every value while no handler is ready, are
/// returned unchanged: an undecoded structure is still shown, only not expanded.
QVariant decodedValue(const QVariant &value, const QOpcUaGenericStructHandler *handler);

/// \brief Reports whether a value still carries a structure that was not decoded.
///
/// Such a value is worth reading again once the server's type definitions are available.
bool containsOpaqueStruct(const QVariant &value);

/// \brief Lists the encoding ids of the structures in a value that were not decoded.
QStringList opaqueEncodingIds(const QVariant &value);

/// \brief Lists the named values of an enumeration DataType.
///
/// The names come from the type definitions the handler read when the session opened, so
/// resolving them costs no extra request. A DataType that is not an enumeration, and every
/// DataType while no handler is ready, resolve to an empty list.
OpcUaEnumEntries enumEntries(const QString &dataTypeId,
                             const QOpcUaGenericStructHandler *handler);

/// \brief Lets structures with a field of the abstract Enumeration type decode.
///
/// Qt's decoder refuses such a field because the type is abstract, although the binary
/// encoding carries every enumeration as an Int32. Declaring Enumeration concrete makes
/// the decoder read those four bytes and finish the structure.
void allowAbstractEnumerationFields(QOpcUaGenericStructHandler *handler);

///
/// \brief Rewrites scalar aliases in standard Server diagnostic structures to built-in types.
/// \param handler Initialized structure handler to update.
///
void allowStandardDiagnosticScalarAliases(QOpcUaGenericStructHandler *handler);

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
                             const Translate &translate,
                             const QOpcUaGenericStructHandler *structHandler = nullptr);
                             
/// \brief Resolves this client's session name from SessionDiagnosticsArray.
QString ownSessionName(const QVariant &value, const QString &applicationUri);

} // namespace QtOpcUaTypeMapper
