// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include "qtopcuatypemapper.h"

#include <QOpcUaApplicationDescription>
#include <QOpcUaBinaryDataEncoding>
#include <QOpcUaExtensionObject>
#include <QOpcUaLocalizedText>
#include <QOpcUaQualifiedName>
#include <QOpcUaUserTokenPolicy>

#include "formatters/attributeformatter.h"

using namespace OpcUaFormat;

namespace {

struct SessionDiagnostics
{
    QString sessionName;
    QString applicationUri;
    QDateTime connectionTime;
    bool valid = false;
};

/// \brief Decodes the leading fields used to identify a session.
SessionDiagnostics decodeSessionDiagnostics(QOpcUaExtensionObject object)
{
    SessionDiagnostics result;
    QOpcUaBinaryDataEncoding decoder(object);
    bool step = false;
    bool ok = true;

    decoder.decode<QString, QOpcUa::Types::NodeId>(step); ok &= step;
    result.sessionName = decoder.decode<QString>(step); ok &= step;
    result.applicationUri = decoder.decode<QString>(step); ok &= step;
    decoder.decode<QString>(step); ok &= step;
    decoder.decode<QOpcUaLocalizedText>(step); ok &= step;
    decoder.decode<quint32>(step); ok &= step;
    decoder.decode<QString>(step); ok &= step;
    decoder.decode<QString>(step); ok &= step;
    decoder.decodeArray<QString>(step); ok &= step;
    decoder.decode<QString>(step); ok &= step;
    decoder.decode<QString>(step); ok &= step;
    decoder.decodeArray<QString>(step); ok &= step;
    decoder.decode<double>(step); ok &= step;
    decoder.decode<quint32>(step); ok &= step;
    result.connectionTime = decoder.decode<QDateTime>(step); ok &= step;
    result.valid = ok;
    return result;
}

/// \brief Returns the ordered node-attribute table shown by the UI.
QList<QPair<QString, QOpcUa::NodeAttribute>> attributeFields(
    const QtOpcUaTypeMapper::Translate &translate)
{
    QList<QPair<QString, QOpcUa::NodeAttribute>> fields = {
        {translate("Node Id"), QOpcUa::NodeAttribute::NodeId},
        {translate("Node Class"), QOpcUa::NodeAttribute::NodeClass},
        {translate("Browse Name"), QOpcUa::NodeAttribute::BrowseName},
        {translate("Display Name"), QOpcUa::NodeAttribute::DisplayName},
        {translate("Description"), QOpcUa::NodeAttribute::Description},
        {translate("Is Abstract"), QOpcUa::NodeAttribute::IsAbstract},
        {translate("Symmetric"), QOpcUa::NodeAttribute::Symmetric},
        {translate("Inverse Name"), QOpcUa::NodeAttribute::InverseName},
        {translate("Contains No Loops"), QOpcUa::NodeAttribute::ContainsNoLoops},
        {translate("Event Notifier"), QOpcUa::NodeAttribute::EventNotifier},
        {translate("Value"), QOpcUa::NodeAttribute::Value},
        {translate("Data Type"), QOpcUa::NodeAttribute::DataType},
        {translate("Value Rank"), QOpcUa::NodeAttribute::ValueRank},
        {translate("Array Dimensions"), QOpcUa::NodeAttribute::ArrayDimensions},
        {translate("Access Level"), QOpcUa::NodeAttribute::AccessLevel},
        {translate("User Access Level"), QOpcUa::NodeAttribute::UserAccessLevel},
        {translate("Minimum Sampling Interval"), QOpcUa::NodeAttribute::MinimumSamplingInterval},
        {translate("Historizing"), QOpcUa::NodeAttribute::Historizing},
        {translate("Executable"), QOpcUa::NodeAttribute::Executable},
        {translate("User Executable"), QOpcUa::NodeAttribute::UserExecutable},
        {translate("Write Mask"), QOpcUa::NodeAttribute::WriteMask},
        {translate("User Write Mask"), QOpcUa::NodeAttribute::UserWriteMask}
    };
    fields.append(qMakePair(translate("Role Permissions"), QOpcUa::NodeAttribute::RolePermissions));
    fields.append(qMakePair(translate("User Role Permissions"), QOpcUa::NodeAttribute::UserRolePermissions));
    fields.append(qMakePair(translate("Access Restrictions"), QOpcUa::NodeAttribute::AccessRestrictions));
    return fields;
}

} // namespace

