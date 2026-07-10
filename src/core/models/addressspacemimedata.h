// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file addressspacemimedata.h
/// \brief Declares drag MIME helpers for OPC UA address-space nodes.
///

#pragma once

#include <QString>

#include "opcua/opcuatypes.h"

class QMimeData;

///
/// \brief MIME helpers for address-space node drag data.
///
namespace AddressSpaceMime {

///
/// \brief Returns the MIME type used for an OPC UA address-space node.
/// \return MIME type string.
///
QString nodeMimeType();

///
/// \brief Builds MIME data for one OPC UA address-space node.
/// \param node Node to encode.
/// \return MIME data owned by the caller.
///
QMimeData *createNodeMimeData(const OpcUaNodeInfo &node);

///
/// \brief Decodes an OPC UA address-space node from MIME data.
/// \param mimeData MIME data to read.
/// \param node Destination for the decoded node.
/// \return True when the MIME data contains a node.
///
bool decodeNode(const QMimeData *mimeData, OpcUaNodeInfo *node);

} // namespace AddressSpaceMime
