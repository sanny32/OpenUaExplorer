// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file opcuatypes.h
/// \brief Defines transport-neutral data objects used by the OPC UA UI.
///

#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>
#include <QtGlobal>
#include <QVariant>
#include <QVector>

///
/// \brief Named OPC UA constants and small helpers shared across the UI.
///
/// Avoids scattering raw bit masks and the well-known Objects-folder NodeId
/// throughout the code. Values follow the OPC UA specification (Part 3/Part 6).
///
namespace OpcUa {

///
/// \brief NodeClass bit values (OPC UA Part 3).
///
enum NodeClass : int {
    Object   = 1,
    Variable = 2,
    Method   = 4,
};

///
/// \brief AccessLevel / UserAccessLevel bits (OPC UA Part 3).
///
enum AccessLevel : quint8 {
    CurrentRead  = 0x01,
    CurrentWrite = 0x02,
};

///
/// \brief Returns true when a NodeClass mask denotes a Variable node.
///
inline bool isVariable(int nodeClass) { return (nodeClass & Variable) != 0; }

///
/// \brief Returns true when a NodeClass mask denotes a Method node.
///
inline bool isMethod(int nodeClass) { return (nodeClass & Method) != 0; }

///
/// \brief Returns true when a UserAccessLevel mask grants current-write access.
///
inline bool isWritable(quint8 userAccessLevel)
{
    return (userAccessLevel & CurrentWrite) != 0;
}

///
/// \brief Returns true when Qt OPC UA provides raw HistoryRead APIs.
///
inline constexpr bool isHistoryReadSupported()
{
    return true;
}

} // namespace OpcUa

///
/// \brief Connection lifecycle state exposed to the UI.
///
enum class OpcUaConnectionState {
    /// \brief No OPC UA backend is available.
    Unavailable,
    /// \brief Client is idle and disconnected.
    Disconnected,
    /// \brief Endpoint discovery is in progress.
    Discovering,
    /// \brief A connection attempt is in progress.
    Connecting,
    /// \brief Client is connected to an endpoint.
    Connected,
    /// \brief Client is disconnecting from an endpoint.
    Closing
};

///
/// \brief User token capabilities advertised by an endpoint.
///
struct EndpointInfo
{
    /// \brief Original endpoint index in the discovery result.
    int index = -1;
    /// \brief Endpoint URL advertised by the server.
    QString endpointUrl;
    /// \brief Security policy URI or display name.
    QString securityPolicy;
    /// \brief Message security mode display name.
    QString securityMode;
    /// \brief Message security mode numeric value.
    int securityModeValue = 1;
    /// \brief Whether anonymous user tokens are supported.
    bool supportsAnonymous = false;
    /// \brief Whether username user tokens are supported.
    bool supportsUsername = false;
    /// \brief Whether certificate user tokens are supported.
    bool supportsCertificate = false;
    /// \brief DER-encoded server certificate.
    QByteArray serverCertificate;
};

///
/// \brief One browsed OPC UA node.
///
struct OpcUaNodeInfo
{
    /// \brief NodeId string.
    QString nodeId;
    /// \brief BrowseName display text.
    QString browseName;
    /// \brief DisplayName text.
    QString displayName;
    /// \brief Display path built from browsed parent nodes.
    QString displayPath;
    /// \brief Reference type that led to this node.
    QString referenceTypeId;
    /// \brief OPC UA NodeClass numeric value.
    int nodeClass = 0;
    /// \brief EventNotifier bit mask (Object/View nodes only; 0 otherwise).
    quint8 eventNotifier = 0;
    /// \brief Whether the server stores historical values (Variable nodes only).
    bool historizing = false;
    /// \brief Whether the node may have children.
    bool hasChildren = true;
};

///
/// \brief One input or output argument of an OPC UA method.
///
struct OpcUaMethodArgument
{
    /// \brief Argument name.
    QString name;
    /// \brief DataType NodeId string.
    QString dataTypeId;
    /// \brief QOpcUa::Types numeric value mapped from the DataType.
    int valueType = 0;
    /// \brief OPC UA ValueRank (-1 for scalar arguments).
    int valueRank = -1;
    /// \brief Argument Description text.
    QString description;
    /// \brief Argument value; carries an output value, unused for input metadata.
    QVariant value;
};

///
/// \brief One displayable OPC UA node attribute.
///
struct OpcUaNodeAttribute
{
    /// \brief Attribute name.
    QString name;
    /// \brief Raw attribute value.
    QVariant value;
    /// \brief Formatted value shown in the UI.
    QString displayValue;
    /// \brief Status display text.
    QString status;
    /// \brief Source timestamp, when provided.
    QDateTime sourceTimestamp;
    /// \brief Server timestamp, when provided.
    QDateTime serverTimestamp;
    /// \brief Nested attribute details.
    QVector<OpcUaNodeAttribute> children;
};

///
/// \brief Complete selected-node information returned by a read.
///
struct OpcUaNodeDetails
{
    /// \brief NodeId string.
    QString nodeId;
    /// \brief DisplayName text.
    QString displayName;
    /// \brief OPC UA NodeClass numeric value.
    int nodeClass = 0;
    /// \brief Current Value attribute.
    QVariant value;
    /// \brief QOpcUa::Types numeric value.
    int valueType = 0;
    /// \brief DataType NodeId string.
    QString dataTypeId;
    /// \brief OPC UA ValueRank.
    int valueRank = -2;
    /// \brief ArrayDimensions attribute values.
    QList<quint32> arrayDimensions;
    /// \brief AccessLevel bit mask.
    quint8 accessLevel = 0;
    /// \brief UserAccessLevel bit mask.
    quint8 userAccessLevel = 0;
    /// \brief Whether the server stores historical values for this node.
    bool historizing = false;
    /// \brief Status display text.
    QString status;
    /// \brief Source timestamp, when provided.
    QDateTime sourceTimestamp;
    /// \brief Server timestamp, when provided.
    QDateTime serverTimestamp;
    /// \brief EventNotifier bit mask.
    quint8 eventNotifier = 0;
    /// \brief Full formatted attribute tree.
    QVector<OpcUaNodeAttribute> attributes;
};

