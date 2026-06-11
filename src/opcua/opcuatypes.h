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
