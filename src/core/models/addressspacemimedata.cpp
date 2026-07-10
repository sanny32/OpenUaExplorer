// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file addressspacemimedata.cpp
/// \brief Implements drag MIME helpers for OPC UA address-space nodes.
///

#include "addressspacemimedata.h"

#include <QDataStream>
#include <QIODevice>
#include <QMimeData>

///
/// \brief Returns the MIME type used for an OPC UA address-space node.
/// \return MIME type string.
///
QString AddressSpaceMime::nodeMimeType()
{
    return QStringLiteral("application/x-ouaexp-address-space-node");
}

///
/// \brief Builds MIME data for one OPC UA address-space node.
/// \param node Node to encode.
/// \return MIME data owned by the caller.
///
QMimeData *AddressSpaceMime::createNodeMimeData(const OpcUaNodeInfo &node)
{
    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream << node.nodeId
           << node.browseName
           << node.displayName
           << node.displayPath
           << node.referenceTypeId
           << node.nodeClass
           << node.eventNotifier
           << node.historizing
           << node.hasChildren;

    auto *mimeData = new QMimeData;
    mimeData->setData(nodeMimeType(), payload);
    return mimeData;
}

///
/// \brief Decodes an OPC UA address-space node from MIME data.
/// \param mimeData MIME data to read.
/// \param node Destination for the decoded node.
/// \return True when the MIME data contains a node.
///
bool AddressSpaceMime::decodeNode(const QMimeData *mimeData, OpcUaNodeInfo *node)
{
    if (!mimeData || !node || !mimeData->hasFormat(nodeMimeType()))
        return false;

    const QByteArray payload = mimeData->data(nodeMimeType());
    QDataStream stream(payload);

    OpcUaNodeInfo decoded;
    stream >> decoded.nodeId
           >> decoded.browseName
           >> decoded.displayName
           >> decoded.displayPath
           >> decoded.referenceTypeId
           >> decoded.nodeClass
           >> decoded.eventNotifier
           >> decoded.historizing
           >> decoded.hasChildren;
    if (stream.status() != QDataStream::Ok)
        return false;

    *node = decoded;
    return true;
}