namespace OpcUa {

///
/// \brief EventNotifier bits (OPC UA Part 3).
///
enum EventNotifier : quint8 {
    SubscribeToEvents = 0x01,
    HistoryRead       = 0x04,
};

///
/// \brief Returns true when a node can be offered for raw HistoryRead.
/// \param details Selected node details.
/// \return True for historizing variable nodes when HistoryRead is available.
///
inline bool canReadHistory(const OpcUaNodeDetails &details)
{
    return isHistoryReadSupported() && isVariable(details.nodeClass) && details.historizing;
}

///
/// \brief Returns true when a node advertises that it emits events.
/// \param details Selected node details.
/// \return True when the EventNotifier SubscribeToEvents bit is set.
///
inline bool canMonitorEvents(const OpcUaNodeDetails &details)
{
    return (details.eventNotifier & SubscribeToEvents) != 0;
}

///
/// \brief Returns true when a node can be offered for event HistoryRead.
/// \param details Selected node details.
/// \return True when the EventNotifier HistoryRead bit is set.
///
inline bool canReadEventHistory(const OpcUaNodeDetails &details)
{
    return isHistoryReadSupported() && (details.eventNotifier & HistoryRead) != 0;
}

///
/// \brief Returns true when a browsed node can be offered for raw HistoryRead.
/// \param node Browsed node carrying its NodeClass and Historizing flag.
/// \return True for historizing variable nodes when HistoryRead is available.
///
inline bool canReadHistory(const OpcUaNodeInfo &node)
{
    return isHistoryReadSupported() && isVariable(node.nodeClass) && node.historizing;
}

///
/// \brief Returns true when a browsed node advertises that it emits events.
/// \param node Browsed node carrying its EventNotifier bits.
/// \return True when the EventNotifier SubscribeToEvents bit is set.
///
inline bool canMonitorEvents(const OpcUaNodeInfo &node)
{
    return (node.eventNotifier & SubscribeToEvents) != 0;
}

///
/// \brief Returns true when a browsed node can be offered for event HistoryRead.
/// \param node Browsed node carrying its EventNotifier bits.
/// \return True when the EventNotifier HistoryRead bit is set and HistoryRead is available.
///
inline bool canReadEventHistory(const OpcUaNodeInfo &node)
{
    return isHistoryReadSupported() && (node.eventNotifier & HistoryRead) != 0;
}

} // namespace OpcUa

///
/// \brief Current value and metadata for a Data Access row.
///
struct OpcUaDataValue
{
    /// \brief NodeId string.
    QString nodeId;
    /// \brief DisplayName text.
    QString displayName;
    /// \brief Current Value attribute.
    QVariant value;
    /// \brief QOpcUa::Types numeric value.
    int valueType = 0;
    /// \brief DataType NodeId string.
    QString dataTypeId;
    /// \brief Status display text.
    QString status;
    /// \brief Source timestamp, when provided.
    QDateTime sourceTimestamp;
    /// \brief Server timestamp, when provided.
    QDateTime serverTimestamp;
    /// \brief UserAccessLevel bit mask.
    quint8 userAccessLevel = 0;
};

///
/// \brief One OPC UA event notification delivered for a monitored node.
///
struct OpcUaEvent
{
    /// \brief Monitored node that produced the event.
    QString sourceNodeId;
    /// \brief Event Time field.
    QDateTime time;
    /// \brief Event Severity field (1-1000).
    quint16 severity = 0;
    /// \brief Event SourceName field.
    QString sourceName;
    /// \brief Event Message field text.
    QString message;
    /// \brief Event EventType NodeId.
    QString eventType;
    /// \brief All select-clause field values in display form.
    QStringList fields;
};

///
/// \brief One historical sample returned by a HistoryRead.
///
struct OpcUaHistoryValue
{
    /// \brief NodeId string.
    QString nodeId;
    /// \brief Sampled Value.
    QVariant value;
    /// \brief QOpcUa::Types numeric value.
    int valueType = 0;
    /// \brief Status display text.
    QString status;
    /// \brief Source timestamp, when provided.
    QDateTime sourceTimestamp;
    /// \brief Server timestamp, when provided.
    QDateTime serverTimestamp;
};

///
/// \brief Node counts keyed by OPC UA namespace index.
///
using OpcUaNamespaceNodeCounts = QHash<int, int>;

Q_DECLARE_METATYPE(EndpointInfo)
Q_DECLARE_METATYPE(OpcUaNamespaceNodeCounts)
Q_DECLARE_METATYPE(OpcUaNodeInfo)
Q_DECLARE_METATYPE(OpcUaMethodArgument)
Q_DECLARE_METATYPE(QVector<OpcUaMethodArgument>)
Q_DECLARE_METATYPE(OpcUaNodeDetails)
Q_DECLARE_METATYPE(OpcUaDataValue)
Q_DECLARE_METATYPE(OpcUaEvent)
Q_DECLARE_METATYPE(OpcUaHistoryValue)
