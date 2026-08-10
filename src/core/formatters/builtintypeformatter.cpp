// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file builtintypeformatter.cpp
/// \brief Implements display text for the OPC UA types Qt carries in classes of their own.
///

#include "attributeformatter.h"

#include <QMetaEnum>

#include <QOpcUaArgument>
#include <QOpcUaAxisInformation>
#include <QOpcUaComplexNumber>
#include <QOpcUaContentFilterElement>
#include <QOpcUaDataValue>
#include <QOpcUaDiagnosticInfo>
#include <QOpcUaDoubleComplexNumber>
#include <QOpcUaEUInformation>
#include <QOpcUaElementOperand>
#include <QOpcUaEnumDefinition>
#include <QOpcUaEnumField>
#include <QOpcUaExpandedNodeId>
#include <QOpcUaExtensionObject>
#include <QOpcUaLiteralOperand>
#include <QOpcUaLocalizedText>
#include <QOpcUaMonitoringParameters>
#include <QOpcUaMultiDimensionalArray>
#include <QOpcUaQualifiedName>
#include <QOpcUaRange>
#include <QOpcUaRelativePathElement>
#include <QOpcUaSimpleAttributeOperand>
#include <QOpcUaStructureDefinition>
#include <QOpcUaStructureField>
#include <QOpcUaVariant>
#include <QOpcUaXValue>