namespace QtOpcUaTypeMapper {

/// \brief Maps discovered Qt endpoints to transport-neutral endpoint records.
QList<EndpointInfo> endpointInfos(const QVector<QOpcUaEndpointDescription> &endpoints)
{
    QList<EndpointInfo> result;
    result.reserve(endpoints.size());
    for (int i = 0; i < endpoints.size(); ++i) {
        const QOpcUaEndpointDescription &endpoint = endpoints.at(i);
        EndpointInfo info;
        info.index = i;
        info.endpointUrl = endpoint.endpointUrl();
        info.securityPolicy = endpoint.securityPolicy();
        info.securityMode = securityModeName(endpoint.securityMode());
        info.securityModeValue = static_cast<int>(endpoint.securityMode());
        info.serverCertificate = endpoint.serverCertificate();
        for (const QOpcUaUserTokenPolicy &token : endpoint.userIdentityTokens()) {
            info.supportsAnonymous |= token.tokenType() == QOpcUaUserTokenPolicy::Anonymous;
            info.supportsUsername |= token.tokenType() == QOpcUaUserTokenPolicy::Username;
            info.supportsCertificate |= token.tokenType() == QOpcUaUserTokenPolicy::Certificate;
        }
        result.append(info);
    }
    return result;
}

/// \brief Maps Qt browse references to transport-neutral node records.
QVector<OpcUaNodeInfo> nodeInfos(const QVector<QOpcUaReferenceDescription> &references)
{
    QVector<OpcUaNodeInfo> result;
    result.reserve(references.size());
    for (const QOpcUaReferenceDescription &reference : references) {
        OpcUaNodeInfo info;
        info.nodeId = reference.targetNodeId().nodeId();
        info.browseName = reference.browseName().name();
        info.displayName = reference.displayName().text();
        info.referenceTypeId = reference.refTypeId();
        info.nodeClass = static_cast<int>(reference.nodeClass());
        result.append(info);
    }
    return result;
}

/// \brief Returns the complete attribute mask used by node-detail reads.
QOpcUa::NodeAttributes nodeDetailAttributes()
{
    return QOpcUaNode::allBaseAttributes()
        | QOpcUa::NodeAttribute::IsAbstract | QOpcUa::NodeAttribute::Symmetric
        | QOpcUa::NodeAttribute::InverseName | QOpcUa::NodeAttribute::ContainsNoLoops
        | QOpcUa::NodeAttribute::EventNotifier | QOpcUa::NodeAttribute::Description
        | QOpcUa::NodeAttribute::Value | QOpcUa::NodeAttribute::DataType
        | QOpcUa::NodeAttribute::ValueRank | QOpcUa::NodeAttribute::ArrayDimensions
        | QOpcUa::NodeAttribute::AccessLevel | QOpcUa::NodeAttribute::UserAccessLevel
        | QOpcUa::NodeAttribute::MinimumSamplingInterval | QOpcUa::NodeAttribute::Historizing
        | QOpcUa::NodeAttribute::Executable | QOpcUa::NodeAttribute::UserExecutable;
}

/// \brief Builds formatted node details from attributes cached by a Qt node.
OpcUaNodeDetails nodeDetails(QOpcUaNode *node, const QString &nodeId,
                             QOpcUa::NodeAttributes attributes, const Translate &translate)
{
    OpcUaNodeDetails details;
    details.nodeId = nodeId;
    details.nodeClass = node->attribute(QOpcUa::NodeAttribute::NodeClass).toInt();
    const auto nodeClass = static_cast<QOpcUa::NodeClass>(details.nodeClass);
    details.value = node->attribute(QOpcUa::NodeAttribute::Value);
    details.dataTypeId = node->attribute(QOpcUa::NodeAttribute::DataType).toString();
    details.valueType = static_cast<int>(valueTypeForDataType(details.dataTypeId));
    const auto valueType = static_cast<QOpcUa::Types>(details.valueType);
    details.valueRank = node->attribute(QOpcUa::NodeAttribute::ValueRank).toInt();
    for (const QVariant &dimension : node->attribute(QOpcUa::NodeAttribute::ArrayDimensions).toList())
        details.arrayDimensions.append(dimension.toUInt());
    details.accessLevel = static_cast<quint8>(node->attribute(QOpcUa::NodeAttribute::AccessLevel).toUInt());
    details.userAccessLevel = static_cast<quint8>(node->attribute(QOpcUa::NodeAttribute::UserAccessLevel).toUInt());
    details.historizing = node->attribute(QOpcUa::NodeAttribute::Historizing).toBool();
    details.eventNotifier = static_cast<quint8>(node->attribute(QOpcUa::NodeAttribute::EventNotifier).toUInt());
    details.status = statusName(node->attributeError(QOpcUa::NodeAttribute::Value));
    details.sourceTimestamp = node->sourceTimestamp(QOpcUa::NodeAttribute::Value);
    details.serverTimestamp = node->serverTimestamp(QOpcUa::NodeAttribute::Value);

    for (const auto &field : attributeFields(translate)) {
        if (!(attributes & field.second)
            || !attributeAppliesToNodeClass(field.second, nodeClass)
            || node->attributeError(field.second) == QOpcUa::UaStatusCode::BadAttributeIdInvalid) {
            continue;
        }
        const QVariant value = node->attribute(field.second);
        OpcUaNodeAttribute attribute;
        attribute.name = field.first;
        attribute.value = value;
        attribute.status = statusName(node->attributeError(field.second));
        attribute.sourceTimestamp = node->sourceTimestamp(field.second);
        attribute.serverTimestamp = node->serverTimestamp(field.second);
        formatAttribute(&attribute, field.second, value, valueType);
        if (field.second == QOpcUa::NodeAttribute::Value) {
            if (attribute.sourceTimestamp.isValid()) {
                OpcUaNodeAttribute timestamp = childAttribute(translate("Source Timestamp"), QString());
                timestamp.sourceTimestamp = attribute.sourceTimestamp;
                attribute.children.append(timestamp);
            }
            if (attribute.serverTimestamp.isValid()) {
                OpcUaNodeAttribute timestamp = childAttribute(translate("Server Timestamp"), QString());
                timestamp.serverTimestamp = attribute.serverTimestamp;
                attribute.children.append(timestamp);
            }
            attribute.children.append(childAttribute(translate("Status Code"), statusDisplay(node->attributeError(field.second))));
            attribute.children.append(valueAttribute(value, valueType));
        }
        details.attributes.append(attribute);
    }
    const QVariant displayName = node->attribute(QOpcUa::NodeAttribute::DisplayName);
    if (displayName.canConvert<QOpcUaLocalizedText>())
        details.displayName = displayName.value<QOpcUaLocalizedText>().text();
    return details;
}

/// \brief Resolves this client's session name from SessionDiagnosticsArray.
QString ownSessionName(const QVariant &value, const QString &applicationUri)
{
    QList<QOpcUaExtensionObject> objects;
    if (value.canConvert<QList<QOpcUaExtensionObject>>()) {
        objects = value.value<QList<QOpcUaExtensionObject>>();
    } else {
        for (const QVariant &entry : value.toList())
            if (entry.canConvert<QOpcUaExtensionObject>())
                objects.append(entry.value<QOpcUaExtensionObject>());
    }

    QString matchedName;
    QDateTime matchedTime;
    QString latestName;
    QDateTime latestTime;
    for (const QOpcUaExtensionObject &object : objects) {
        const SessionDiagnostics diagnostics = decodeSessionDiagnostics(object);
        if (!diagnostics.valid)
            continue;
        if (latestName.isEmpty() || diagnostics.connectionTime > latestTime) {
            latestName = diagnostics.sessionName;
            latestTime = diagnostics.connectionTime;
        }
        if (applicationUri.isEmpty() || diagnostics.applicationUri != applicationUri)
            continue;
        if (matchedName.isEmpty() || diagnostics.connectionTime > matchedTime) {
            matchedName = diagnostics.sessionName;
            matchedTime = diagnostics.connectionTime;
        }
    }
    return matchedName.isEmpty() ? latestName : matchedName;
}

} // namespace QtOpcUaTypeMapper
