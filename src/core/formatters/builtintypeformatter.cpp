// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file builtintypeformatter.cpp
/// \brief Implements display text for the OPC UA types Qt carries in classes of their own.
///

#include "attributeformatter.h"

#include <QOpcUaArgument>
#include <QOpcUaAxisInformation>
#include <QOpcUaComplexNumber>
#include <QOpcUaDoubleComplexNumber>
#include <QOpcUaEUInformation>
#include <QOpcUaExpandedNodeId>
#include <QOpcUaExtensionObject>
#include <QOpcUaLocalizedText>
#include <QOpcUaMultiDimensionalArray>
#include <QOpcUaQualifiedName>
#include <QOpcUaRange>
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

    // A Variant field only wraps the value it transports; the wrapper itself says nothing.
    if (type == qMetaTypeId<QOpcUaVariant>())
        return displayValue(value.value<QOpcUaVariant>().value());

    if (type == qMetaTypeId<QOpcUaExtensionObject>())
        return extensionObjectText(value.value<QOpcUaExtensionObject>());

    return std::nullopt;
}

} // namespace OpcUaFormat
