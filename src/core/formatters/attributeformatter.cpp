// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file attributeformatter.cpp
/// \brief Implements OPC UA attribute row formatting.
///

#include "attributeformatter.h"

#include <QOpcUaLocalizedText>
#include <QOpcUaQualifiedName>

namespace OpcUaFormat {

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

} // namespace OpcUaFormat
