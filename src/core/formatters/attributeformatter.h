// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file attributeformatter.h
/// \brief Pure helpers that format OPC UA values and attributes for display.
///

#pragma once

#include <optional>

#include <QSize>

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
/// \brief Renders a value of an enumeration DataType as its number and field name.
/// \param value Variant to format; arrays are rendered element by element.
/// \param entries Named values of the DataType.
/// \return Text such as "0 (Disabled)", or the plain display value when the number is not named.
///
/// The number stays in front of the name because it is what the server carries and what a
/// write has to produce; the name only explains it. Values outside the definition are shown
/// as they are: a server is free to send one, and hiding it would lose the fact.
///
QString enumDisplayValue(const QVariant &value, const OpcUaEnumEntries &entries);

///
/// \brief Returns the field name an enumeration DataType gives a number.
/// \param value Numeric enumeration value.
/// \param entries Named values of the DataType.
/// \return Field name, or an empty string when the definition does not name the value.
///
QString enumEntryName(qint64 value, const OpcUaEnumEntries &entries);

///
/// \brief Renders the OPC UA types Qt carries in classes of their own.
/// \param value Variant to format.
/// \return Display text, or nullopt when the value is not one of those types.
///
/// QVariant::toString() returns an empty string for QOpcUaQualifiedName and its siblings
/// because Qt OPC UA registers no string converters for them, so every one of them needs
/// a rule of its own.
///
std::optional<QString> builtinTypeText(const QVariant &value);

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
/// \return Standard BrowseName, built-in type name, or the original NodeId for custom types.
///
QString dataTypeDisplay(const QString &nodeId);

///
/// \brief Names the type of a value, falling back to its declared DataType.
/// \param type Value type resolved from the DataType, may be Undefined.
/// \param dataTypeId DataType NodeId string backing the value.
/// \return The DataType's own name, or the value-type name when no DataType is known.
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
/// \brief Builds the Value attribute, expanding arrays and structures into child rows.
/// \param value Node value.
/// \param type Declared value type, used to label arrays.
/// \param dataTypeId DataType NodeId string, used to name types that are not built-in.
/// \param enumEntries Named values of the DataType; empty unless it is an enumeration.
/// \return The constructed Value attribute.
///
OpcUaNodeAttribute valueAttribute(const QVariant &value, QOpcUa::Types type,
                                  const QString &dataTypeId = QString(),
                                  const OpcUaEnumEntries &enumEntries = {});

///
/// \brief One expandable part of a composite value: an array element or a structure field.
///
struct ValueElement
{
    /// \brief Row label, "[0]" for an array element or the field name of a structure.
    QString label;
    /// \brief Formatted element value.
    QString text;
    /// \brief Element type name; empty when only the containing value knows it.
    QString typeName;
    /// \brief Raw element value, used to expand the element further.
    QVariant value;
    /// \brief True when the element itself expands into elements.
    bool hasChildren = false;
};

///
/// \brief Reports whether a value expands into elements of its own.
/// \param value Variant to inspect.
/// \return True for arrays; strings and byte arrays stay scalar.
///
bool hasValueElements(const QVariant &value);

///
/// \brief Splits a composite value into its elements.
/// \param value Variant to split.
/// \param limit Largest number of elements to return; negative returns all of them.
/// \param totalCount Receives the untruncated element count when not null.
/// \return Elements in their natural order, at most \a limit of them.
///
QVector<ValueElement> valueElements(const QVariant &value, int limit = -1,
                                    int *totalCount = nullptr);

/// \brief Array elements a value summary spells out before it names the array instead.
inline constexpr int defaultInlineElementLimit = 10;

///
/// \brief Renders a composite value as a short summary naming its type and size.
/// \param value Variant to describe.
/// \param type Declared value type, used to name array elements.
/// \param dataTypeId DataType NodeId string, used to name types that are not built-in.
/// \param enumEntries Named values of the DataType; empty unless it is an enumeration.
/// \param inlineElementLimit Longest array still spelled out element by element.
/// \return Summary such as "Int16[3]", or the plain display value for scalars.
///
QString valueSummary(const QVariant &value, QOpcUa::Types type,
                     const QString &dataTypeId = QString(),
                     const OpcUaEnumEntries &enumEntries = {},
                     int inlineElementLimit = defaultInlineElementLimit);

///
/// \brief Picture encoding an OPC UA image DataType stands for.
///
enum class ImageEncoding {
    None,
    Any,
    Bmp,
    Gif,
    Jpeg,
    Png
};

///
/// \brief Classifies a DataType NodeId as one of the standard image types.
/// \param dataTypeId DataType NodeId string.
/// \return Encoding the DataType prescribes; Any for the abstract Image type, None otherwise.
///
ImageEncoding imageEncodingForDataType(const QString &dataTypeId);

///
/// \brief A ByteString value whose DataType declares it to be a picture.
///
struct ImageValueInfo
{
    /// \brief Encoded picture exactly as the server sent it.
    QByteArray data;
    /// \brief Format name, "PNG", "JPEG", "GIF", or "BMP".
    QString formatName;
    /// \brief Pixel dimensions; empty when the header cannot be read.
    QSize size;
};

///
/// \brief Recognises a value as a picture from its declared DataType.
/// \param value Variant to inspect.
/// \param dataTypeId DataType NodeId string backing the value.
/// \return Picture data with its format and dimensions, or nullopt when the value is none.
///
/// Only the DataType decides: an arbitrary ByteString is never sniffed for image magic,
/// so a value that merely happens to start with a known header keeps its hex rendering.
///
std::optional<ImageValueInfo> imageValue(const QVariant &value, const QString &dataTypeId);

///
/// \brief Renders a picture as a one-line summary with a short hex prefix.
/// \param info Picture to describe.
/// \return Summary such as "PNG 640x480, 12.1 KB - 89 50 4e 47 0d 0a 1a 0a...".
///
QString imageSummary(const ImageValueInfo &info);

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
