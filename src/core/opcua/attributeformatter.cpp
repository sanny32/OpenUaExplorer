// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file attributeformatter.cpp
/// \brief Implements the OPC UA value and attribute formatting helpers.
///

#include "attributeformatter.h"

#include <limits>

#include <QDateTime>
#include <QMetaEnum>
#include <QObject>
#include <QUuid>

#include <QOpcUaLocalizedText>
#include <QOpcUaQualifiedName>

namespace OpcUaFormat {

///
/// \brief Reports whether a value is an array, treating strings and byte arrays as scalars.
/// \param value Variant to inspect.
/// \return True when the value should be rendered as a list.
///
bool isValueArray(const QVariant &value)
{
    return value.userType() != QMetaType::QString
        && value.userType() != QMetaType::QByteArray
        && value.canConvert<QVariantList>();
}

///
/// \brief Renders a value for display, recursing into arrays and hex-encoding byte strings.
/// \param value Variant to format.
/// \return Human-readable representation.
///
QString displayValue(const QVariant &value)
{
    if (!value.isValid())
        return QString();
    if (value.userType() == QMetaType::QByteArray)
        return QString::fromLatin1(value.toByteArray().toHex(' '));
    if (value.userType() == QMetaType::QDateTime)
        return value.toDateTime().toString(Qt::ISODateWithMs);
    if (isValueArray(value)) {
        const QVariantList list = value.toList();
        QStringList parts;
        parts.reserve(list.size());
        for (const QVariant &entry : list)
            parts.append(displayValue(entry));
        return QStringLiteral("[%1]").arg(parts.join(QStringLiteral(", ")));
    }
    return value.toString();
}

///
/// \brief Returns the translated name of a message security mode.
/// \param mode Security mode to name.
/// \return Localised mode name.
///
QString securityModeName(QOpcUaEndpointDescription::MessageSecurityMode mode)
{
    switch (mode) {
    case QOpcUaEndpointDescription::None: return QObject::tr("None");
    case QOpcUaEndpointDescription::Sign: return QObject::tr("Sign");
    case QOpcUaEndpointDescription::SignAndEncrypt: return QObject::tr("Sign & Encrypt");
    default: return QObject::tr("Invalid");
    }
}

///
/// \brief Returns the textual name of an OPC UA status code.
/// \param status Status code to name.
/// \return Status code name.
///
QString statusName(QOpcUa::UaStatusCode status)
{
    return QOpcUa::statusToString(status);
}

///
/// \brief Formats a status code as name plus zero-padded hexadecimal value.
/// \param status Status code to format.
/// \return Combined name and hex representation.
///
QString statusDisplay(QOpcUa::UaStatusCode status)
{
    return QStringLiteral("%1 (0x%2)")
        .arg(statusName(status))
        .arg(static_cast<quint32>(status), 8, 16, QLatin1Char('0'));
}

///
/// \brief Formats a timestamp in local time, or empty when invalid.
/// \param timestamp Timestamp to format.
/// \return Localised timestamp string.
///
QString timestampDisplay(const QDateTime &timestamp)
{
    return timestamp.isValid()
        ? timestamp.toLocalTime().toString(QStringLiteral("dd.MM.yyyy H:mm:ss.zzz"))
        : QString();
}

///
/// \brief Returns the enum-key name of an OPC UA value type.
/// \param type Value type to name.
/// \return Type name, or "Unknown" when unrecognised.
///
QString valueTypeName(QOpcUa::Types type)
{
    const char *key = QMetaEnum::fromType<QOpcUa::Types>().valueToKey(type);
    return key ? QString::fromLatin1(key) : QObject::tr("Unknown");
}

///
/// \brief Returns the enum-key name of a node class.
/// \param nodeClass Node class to name.
/// \return Class name, or its numeric value when unrecognised.
///
QString nodeClassName(QOpcUa::NodeClass nodeClass)
{
    const char *key = QMetaEnum::fromType<QOpcUa::NodeClass>()
                          .valueToKey(static_cast<int>(nodeClass));
    return key ? QString::fromLatin1(key) : QString::number(static_cast<int>(nodeClass));
}

///
/// \brief Decodes an access-level bitmask into a pipe-separated list of flag names.
/// \param accessLevel Access-level bits.
/// \return Flag names, or "None" when no bits are set.
///
QString accessLevelDisplay(quint32 accessLevel)
{
    const QMetaEnum metaEnum = QMetaEnum::fromType<QOpcUa::AccessLevelBit>();
    QStringList names;
    for (int index = 0; index < metaEnum.keyCount(); ++index) {
        const quint32 flag = static_cast<quint32>(metaEnum.value(index));
        if (flag && (accessLevel & flag) == flag)
            names.append(QString::fromLatin1(metaEnum.key(index)));
    }
    return names.isEmpty() ? QStringLiteral("None") : names.join(QStringLiteral(" | "));
}

///
/// \brief Decodes a write-mask bitmask into a pipe-separated list of flag names.
/// \param writeMask Write-mask bits.
/// \return Flag names, or the numeric value when no known bits match.
///
QString writeMaskDisplay(quint32 writeMask)
{
    if (!writeMask)
        return QStringLiteral("0");
    const QMetaEnum metaEnum = QMetaEnum::fromType<QOpcUa::WriteMaskBit>();
    QStringList names;
    for (int index = 0; index < metaEnum.keyCount(); ++index) {
        const quint32 flag = static_cast<quint32>(metaEnum.value(index));
        if (flag && (writeMask & flag) == flag)
            names.append(QString::fromLatin1(metaEnum.key(index)));
    }
    return names.isEmpty() ? QString::number(writeMask)
                           : names.join(QStringLiteral(" | "));
}

///
/// \brief Decodes an event-notifier bitmask into a pipe-separated list of flag names.
/// \param eventNotifier Event-notifier bits.
/// \return Flag names, or "None" when no bits are set.
///
QString eventNotifierDisplay(quint8 eventNotifier)
{
    const QMetaEnum metaEnum = QMetaEnum::fromType<QOpcUa::EventNotifierBit>();
    QStringList names;
    for (int index = 0; index < metaEnum.keyCount(); ++index) {
        const quint8 flag = static_cast<quint8>(metaEnum.value(index));
        if (flag && (eventNotifier & flag) == flag)
            names.append(QString::fromLatin1(metaEnum.key(index)));
    }
    return names.isEmpty() ? QStringLiteral("None") : names.join(QStringLiteral(" | "));
}

///
/// \brief Formats a value rank with its symbolic name for the well-known ranks.
/// \param valueRank Value rank to format.
/// \return Rank with description, or the bare number for custom ranks.
///
QString valueRankDisplay(int valueRank)
{
    switch (valueRank) {
    case -3: return QStringLiteral("-3 (ScalarOrOneDimension)");
    case -2: return QStringLiteral("-2 (Any)");
    case -1: return QStringLiteral("-1 (Scalar)");
    case 0: return QStringLiteral("0 (OneOrMoreDimensions)");
    case 1: return QStringLiteral("1 (OneDimension)");
    case 2: return QStringLiteral("2 (TwoDimensions)");
    default: return QString::number(valueRank);
    }
}

///
/// \brief Names a NodeId identifier type from its single-character code.
/// \param identifierType Code: 'i', 's', 'g', or 'b'.
/// \return Identifier-type name, or "Unknown".
///
QString identifierTypeName(char identifierType)
{
    switch (identifierType) {
    case 'i': return QStringLiteral("Numeric");
    case 's': return QStringLiteral("String");
    case 'g': return QStringLiteral("Guid");
    case 'b': return QStringLiteral("ByteString");
    default: return QObject::tr("Unknown");
    }
}

///
/// \brief Builds a leaf attribute row with a name and display value.
/// \param name Attribute name.
/// \param displayValue Pre-formatted display value.
/// \return The constructed attribute.
///
OpcUaNodeAttribute childAttribute(const QString &name, const QString &displayValue)
{
    OpcUaNodeAttribute child;
    child.name = name;
    child.displayValue = displayValue;
    return child;
}

///
/// \brief Appends namespace index, identifier type, and identifier child rows for a NodeId.
/// \param attribute Parent attribute to extend; unchanged when the NodeId cannot be split.
/// \param nodeId NodeId string to decompose.
///
void addNodeIdChildren(OpcUaNodeAttribute *attribute, const QString &nodeId)
{
    quint16 namespaceIndex = 0;
    QString identifier;
    char identifierType = '\0';
    if (!QOpcUa::nodeIdStringSplit(nodeId, &namespaceIndex, &identifier, &identifierType))
        return;

    attribute->children.append(
        childAttribute(QObject::tr("NamespaceIndex"), QString::number(namespaceIndex)));
    attribute->children.append(
        childAttribute(QObject::tr("IdentifierType"), identifierTypeName(identifierType)));
    attribute->children.append(
        childAttribute(QObject::tr("Identifier"), identifier));
}

///
/// \brief Sets a NodeId attribute's display value and expands its components as children.
/// \param attribute Attribute to populate.
/// \param nodeId NodeId string.
///
void formatNodeIdAttribute(OpcUaNodeAttribute *attribute, const QString &nodeId)
{
    attribute->displayValue = nodeId;
    addNodeIdChildren(attribute, nodeId);
}

///
/// \brief Sets a DataType attribute to the resolved type name (or NodeId) plus component children.
/// \param attribute Attribute to populate.
/// \param nodeId DataType NodeId string.
///
void formatDataTypeAttribute(OpcUaNodeAttribute *attribute, const QString &nodeId)
{
    const QOpcUa::Types type = valueTypeForDataType(nodeId);
    attribute->displayValue = type == QOpcUa::Types::Undefined
        ? nodeId
        : valueTypeName(type);
    addNodeIdChildren(attribute, nodeId);
}

///
/// \brief Builds the Value attribute, expanding arrays into indexed child rows.
/// \param value Node value.
/// \param type Declared value type, used to label arrays.
/// \return The constructed Value attribute.
///
OpcUaNodeAttribute valueAttribute(const QVariant &value, QOpcUa::Types type)
{
    OpcUaNodeAttribute result = childAttribute(QObject::tr("Value"), displayValue(value));
    if (!isValueArray(value))
        return result;

    const QVariantList values = value.toList();
    result.displayValue = QStringLiteral("%1 Array[%2]")
                              .arg(valueTypeName(type))
                              .arg(values.size());
    for (int index = 0; index < values.size(); ++index) {
        result.children.append(
            childAttribute(QStringLiteral("[%1]").arg(index), displayValue(values.at(index))));
    }
    return result;
}

///
/// \brief Fills an attribute's display value (and children) using the rules for its attribute id.
/// \param attribute Attribute to populate.
/// \param nodeAttribute Which OPC UA attribute is being formatted.
/// \param value Raw attribute value.
/// \param valueType Value type, used when formatting the Value attribute.
///
void formatAttribute(OpcUaNodeAttribute *attribute,
                     QOpcUa::NodeAttribute nodeAttribute,
                     const QVariant &value,
                     QOpcUa::Types valueType)
{
    switch (nodeAttribute) {
    case QOpcUa::NodeAttribute::NodeId:
        formatNodeIdAttribute(attribute, value.toString());
        break;
    case QOpcUa::NodeAttribute::NodeClass:
        attribute->displayValue =
            nodeClassName(static_cast<QOpcUa::NodeClass>(value.toInt()));
        break;
    case QOpcUa::NodeAttribute::BrowseName:
        if (value.canConvert<QOpcUaQualifiedName>()) {
            const QOpcUaQualifiedName name = value.value<QOpcUaQualifiedName>();
            attribute->displayValue = QStringLiteral("%1, \"%2\"")
                                          .arg(name.namespaceIndex())
                                          .arg(name.name());
        }
        break;
    case QOpcUa::NodeAttribute::DisplayName:
    case QOpcUa::NodeAttribute::Description:
    case QOpcUa::NodeAttribute::InverseName:
        if (value.canConvert<QOpcUaLocalizedText>()) {
            const QOpcUaLocalizedText text = value.value<QOpcUaLocalizedText>();
            attribute->displayValue = QStringLiteral("\"%1\", \"%2\"")
                                          .arg(text.locale(), text.text());
        }
        break;
    case QOpcUa::NodeAttribute::ValueRank:
        attribute->displayValue = valueRankDisplay(value.toInt());
        break;
    case QOpcUa::NodeAttribute::DataType:
        formatDataTypeAttribute(attribute, value.toString());
        break;
    case QOpcUa::NodeAttribute::ArrayDimensions: {
        const QVariantList dimensions = value.toList();
        attribute->displayValue =
            QStringLiteral("UInt32 Array[%1]").arg(dimensions.size());
        for (int index = 0; index < dimensions.size(); ++index) {
            attribute->children.append(
                childAttribute(QStringLiteral("[%1]").arg(index),
                               dimensions.at(index).toString()));
        }
        break;
    }
    case QOpcUa::NodeAttribute::AccessLevel:
    case QOpcUa::NodeAttribute::UserAccessLevel:
        attribute->displayValue = accessLevelDisplay(value.toUInt());
        break;
    case QOpcUa::NodeAttribute::WriteMask:
    case QOpcUa::NodeAttribute::UserWriteMask:
        attribute->displayValue = writeMaskDisplay(value.toUInt());
        break;
    case QOpcUa::NodeAttribute::EventNotifier:
        attribute->displayValue =
            eventNotifierDisplay(static_cast<quint8>(value.toUInt()));
        break;
    default:
        attribute->displayValue = displayValue(value);
        break;
    }

    if (nodeAttribute == QOpcUa::NodeAttribute::Value) {
        attribute->displayValue = isValueArray(value)
            ? QStringLiteral("%1 Array[%2]")
                  .arg(valueTypeName(valueType))
                  .arg(value.toList().size())
            : valueTypeName(valueType);
    }
}

///
/// \brief Reports whether an attribute is meaningful for a given node class.
/// \param attribute Attribute to test.
/// \param nodeClass Node class to test against.
/// \return True when the attribute applies; class-agnostic attributes always return true.
///
bool attributeAppliesToNodeClass(QOpcUa::NodeAttribute attribute,
                                 QOpcUa::NodeClass nodeClass)
{
    switch (attribute) {
    case QOpcUa::NodeAttribute::Value:
    case QOpcUa::NodeAttribute::DataType:
    case QOpcUa::NodeAttribute::ValueRank:
    case QOpcUa::NodeAttribute::ArrayDimensions:
        return nodeClass == QOpcUa::NodeClass::Variable
            || nodeClass == QOpcUa::NodeClass::VariableType;
    case QOpcUa::NodeAttribute::AccessLevel:
    case QOpcUa::NodeAttribute::UserAccessLevel:
    case QOpcUa::NodeAttribute::MinimumSamplingInterval:
    case QOpcUa::NodeAttribute::Historizing:
        return nodeClass == QOpcUa::NodeClass::Variable;
    case QOpcUa::NodeAttribute::Executable:
    case QOpcUa::NodeAttribute::UserExecutable:
        return nodeClass == QOpcUa::NodeClass::Method;
    case QOpcUa::NodeAttribute::IsAbstract:
        return nodeClass == QOpcUa::NodeClass::ObjectType
            || nodeClass == QOpcUa::NodeClass::VariableType
            || nodeClass == QOpcUa::NodeClass::ReferenceType
            || nodeClass == QOpcUa::NodeClass::DataType;
    case QOpcUa::NodeAttribute::Symmetric:
    case QOpcUa::NodeAttribute::InverseName:
        return nodeClass == QOpcUa::NodeClass::ReferenceType;
    case QOpcUa::NodeAttribute::ContainsNoLoops:
        return nodeClass == QOpcUa::NodeClass::View;
    case QOpcUa::NodeAttribute::EventNotifier:
        return nodeClass == QOpcUa::NodeClass::Object
            || nodeClass == QOpcUa::NodeClass::View;
    default:
        return true;
    }
}

///
/// \brief Maps a namespace-0 built-in DataType NodeId to its OPC UA value type.
/// \param nodeId DataType NodeId, expected as "ns=0;i=...".
/// \return Matching value type, or Undefined for non-builtin or unparsable ids.
///
QOpcUa::Types valueTypeForDataType(const QString &nodeId)
{
    bool ok = false;
    const int identifier = nodeId.section(QLatin1String("i="), 1).toInt(&ok);
    if (!ok || !nodeId.startsWith(QLatin1String("ns=0;")))
        return QOpcUa::Types::Undefined;
    switch (identifier) {
    case 1: return QOpcUa::Types::Boolean;
    case 2: return QOpcUa::Types::SByte;
    case 3: return QOpcUa::Types::Byte;
    case 4: return QOpcUa::Types::Int16;
    case 5: return QOpcUa::Types::UInt16;
    case 6: return QOpcUa::Types::Int32;
    case 7: return QOpcUa::Types::UInt32;
    case 8: return QOpcUa::Types::Int64;
    case 9: return QOpcUa::Types::UInt64;
    case 10: return QOpcUa::Types::Float;
    case 11: return QOpcUa::Types::Double;
    case 12: return QOpcUa::Types::String;
    case 13: return QOpcUa::Types::DateTime;
    case 14: return QOpcUa::Types::Guid;
    case 15: return QOpcUa::Types::ByteString;
    case 16: return QOpcUa::Types::XmlElement;
    case 17: return QOpcUa::Types::NodeId;
    case 18: return QOpcUa::Types::ExpandedNodeId;
    case 19: return QOpcUa::Types::StatusCode;
    case 20: return QOpcUa::Types::QualifiedName;
    case 21: return QOpcUa::Types::LocalizedText;
    case 22: return QOpcUa::Types::ExtensionObject;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    case 25: return QOpcUa::Types::DiagnosticInfo;
#endif
    default: return QOpcUa::Types::Undefined;
    }
}

///
/// \brief Converts text to a typed scalar, range-checking integral types.
/// \param text Source text.
/// \param type Target OPC UA value type.
/// \param ok Receives the conversion status; must not be null.
/// \return Converted scalar, or an invalid variant on failure.
///
QVariant scalarFromText(const QString &text, QOpcUa::Types type, bool *ok)
{
    *ok = true;
    switch (type) {
    case QOpcUa::Types::Boolean:
        if (text.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0 || text == QLatin1String("1"))
            return true;
        if (text.compare(QLatin1String("false"), Qt::CaseInsensitive) == 0 || text == QLatin1String("0"))
            return false;
        break;
    case QOpcUa::Types::SByte: {
        const int value = text.toInt(ok);
        *ok = *ok && value >= std::numeric_limits<qint8>::min()
            && value <= std::numeric_limits<qint8>::max();
        return QVariant::fromValue(static_cast<qint8>(value));
    }
    case QOpcUa::Types::Byte: {
        const uint value = text.toUInt(ok);
        *ok = *ok && value <= std::numeric_limits<quint8>::max();
        return QVariant::fromValue(static_cast<quint8>(value));
    }
    case QOpcUa::Types::Int16: {
        const int value = text.toInt(ok);
        *ok = *ok && value >= std::numeric_limits<qint16>::min()
            && value <= std::numeric_limits<qint16>::max();
        return QVariant::fromValue(static_cast<qint16>(value));
    }
    case QOpcUa::Types::UInt16: {
        const uint value = text.toUInt(ok);
        *ok = *ok && value <= std::numeric_limits<quint16>::max();
        return QVariant::fromValue(static_cast<quint16>(value));
    }
    case QOpcUa::Types::Int32: return text.toInt(ok);
    case QOpcUa::Types::UInt32:
    case QOpcUa::Types::StatusCode: return text.toUInt(ok);
    case QOpcUa::Types::Int64: return text.toLongLong(ok);
    case QOpcUa::Types::UInt64: return text.toULongLong(ok);
    case QOpcUa::Types::Float: return text.toFloat(ok);
    case QOpcUa::Types::Double: return text.toDouble(ok);
    case QOpcUa::Types::DateTime: {
        const QDateTime dateTime = QDateTime::fromString(text, Qt::ISODateWithMs);
        *ok = dateTime.isValid();
        return dateTime;
    }
    case QOpcUa::Types::Guid: {
        const QUuid uuid(text);
        *ok = !uuid.isNull();
        return uuid;
    }
    case QOpcUa::Types::ByteString:
        return QByteArray::fromBase64(text.toLatin1());
    case QOpcUa::Types::String:
    case QOpcUa::Types::XmlElement:
    case QOpcUa::Types::NodeId:
    case QOpcUa::Types::ExpandedNodeId:
    case QOpcUa::Types::LocalizedText:
    case QOpcUa::Types::QualifiedName:
    case QOpcUa::Types::Undefined:
        return text;
    default:
        break;
    }
    *ok = false;
    return {};
}

} // namespace OpcUaFormat
