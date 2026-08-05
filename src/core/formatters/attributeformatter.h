// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file attributeformatter.h
/// \brief Pure helpers that format OPC UA values and attributes for display.
///

#pragma once

#include <QtOpcUa/qopcuatype.h>

#include <QOpcUaEndpointDescription>

#include "opcua/opcuatypes.h"

///
/// \brief Side-effect-free formatting of OPC UA values and node attributes.
///
/// These helpers were extracted from OpcUaBackend so they can be unit
/// tested without a live connection or a backend plugin. Every function is a
/// pure transformation of its inputs.
///
namespace OpcUaFormat {

///
/// \brief Display mode for OPC UA timestamps.
///
enum class TimestampMode {
    LocalTime,
    Utc
};

///
/// \brief Quality class of an OPC UA status code, derived from its name.
///
enum class StatusSeverity {
    Unknown,
    Good,
    Uncertain,
    Bad
};

///
/// \brief Reports whether a value is an array, treating strings and byte arrays as scalars.
/// \param value Variant to inspect.
/// \return True when the value should be rendered as a list.
///
bool isValueArray(const QVariant &value);

///
/// \brief Renders a value for display, recursing into arrays and hex-encoding byte strings.
/// \param value Variant to format.
/// \return Human-readable representation.
///
QString displayValue(const QVariant &value);

///
/// \brief Returns the translated name of a message security mode.
/// \param mode Security mode to name.
/// \return Localised mode name.
///
QString securityModeName(QOpcUaEndpointDescription::MessageSecurityMode mode);

///
/// \brief Returns the textual name of an OPC UA status code.
/// \param status Status code to name.
/// \return Status code name.
///
QString statusName(QOpcUa::UaStatusCode status);

///
/// \brief Formats a status code as name plus zero-padded hexadecimal value.
/// \param status Status code to format.
/// \return Combined name and hex representation.
///
QString statusDisplay(QOpcUa::UaStatusCode status);

///
/// \brief Classifies a status-code name into its OPC UA quality class.
/// \param statusName Status-code name as produced by statusName().
/// \return Matching severity; Unknown for empty or unrecognised names.
///
/// OPC UA guarantees that every status-code name starts with "Good", "Uncertain",
/// or "Bad", so the name alone carries the quality class.
///
StatusSeverity statusSeverity(const QString &statusName);

///
/// \brief Formats a timestamp as ISO 8601 with a zone indicator, or empty when invalid.
/// \param timestamp Timestamp to format.
/// \param mode Local time (trailing UTC offset) or UTC (trailing "Z").
/// \return ISO 8601 string with millisecond precision and a trailing zone indicator.
///
QString isoTimestampWithZone(const QDateTime &timestamp,
                             TimestampMode mode = TimestampMode::LocalTime);

///
/// \brief Returns the enum-key name of an OPC UA value type.
/// \param type Value type to name.
/// \return Type name, or "Unknown" when unrecognised.
///
QString valueTypeName(QOpcUa::Types type);

///
/// \brief Resolves a DataType NodeId to its type name.
/// \param nodeId DataType NodeId string.
/// \return Built-in type name, standard BrowseName, or the original NodeId for custom types.
///
QString dataTypeDisplay(const QString &nodeId);

///
/// \brief Names the type of a value, falling back to its declared DataType.
/// \param type Value type resolved from the DataType, may be Undefined.
/// \param dataTypeId DataType NodeId string backing the value.
/// \return Value-type name, or the DataType's own name when it is not a built-in type.
///
QString valueTypeDisplay(QOpcUa::Types type, const QString &dataTypeId);

///
/// \brief Resolves a known namespace-0 NodeId to its BrowseName.
/// \param nodeId NodeId string.
/// \return Standard BrowseName, or the original NodeId for custom/unknown nodes.
///
QString standardNodeDisplayName(const QString &nodeId);

///
/// \brief Returns the enum-key name of a node class.
/// \param nodeClass Node class to name.
/// \return Class name, or its numeric value when unrecognised.
///
QString nodeClassName(QOpcUa::NodeClass nodeClass);

///
/// \brief Decodes an access-level bitmask into a pipe-separated list of flag names.
/// \param accessLevel Access-level bits.
/// \return Flag names, or "None" when no bits are set.
///
QString accessLevelDisplay(quint32 accessLevel);

///
/// \brief Decodes a write-mask bitmask into a pipe-separated list of flag names.
/// \param writeMask Write-mask bits.
/// \return Flag names, or the numeric value when no known bits match.
///
QString writeMaskDisplay(quint32 writeMask);

///
/// \brief Decodes an event-notifier bitmask into a pipe-separated list of flag names.
/// \param eventNotifier Event-notifier bits.
/// \return Flag names, or "None" when no bits are set.
///
QString eventNotifierDisplay(quint8 eventNotifier);

///
/// \brief Formats a value rank with its symbolic name for the well-known ranks.
/// \param valueRank Value rank to format.
/// \return Rank with description, or the bare number for custom ranks.
///
QString valueRankDisplay(int valueRank);

///
/// \brief Names a NodeId identifier type from its single-character code.
/// \param identifierType Code: 'i', 's', 'g', or 'b'.
/// \return Identifier-type name, or "Unknown".
///
QString identifierTypeName(char identifierType);

///
/// \brief Builds a leaf attribute row with a name and display value.
/// \param name Attribute name.
/// \param displayValue Pre-formatted display value.
/// \return The constructed attribute.
///
OpcUaNodeAttribute childAttribute(const QString &name, const QString &displayValue);

///
/// \brief Appends namespace index, identifier type, and identifier child rows for a NodeId.
/// \param attribute Parent attribute to extend; unchanged when the NodeId cannot be split.
/// \param nodeId NodeId string to decompose.
///
void addNodeIdChildren(OpcUaNodeAttribute *attribute, const QString &nodeId);

///
/// \brief Sets a NodeId attribute's display value and expands its components as children.
/// \param attribute Attribute to populate.
/// \param nodeId NodeId string.
///
void formatNodeIdAttribute(OpcUaNodeAttribute *attribute, const QString &nodeId);

///
/// \brief Sets a DataType attribute to the resolved type name (or NodeId) plus component children.
/// \param attribute Attribute to populate.
/// \param nodeId DataType NodeId string.
///
void formatDataTypeAttribute(OpcUaNodeAttribute *attribute, const QString &nodeId);

///
/// \brief Builds the Value attribute, expanding arrays into indexed child rows.
/// \param value Node value.
/// \param type Declared value type, used to label arrays.
/// \param dataTypeId DataType NodeId string, used to name types that are not built-in.
/// \return The constructed Value attribute.
///
OpcUaNodeAttribute valueAttribute(const QVariant &value, QOpcUa::Types type,
                                  const QString &dataTypeId = QString());

///
/// \brief Fills an attribute's display value (and children) using the rules for its attribute id.
/// \param attribute Attribute to populate.
/// \param nodeAttribute Which OPC UA attribute is being formatted.
/// \param value Raw attribute value.
/// \param valueType Value type, used when formatting the Value attribute.
/// \param dataTypeId DataType NodeId string, used to name types that are not built-in.
///
void formatAttribute(OpcUaNodeAttribute *attribute,
                     QOpcUa::NodeAttribute nodeAttribute,
                     const QVariant &value,
                     QOpcUa::Types valueType,
                     const QString &dataTypeId = QString());

///
/// \brief Reports whether an attribute is meaningful for a given node class.
/// \param attribute Attribute to test.
/// \param nodeClass Node class to test against.
/// \return True when the attribute applies; class-agnostic attributes always return true.
///
bool attributeAppliesToNodeClass(QOpcUa::NodeAttribute attribute,
                                 QOpcUa::NodeClass nodeClass);

///
/// \brief Maps a namespace-0 built-in DataType NodeId to its OPC UA value type.
/// \param nodeId DataType NodeId, expected as "ns=0;i=...".
/// \return Matching value type, or Undefined for non-builtin or unparsable ids.
///
QOpcUa::Types valueTypeForDataType(const QString &nodeId);

///
/// \brief Reports whether a DataType NodeId denotes values that can be read as numbers.
/// \param nodeId DataType NodeId string.
/// \return True for numeric namespace-0 DataTypes and for types that cannot be classified.
///
/// Namespace-0 DataTypes are classified from the specification: the integer and
/// floating-point built-ins and their abstract parents (Number, Integer, UInteger)
/// and numeric subtypes (IntegerId, Counter, Duration) are numeric, every other
/// namespace-0 type (Boolean, String, DateTime, structures, ...) is not. Types from
/// other namespaces and the abstract BaseDataType are accepted, because resolving
/// their supertype needs extra server reads and rejecting them would hide the custom
/// numeric and enumeration types servers commonly define.
///
bool isNumericDataType(const QString &nodeId);

///
/// \brief Converts text to a typed scalar, range-checking integral types.
/// \param text Source text.
/// \param type Target OPC UA value type.
/// \param ok Receives the conversion status; must not be null.
/// \return Converted scalar, or an invalid variant on failure.
///
QVariant scalarFromText(const QString &text, QOpcUa::Types type, bool *ok);

} // namespace OpcUaFormat