namespace OpcUaFormat {

namespace {

///
/// \brief Joins named fields into the brace notation the decoded structures use.
/// \param typeName OPC UA name of the structure.
/// \param fields Field texts, each already formatted as "Name: value".
/// \return Structure notation, "TypeName {Name: value, ...}".
///
QString structText(const QString &typeName, const QStringList &fields)
{
    return QStringLiteral("%1 {%2}").arg(typeName, fields.join(QStringLiteral(", ")));
}

///
/// \brief Formats one structure field.
/// \param name Field name as the specification writes it.
/// \param value Field value.
/// \return Field text, "Name: value".
///
QString field(const QString &name, const QVariant &value)
{
    return QStringLiteral("%1: %2").arg(name, displayValue(value));
}

///
/// \brief Wraps a typed list so it formats like any other array.
/// \param items List to wrap.
/// \return The same elements as a variant list.
///
template <typename T>
QVariant variantList(const QList<T> &items)
{
    QVariantList list;
    list.reserve(items.size());
    for (const T &item : items)
        list.append(QVariant::fromValue(item));
    return list;
}

///
/// \brief Names an axis scale.
/// \param scale Scale to name.
/// \return Scale name, or its numeric value when unrecognised.
///
QString axisScaleName(QOpcUa::AxisScale scale)
{
    switch (scale) {
    case QOpcUa::AxisScale::Linear: return QStringLiteral("Linear");
    case QOpcUa::AxisScale::Log: return QStringLiteral("Log");
    case QOpcUa::AxisScale::Ln: return QStringLiteral("Ln");
    }
    return QString::number(static_cast<quint32>(scale));
}

///
/// \brief Names a structure kind.
/// \param type Structure type to name.
/// \return Type name, or its numeric value when unrecognised.
///
QString structureTypeName(QOpcUaStructureDefinition::StructureType type)
{
    switch (type) {
    case QOpcUaStructureDefinition::StructureType::Structure:
        return QStringLiteral("Structure");
    case QOpcUaStructureDefinition::StructureType::StructureWithOptionalFields:
        return QStringLiteral("StructureWithOptionalFields");
    case QOpcUaStructureDefinition::StructureType::Union:
        return QStringLiteral("Union");
    }
    return QString::number(static_cast<int>(type));
}

///
/// \brief Names a content-filter operator.
/// \param filterOperator Operator to name.
/// \return Operator name, or its numeric value when unrecognised.
///
QString filterOperatorName(QOpcUaContentFilterElement::FilterOperator filterOperator)
{
    switch (filterOperator) {
    case QOpcUaContentFilterElement::Equals: return QStringLiteral("Equals");
    case QOpcUaContentFilterElement::IsNull: return QStringLiteral("IsNull");
    case QOpcUaContentFilterElement::GreaterThan: return QStringLiteral("GreaterThan");
    case QOpcUaContentFilterElement::LessThan: return QStringLiteral("LessThan");
    case QOpcUaContentFilterElement::GreaterThanOrEqual: return QStringLiteral("GreaterThanOrEqual");
    case QOpcUaContentFilterElement::LessThanOrEqual: return QStringLiteral("LessThanOrEqual");
    case QOpcUaContentFilterElement::Like: return QStringLiteral("Like");
    case QOpcUaContentFilterElement::Not: return QStringLiteral("Not");
    case QOpcUaContentFilterElement::Between: return QStringLiteral("Between");
    case QOpcUaContentFilterElement::InList: return QStringLiteral("InList");
    case QOpcUaContentFilterElement::And: return QStringLiteral("And");
    case QOpcUaContentFilterElement::Or: return QStringLiteral("Or");
    case QOpcUaContentFilterElement::Cast: return QStringLiteral("Cast");
    case QOpcUaContentFilterElement::InView: return QStringLiteral("InView");
    case QOpcUaContentFilterElement::OfType: return QStringLiteral("OfType");
    case QOpcUaContentFilterElement::RelatedTo: return QStringLiteral("RelatedTo");
    case QOpcUaContentFilterElement::BitwiseAnd: return QStringLiteral("BitwiseAnd");
    case QOpcUaContentFilterElement::BitwiseOr: return QStringLiteral("BitwiseOr");
    }
    return QString::number(static_cast<quint32>(filterOperator));
}

///
/// \brief Names a node attribute.
/// \param attribute Attribute to name.
/// \return Attribute name, or its numeric value when unrecognised.
///
QString nodeAttributeName(QOpcUa::NodeAttribute attribute)
{
    const char *key = QMetaEnum::fromType<QOpcUa::NodeAttribute>()
                          .valueToKey(static_cast<int>(attribute));
    return key ? QString::fromLatin1(key) : QString::number(static_cast<quint32>(attribute));
}

///
/// \brief Describes an ExtensionObject that stayed opaque.
/// \param object Extension object to describe.
/// \return Encoded XML body, or the encoding type and body size of a binary body.
///
/// A binary body only decodes with the server's type definitions at hand.
///
QString extensionObjectText(const QOpcUaExtensionObject &object)
{
    const QString typeName = object.encodingTypeId().isEmpty()
        ? QStringLiteral("ExtensionObject")
        : standardNodeDisplayName(object.encodingTypeId());
    if (object.encoding() == QOpcUaExtensionObject::Encoding::NoBody)
        return typeName;
    if (object.encoding() == QOpcUaExtensionObject::Encoding::Xml)
        return QString::fromUtf8(object.encodedBody());
    return QStringLiteral("%1 [%2 bytes]").arg(typeName).arg(object.encodedBody().size());
}

} // namespace

///
/// \brief Renders the OPC UA types Qt carries in classes of their own.
/// \param value Variant to format.
/// \return Display text, or nullopt when the value is not one of those types.
///
std::optional<QString> builtinTypeText(const QVariant &value)
{
    const int type = value.userType();

    if (type == qMetaTypeId<QOpcUaQualifiedName>()) {
        const QOpcUaQualifiedName name = value.value<QOpcUaQualifiedName>();
        return QStringLiteral("%1, \"%2\"").arg(name.namespaceIndex()).arg(name.name());
    }

    if (type == qMetaTypeId<QOpcUaLocalizedText>()) {
        const QOpcUaLocalizedText text = value.value<QOpcUaLocalizedText>();
        return QStringLiteral("\"%1\", \"%2\"").arg(text.locale(), text.text());
    }

    if (type == qMetaTypeId<QOpcUaExpandedNodeId>()) {
        const QOpcUaExpandedNodeId nodeId = value.value<QOpcUaExpandedNodeId>();
        QStringList parts;
        if (nodeId.serverIndex() != 0)
            parts.append(QStringLiteral("svr=%1").arg(nodeId.serverIndex()));
        if (!nodeId.namespaceUri().isEmpty())
            parts.append(QStringLiteral("nsu=%1").arg(nodeId.namespaceUri()));
        parts.append(nodeId.nodeId());
        return parts.join(QLatin1Char(';'));
    }

    if (type == qMetaTypeId<QOpcUaRange>()) {
        const QOpcUaRange range = value.value<QOpcUaRange>();
        return structText(QStringLiteral("Range"),
                          {field(QStringLiteral("Low"), range.low()),
                           field(QStringLiteral("High"), range.high())});
    }

    if (type == qMetaTypeId<QOpcUaEUInformation>()) {
        const QOpcUaEUInformation info = value.value<QOpcUaEUInformation>();
        return structText(QStringLiteral("EUInformation"),
                          {field(QStringLiteral("NamespaceUri"), info.namespaceUri()),
                           field(QStringLiteral("UnitId"), info.unitId()),
                           field(QStringLiteral("DisplayName"), info.displayName()),
                           field(QStringLiteral("Description"), info.description())});
    }

    if (type == qMetaTypeId<QOpcUaComplexNumber>()) {
        const QOpcUaComplexNumber number = value.value<QOpcUaComplexNumber>();
        return structText(QStringLiteral("ComplexNumberType"),
                          {field(QStringLiteral("Real"), number.real()),
                           field(QStringLiteral("Imaginary"), number.imaginary())});
    }

    if (type == qMetaTypeId<QOpcUaDoubleComplexNumber>()) {
        const QOpcUaDoubleComplexNumber number = value.value<QOpcUaDoubleComplexNumber>();
        return structText(QStringLiteral("DoubleComplexNumberType"),
                          {field(QStringLiteral("Real"), number.real()),
                           field(QStringLiteral("Imaginary"), number.imaginary())});
    }

    if (type == qMetaTypeId<QOpcUaXValue>()) {
        const QOpcUaXValue xValue = value.value<QOpcUaXValue>();
        return structText(QStringLiteral("XVType"),
                          {field(QStringLiteral("X"), xValue.x()),
                           field(QStringLiteral("Value"), xValue.value())});
    }

    if (type == qMetaTypeId<QOpcUaAxisInformation>()) {
        const QOpcUaAxisInformation axis = value.value<QOpcUaAxisInformation>();
        QVariantList steps;
        steps.reserve(axis.axisSteps().size());
        for (double step : axis.axisSteps())
            steps.append(step);
        return structText(QStringLiteral("AxisInformation"),
                          {field(QStringLiteral("EngineeringUnits"), axis.engineeringUnits()),
                           field(QStringLiteral("EURange"), axis.eURange()),
                           field(QStringLiteral("Title"), axis.title()),
                           QStringLiteral("AxisScaleType: %1").arg(axisScaleName(axis.axisScaleType())),
                           field(QStringLiteral("AxisSteps"), steps)});
    }

    if (type == qMetaTypeId<QOpcUaArgument>()) {
        const QOpcUaArgument argument = value.value<QOpcUaArgument>();
        QVariantList dimensions;
        dimensions.reserve(argument.arrayDimensions().size());
        for (quint32 dimension : argument.arrayDimensions())
            dimensions.append(dimension);
        return structText(QStringLiteral("Argument"),
                          {field(QStringLiteral("Name"), argument.name()),
                           QStringLiteral("DataType: %1").arg(dataTypeDisplay(argument.dataTypeId())),
                           QStringLiteral("ValueRank: %1").arg(valueRankDisplay(argument.valueRank())),
                           field(QStringLiteral("ArrayDimensions"), dimensions),
                           field(QStringLiteral("Description"), argument.description())});
    }

    if (type == qMetaTypeId<QOpcUaMultiDimensionalArray>()) {
        const QOpcUaMultiDimensionalArray array = value.value<QOpcUaMultiDimensionalArray>();
        QStringList dimensions;
        dimensions.reserve(array.arrayDimensions().size());
        for (quint32 dimension : array.arrayDimensions())
            dimensions.append(QString::number(dimension));
        return QStringLiteral("%1 [%2]")
            .arg(displayValue(array.valueArray()), dimensions.join(QLatin1Char('x')));
    }

    if (type == qMetaTypeId<QOpcUa::UaStatusCode>())
        return statusDisplay(value.value<QOpcUa::UaStatusCode>());

    if (type == qMetaTypeId<QOpcUaDataValue>()) {
        const QOpcUaDataValue dataValue = value.value<QOpcUaDataValue>();
        QStringList fields{field(QStringLiteral("Value"), dataValue.value()),
                           QStringLiteral("StatusCode: %1").arg(statusDisplay(dataValue.statusCode()))};
        if (dataValue.sourceTimestamp().isValid())
            fields.append(field(QStringLiteral("SourceTimestamp"), dataValue.sourceTimestamp()));
        if (dataValue.serverTimestamp().isValid())
            fields.append(field(QStringLiteral("ServerTimestamp"), dataValue.serverTimestamp()));
        return structText(QStringLiteral("DataValue"), fields);
    }

    if (type == qMetaTypeId<QOpcUaDiagnosticInfo>()) {
        const QOpcUaDiagnosticInfo info = value.value<QOpcUaDiagnosticInfo>();
        QStringList fields;
        if (info.hasSymbolicId())
            fields.append(field(QStringLiteral("SymbolicId"), info.symbolicId()));
        if (info.hasNamespaceUri())
            fields.append(field(QStringLiteral("NamespaceUri"), info.namespaceUri()));
        if (info.hasLocale())
            fields.append(field(QStringLiteral("Locale"), info.locale()));
        if (info.hasLocalizedText())
            fields.append(field(QStringLiteral("LocalizedText"), info.localizedText()));
        if (info.hasAdditionalInfo())
            fields.append(field(QStringLiteral("AdditionalInfo"), info.additionalInfo()));
        if (info.hasInnerStatusCode())
            fields.append(QStringLiteral("InnerStatusCode: %1").arg(statusDisplay(info.innerStatusCode())));
        if (info.hasInnerDiagnosticInfo())
            fields.append(field(QStringLiteral("InnerDiagnosticInfo"), info.innerDiagnosticInfo()));
        return structText(QStringLiteral("DiagnosticInfo"), fields);
    }

    if (type == qMetaTypeId<QOpcUaEnumField>()) {
        const QOpcUaEnumField enumField = value.value<QOpcUaEnumField>();
        return structText(QStringLiteral("EnumField"),
                          {field(QStringLiteral("Name"), enumField.name()),
                           field(QStringLiteral("Value"), enumField.value()),
                           field(QStringLiteral("DisplayName"), enumField.displayName()),
                           field(QStringLiteral("Description"), enumField.description())});
    }

    if (type == qMetaTypeId<QOpcUaEnumDefinition>()) {
        const QOpcUaEnumDefinition definition = value.value<QOpcUaEnumDefinition>();
        return structText(QStringLiteral("EnumDefinition"),
                          {field(QStringLiteral("Fields"), variantList(definition.fields()))});
    }

    if (type == qMetaTypeId<QOpcUaStructureField>()) {
        const QOpcUaStructureField structField = value.value<QOpcUaStructureField>();
        QVariantList dimensions;
        dimensions.reserve(structField.arrayDimensions().size());
        for (quint32 dimension : structField.arrayDimensions())
            dimensions.append(dimension);
        return structText(QStringLiteral("StructureField"),
                          {field(QStringLiteral("Name"), structField.name()),
                           QStringLiteral("DataType: %1").arg(dataTypeDisplay(structField.dataType())),
                           QStringLiteral("ValueRank: %1").arg(valueRankDisplay(structField.valueRank())),
                           field(QStringLiteral("ArrayDimensions"), dimensions),
                           field(QStringLiteral("IsOptional"), structField.isOptional())});
    }

    if (type == qMetaTypeId<QOpcUaStructureDefinition>()) {
        const QOpcUaStructureDefinition definition = value.value<QOpcUaStructureDefinition>();
        return structText(
            QStringLiteral("StructureDefinition"),
            {QStringLiteral("StructureType: %1").arg(structureTypeName(definition.structureType())),
             QStringLiteral("BaseDataType: %1").arg(dataTypeDisplay(definition.baseDataType())),
             field(QStringLiteral("DefaultEncodingId"), definition.defaultEncodingId()),
             field(QStringLiteral("Fields"), variantList(definition.fields()))});
    }

    if (type == qMetaTypeId<QOpcUaSimpleAttributeOperand>()) {
        const QOpcUaSimpleAttributeOperand operand = value.value<QOpcUaSimpleAttributeOperand>();
        return structText(
            QStringLiteral("SimpleAttributeOperand"),
            {QStringLiteral("TypeId: %1").arg(standardNodeDisplayName(operand.typeId())),
             field(QStringLiteral("BrowsePath"), variantList(operand.browsePath())),
             QStringLiteral("AttributeId: %1").arg(nodeAttributeName(operand.attributeId())),
             field(QStringLiteral("IndexRange"), operand.indexRange())});
    }

    if (type == qMetaTypeId<QOpcUaLiteralOperand>()) {
        const QOpcUaLiteralOperand operand = value.value<QOpcUaLiteralOperand>();
        return structText(QStringLiteral("LiteralOperand"),
                          {field(QStringLiteral("Value"), operand.value()),
                           QStringLiteral("Type: %1").arg(valueTypeName(operand.type()))});
    }

    if (type == qMetaTypeId<QOpcUaElementOperand>()) {
        return structText(QStringLiteral("ElementOperand"),
                          {field(QStringLiteral("Index"),
                                 value.value<QOpcUaElementOperand>().index())});
    }

    if (type == qMetaTypeId<QOpcUaRelativePathElement>()) {
        const QOpcUaRelativePathElement element = value.value<QOpcUaRelativePathElement>();
        return structText(
            QStringLiteral("RelativePathElement"),
            {QStringLiteral("ReferenceTypeId: %1").arg(standardNodeDisplayName(element.referenceTypeId())),
             field(QStringLiteral("IsInverse"), element.isInverse()),
             field(QStringLiteral("IncludeSubtypes"), element.includeSubtypes()),
             field(QStringLiteral("TargetName"), element.targetName())});
    }

    if (type == qMetaTypeId<QOpcUaContentFilterElement>()) {
        const QOpcUaContentFilterElement element = value.value<QOpcUaContentFilterElement>();
        return structText(
            QStringLiteral("ContentFilterElement"),
            {QStringLiteral("FilterOperator: %1").arg(filterOperatorName(element.filterOperator())),
             field(QStringLiteral("FilterOperands"), QVariant(element.filterOperands()))});
    }

    if (type == qMetaTypeId<QOpcUaMonitoringParameters::EventFilter>()) {
        const auto filter = value.value<QOpcUaMonitoringParameters::EventFilter>();
        return structText(QStringLiteral("EventFilter"),
                          {field(QStringLiteral("SelectClauses"), variantList(filter.selectClauses())),
                           field(QStringLiteral("WhereClause"), variantList(filter.whereClause()))});
    }

    // A Variant field only wraps the value it transports; the wrapper itself says nothing.
    if (type == qMetaTypeId<QOpcUaVariant>())
        return displayValue(value.value<QOpcUaVariant>().value());

    if (type == qMetaTypeId<QOpcUaExtensionObject>())
        return extensionObjectText(value.value<QOpcUaExtensionObject>());

    return std::nullopt;
}

} // namespace OpcUaFormat
