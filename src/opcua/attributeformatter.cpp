// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file attributeformatter.cpp
/// \brief Implements the OPC UA value and attribute formatting helpers.
///

#include "attributeformatter.h"

#include <QMetaEnum>
#include <QObject>

#include <QOpcUaLocalizedText>
#include <QOpcUaQualifiedName>

namespace OpcUaFormat {

bool isValueArray(const QVariant &value)
{
    return value.userType() != QMetaType::QString
        && value.userType() != QMetaType::QByteArray
        && value.canConvert<QVariantList>();
}

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

QString securityModeName(QOpcUaEndpointDescription::MessageSecurityMode mode)
{
    switch (mode) {
    case QOpcUaEndpointDescription::None: return QObject::tr("None");
    case QOpcUaEndpointDescription::Sign: return QObject::tr("Sign");
    case QOpcUaEndpointDescription::SignAndEncrypt: return QObject::tr("Sign & Encrypt");
    default: return QObject::tr("Invalid");
    }
}

QString statusName(QOpcUa::UaStatusCode status)
{
    return QOpcUa::statusToString(status);
}

QString statusDisplay(QOpcUa::UaStatusCode status)
{
    return QStringLiteral("%1 (0x%2)")
        .arg(statusName(status))
        .arg(static_cast<quint32>(status), 8, 16, QLatin1Char('0'));
}

QString timestampDisplay(const QDateTime &timestamp)
{
    return timestamp.isValid()
        ? timestamp.toLocalTime().toString(QStringLiteral("dd.MM.yyyy H:mm:ss.zzz"))
        : QString();
}

QString valueTypeName(QOpcUa::Types type)
{
    const char *key = QMetaEnum::fromType<QOpcUa::Types>().valueToKey(type);
    return key ? QString::fromLatin1(key) : QObject::tr("Unknown");
}

QString nodeClassName(QOpcUa::NodeClass nodeClass)
{
    const char *key = QMetaEnum::fromType<QOpcUa::NodeClass>()
                          .valueToKey(static_cast<int>(nodeClass));
    return key ? QString::fromLatin1(key) : QString::number(static_cast<int>(nodeClass));
}

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

OpcUaNodeAttribute childAttribute(const QString &name, const QString &displayValue)
{
    OpcUaNodeAttribute child;
    child.name = name;
    child.displayValue = displayValue;
    return child;
}

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

void formatNodeIdAttribute(OpcUaNodeAttribute *attribute, const QString &nodeId)
{
    attribute->displayValue = nodeId;
    addNodeIdChildren(attribute, nodeId);
}

void formatDataTypeAttribute(OpcUaNodeAttribute *attribute, const QString &nodeId)
{
    const QOpcUa::Types type = valueTypeForDataType(nodeId);
    attribute->displayValue = type == QOpcUa::Types::Undefined
        ? nodeId
        : valueTypeName(type);
    addNodeIdChildren(attribute, nodeId);
}

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
    case 25: return QOpcUa::Types::DiagnosticInfo;
    default: return QOpcUa::Types::Undefined;
    }
}

} // namespace OpcUaFormat
