// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file nodeidformatter.cpp
/// \brief Implements OPC UA NodeId, node class, and type-name formatting helpers.
///

#include "attributeformatter.h"

#include <QMetaEnum>
#include <QObject>

#include <QtOpcUa/qopcuanodeids.h>

namespace OpcUaFormat {

namespace {

///
/// \brief Extracts a namespace-0 numeric NodeId identifier.
/// \param nodeId NodeId text in expanded or compact string form.
/// \param identifier Resolved numeric identifier.
/// \return True when the text denotes a numeric namespace-0 NodeId.
///
bool namespace0NumericId(const QString &nodeId, int *identifier)
{
    quint16 namespaceIndex = 0;
    QString numeric;
    char identifierType = '\0';
    if (!QOpcUa::nodeIdStringSplit(nodeId.trimmed(), &namespaceIndex, &numeric, &identifierType))
        return false;
    if (namespaceIndex != 0 || identifierType != 'i')
        return false;

    bool ok = false;
    const int value = numeric.toInt(&ok);
    if (ok)
        *identifier = value;
    return ok;
}

} // namespace

///
/// \brief Resolves a DataType NodeId to a built-in OPC UA type name.
/// \param nodeId DataType NodeId string.
/// \return Built-in type name, or the original NodeId for custom types.
///
QString dataTypeDisplay(const QString &nodeId)
{
    const QOpcUa::Types type = valueTypeForDataType(nodeId);
    return type == QOpcUa::Types::Undefined ? nodeId : valueTypeName(type);
}

///
/// \brief Resolves a known namespace-0 NodeId to its BrowseName.
/// \param nodeId NodeId string.
/// \return Standard BrowseName, or the original NodeId for custom/unknown nodes.
///
QString standardNodeDisplayName(const QString &nodeId)
{
    int identifier = 0;
    if (!namespace0NumericId(nodeId, &identifier))
        return nodeId;

    const QMetaEnum metaEnum = QMetaEnum::fromType<QOpcUa::NodeIds::Namespace0>();
    const char *key = metaEnum.valueToKey(identifier);
    return key ? QString::fromLatin1(key) : nodeId;
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
    attribute->displayValue = dataTypeDisplay(nodeId);
    addNodeIdChildren(attribute, nodeId);
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
    case 25: return QOpcUa::Types::DiagnosticInfo;
    default: return QOpcUa::Types::Undefined;
    }
}

} // namespace OpcUaFormat
