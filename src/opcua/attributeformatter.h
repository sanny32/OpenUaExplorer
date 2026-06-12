// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file attributeformatter.h
/// \brief Pure helpers that format OPC UA values and attributes for display.
///

#pragma once

#include <QtOpcUa/qopcuatype.h>

#include <QOpcUaEndpointDescription>

#include "opcuatypes.h"

///
/// \brief Side-effect-free formatting of OPC UA values and node attributes.
///
/// These helpers were extracted from OpcUaClientService so they can be unit
/// tested without a live connection or a backend plugin. Every function is a
/// pure transformation of its inputs.
///
namespace OpcUaFormat {

///
/// \brief Checks whether a QVariant contains a sequential value array.
/// \param value Value to inspect.
/// \return True for list-like values, excluding scalar strings and byte strings.
///
bool isValueArray(const QVariant &value);

///
/// \brief Formats a QVariant for display without discarding its typed value.
/// \param value Value to format.
/// \return Human-readable value.
///
QString displayValue(const QVariant &value);

///
/// \brief Returns the localized security mode name.
/// \param mode OPC UA message security mode.
/// \return Display name.
///
QString securityModeName(QOpcUaEndpointDescription::MessageSecurityMode mode);

///
/// \brief Returns a display string for a status code.
/// \param status OPC UA status code.
/// \return Status name.
///
QString statusName(QOpcUa::UaStatusCode status);

///
/// \brief Formats a status code with its numeric representation.
/// \param status OPC UA status code.
/// \return Status name and hexadecimal value.
///
QString statusDisplay(QOpcUa::UaStatusCode status);

///
/// \brief Formats a timestamp like UaExpert.
/// \param timestamp Timestamp to format.
/// \return Localized timestamp text.
///
QString timestampDisplay(const QDateTime &timestamp);

///
/// \brief Returns the symbolic name of an OPC UA value type.
/// \param type OPC UA value type.
/// \return Symbolic type name.
///
QString valueTypeName(QOpcUa::Types type);

///
/// \brief Returns the symbolic name of an OPC UA node class.
/// \param nodeClass OPC UA node class.
/// \return Symbolic node class name.
///
QString nodeClassName(QOpcUa::NodeClass nodeClass);

///
/// \brief Formats an OPC UA access level mask.
/// \param accessLevel Access level mask.
/// \return Set flag names separated by vertical bars.
///
QString accessLevelDisplay(quint32 accessLevel);

///
/// \brief Formats an OPC UA write mask.
/// \param writeMask Write mask.
/// \return Set flag names or zero.
///
QString writeMaskDisplay(quint32 writeMask);

///
/// \brief Formats an OPC UA value rank.
/// \param valueRank Value rank.
/// \return Numeric rank with its symbolic meaning.
///
QString valueRankDisplay(int valueRank);

///
/// \brief Returns the textual name of a NodeId identifier type.
/// \param identifierType OPC UA NodeId identifier type marker.
/// \return Identifier type name.
///
QString identifierTypeName(char identifierType);

///
/// \brief Creates a child row.
/// \param name Row name.
/// \param displayValue Display value.
/// \return Child attribute row.
///
OpcUaNodeAttribute childAttribute(const QString &name, const QString &displayValue);

///
/// \brief Adds parsed NodeId fields to an attribute row.
/// \param attribute Attribute row to populate.
/// \param nodeId OPC UA NodeId string.
///
void addNodeIdChildren(OpcUaNodeAttribute *attribute, const QString &nodeId);

///
/// \brief Formats a NodeId and adds its parsed fields.
/// \param attribute Attribute row to populate.
/// \param nodeId OPC UA NodeId string.
///
void formatNodeIdAttribute(OpcUaNodeAttribute *attribute, const QString &nodeId);

///
/// \brief Formats a DataType NodeId using its built-in type name when possible.
/// \param attribute Attribute row to populate.
/// \param nodeId DataType NodeId.
///
void formatDataTypeAttribute(OpcUaNodeAttribute *attribute, const QString &nodeId);

///
/// \brief Formats a QVariant as a tree value.
/// \param value Value to format.
/// \param type OPC UA value type.
/// \return Root row containing scalar or array entries.
///
OpcUaNodeAttribute valueAttribute(const QVariant &value, QOpcUa::Types type);

///
/// \brief Formats an attribute value and creates expandable child rows.
/// \param attribute Attribute row to populate.
/// \param nodeAttribute OPC UA attribute identifier.
/// \param value Attribute value.
/// \param valueType Variable value type.
///
void formatAttribute(OpcUaNodeAttribute *attribute,
                     QOpcUa::NodeAttribute nodeAttribute,
                     const QVariant &value,
                     QOpcUa::Types valueType);

///
/// \brief Checks whether an attribute is defined for a node class.
/// \param attribute OPC UA attribute.
/// \param nodeClass OPC UA node class.
/// \return True if the attribute applies to the node class.
///
bool attributeAppliesToNodeClass(QOpcUa::NodeAttribute attribute,
                                 QOpcUa::NodeClass nodeClass);

///
/// \brief Maps a namespace zero DataType NodeId to QOpcUa::Types.
/// \param nodeId OPC UA DataType NodeId.
/// \return Matching Qt OPC UA type.
///
QOpcUa::Types valueTypeForDataType(const QString &nodeId);

} // namespace OpcUaFormat
