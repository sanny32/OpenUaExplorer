// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file opcuatypes.h
/// \brief Defines transport-neutral data objects used by the OPC UA UI.
///

#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>
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
/// \brief Well-known NodeId of the standard Objects folder (browse root).
///
inline constexpr char kObjectsFolderId[] = "ns=0;i=84";

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

} // namespace OpcUa

///
/// \brief Connection lifecycle state exposed to the UI.
///
enum class OpcUaConnectionState {
    Unavailable,
    Disconnected,
    Discovering,
    Connecting,
    Connected,
    Closing
};

///
/// \brief User token capabilities advertised by an endpoint.
///
struct EndpointInfo
{
    int index = -1;
    QString endpointUrl;
    QString securityPolicy;
    QString securityMode;
    int securityModeValue = 1;
    bool supportsAnonymous = false;
    bool supportsUsername = false;
    bool supportsCertificate = false;
    QByteArray serverCertificate;
};

///
/// \brief One browsed OPC UA node.
///
struct OpcUaNodeInfo
{
    QString nodeId;
    QString browseName;
    QString displayName;
    QString referenceTypeId;
    int nodeClass = 0;
    bool hasChildren = true;
};

///
/// \brief One displayable OPC UA node attribute.
///
struct OpcUaNodeAttribute
{
    QString name;
    QVariant value;
    QString displayValue;
    QString status;
    QDateTime sourceTimestamp;
    QDateTime serverTimestamp;
    QVector<OpcUaNodeAttribute> children;
};

///
/// \brief Complete selected-node information returned by a read.
///
struct OpcUaNodeDetails
{
    QString nodeId;
    QString displayName;
    int nodeClass = 0;
    QVariant value;
    int valueType = 0;
    QString dataTypeId;
    int valueRank = -2;
    QList<quint32> arrayDimensions;
    quint8 accessLevel = 0;
    quint8 userAccessLevel = 0;
    QString status;
    QDateTime sourceTimestamp;
    QDateTime serverTimestamp;
    QVector<OpcUaNodeAttribute> attributes;
};

///
/// \brief Current value and metadata for a Data Access row.
///
struct OpcUaDataValue
{
    QString nodeId;
    QString displayName;
    QVariant value;
    int valueType = 0;
    QString dataTypeId;
    QString status;
    QDateTime sourceTimestamp;
    QDateTime serverTimestamp;
    quint8 userAccessLevel = 0;
};

Q_DECLARE_METATYPE(EndpointInfo)
Q_DECLARE_METATYPE(OpcUaNodeInfo)
Q_DECLARE_METATYPE(OpcUaNodeDetails)
Q_DECLARE_METATYPE(OpcUaDataValue)
