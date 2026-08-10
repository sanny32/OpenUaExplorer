// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_attributeformatter_values.cpp
/// \brief Tests formatting of built-in and transported OPC UA values.
///

#include <QDateTime>
#include <QTest>
#include <QTimeZone>
#include <QUuid>
#include <QVariant>

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
#include <QOpcUaGenericStructValue>
#include <QOpcUaLiteralOperand>
#include <QOpcUaLocalizedText>
#include <QOpcUaMonitoringParameters>
#include <QOpcUaMultiDimensionalArray>
#include <QOpcUaQualifiedName>
#include <QOpcUaRange>
#include <QOpcUaRelativePathElement>
#include <QOpcUaSimpleAttributeOperand>
#include <QOpcUaVariant>
#include <QOpcUaXValue>
#include <QOpcUaStructureDefinition>
#include <QOpcUaStructureField>

#include "formatters/attributeformatter.h"
#include "testimages.h"

using namespace OpcUaFormat;
using TestImages::encodedPng;

///
/// \brief Tests the related behavior in an isolated Qt Test executable.
///
class TestAttributeFormatterValues : public QObject
{
    Q_OBJECT

private slots:
    void builtinTypesFormatWithRulesOfTheirOwn();
    void undecodedExtensionObjectsNameTheirEncoding();
    void everyTransportedTypeRendersAsText();
    void formatAttributeDispatchesPerAttribute();
};

void TestAttributeFormatterValues::builtinTypesFormatWithRulesOfTheirOwn()
{
    // QVariant::toString() is empty for all of these, so every one needs a rule of its own.
    QOpcUaQualifiedName qualified;
    qualified.setNamespaceIndex(2);
    qualified.setName(QStringLiteral("Colour"));
    QCOMPARE(displayValue(QVariant::fromValue(qualified)), QStringLiteral("2, \"Colour\""));

    QOpcUaLocalizedText localized;
    localized.setLocale(QStringLiteral("en"));
    localized.setText(QStringLiteral("Sensor"));
    QCOMPARE(displayValue(QVariant::fromValue(localized)), QStringLiteral("\"en\", \"Sensor\""));

    QOpcUaExpandedNodeId expanded;
    expanded.setNamespaceUri(QStringLiteral("urn:example"));
    expanded.setNodeId(QStringLiteral("ns=2;i=5"));
    QCOMPARE(displayValue(QVariant::fromValue(expanded)),
             QStringLiteral("nsu=urn:example;ns=2;i=5"));

    QOpcUaRange range;
    range.setLow(0.0);
    range.setHigh(100.0);
    QCOMPARE(displayValue(QVariant::fromValue(range)),
             QStringLiteral("Range {Low: 0, High: 100}"));

    QOpcUaEUInformation unit;
    unit.setNamespaceUri(QStringLiteral("urn:units"));
    unit.setUnitId(4408652);
    unit.setDisplayName(QOpcUaLocalizedText(QStringLiteral("en"), QStringLiteral("°C")));
    QCOMPARE(displayValue(QVariant::fromValue(unit)),
             QStringLiteral("EUInformation {NamespaceUri: urn:units, UnitId: 4408652, "
                            "DisplayName: \"en\", \"°C\", Description: \"\", \"\"}"));

    QOpcUaComplexNumber complex;
    complex.setReal(1.5f);
    complex.setImaginary(-2.0f);
    QCOMPARE(displayValue(QVariant::fromValue(complex)),
             QStringLiteral("ComplexNumberType {Real: 1.5, Imaginary: -2}"));

    QCOMPARE(displayValue(QVariant::fromValue(QOpcUa::UaStatusCode::BadInternalError)),
             QStringLiteral("BadInternalError (0x80020000)"));

    QOpcUaEnumField enumField;
    enumField.setName(QStringLiteral("Running"));
    enumField.setValue(1);
    QCOMPARE(displayValue(QVariant::fromValue(enumField)),
             QStringLiteral("EnumField {Name: Running, Value: 1, "
                            "DisplayName: \"\", \"\", Description: \"\", \"\"}"));

    QOpcUaStructureField structField;
    structField.setName(QStringLiteral("Low"));
    structField.setDataType(QStringLiteral("ns=0;i=11"));
    QCOMPARE(displayValue(QVariant::fromValue(structField)),
             QStringLiteral("StructureField {Name: Low, DataType: Double, "
                            "ValueRank: -1 (Scalar), ArrayDimensions: [], IsOptional: false}"));

    // An array of them is still an array, and its elements format one by one.
    QCOMPARE(displayValue(QVariant::fromValue(QList<QOpcUaQualifiedName>{qualified, qualified})),
             QStringLiteral("[2, \"Colour\", 2, \"Colour\"]"));
}

void TestAttributeFormatterValues::undecodedExtensionObjectsNameTheirEncoding()
{
    QOpcUaExtensionObject binary;
    binary.setBinaryEncodedBody(QByteArray(4, '\0'), QStringLiteral("ns=2;i=3062"));
    QCOMPARE(displayValue(QVariant::fromValue(binary)), QStringLiteral("ns=2;i=3062 [4 bytes]"));

    QOpcUaExtensionObject empty;
    QCOMPARE(displayValue(QVariant::fromValue(empty)), QStringLiteral("ExtensionObject"));

    QOpcUaExtensionObject xml;
    xml.setXmlEncodedBody(QByteArray("<Value>1</Value>"), QStringLiteral("ns=2;i=3062"));
    QCOMPARE(displayValue(QVariant::fromValue(xml)), QStringLiteral("<Value>1</Value>"));
}

void TestAttributeFormatterValues::everyTransportedTypeRendersAsText()
{
    // Every type the open62541 plugin and the struct decoder can put into a value: none of
    // them may fall through to QVariant::toString(), which is empty for all of the gadgets.
    const QVector<QVariant> values{
        QVariant(true),
        QVariant::fromValue<qint8>(-1),
        QVariant::fromValue<quint8>(1),
        QVariant::fromValue<qint16>(-2),
        QVariant::fromValue<quint16>(2),
        QVariant(-3),
        QVariant(3u),
        QVariant::fromValue<qint64>(-4),
        QVariant::fromValue<quint64>(4),
        QVariant::fromValue<float>(1.5f),
        QVariant(2.5),
        QVariant(QStringLiteral("text")),
        QVariant(QByteArray("\x01")),
        QVariant(QDateTime::currentDateTimeUtc()),
        QVariant::fromValue(QUuid::createUuid()),
        QVariant::fromValue(QOpcUa::UaStatusCode::BadInternalError),
        QVariant::fromValue(QOpcUaQualifiedName(1, QStringLiteral("Name"))),
        QVariant::fromValue(QOpcUaLocalizedText(QStringLiteral("en"), QStringLiteral("Text"))),
        QVariant::fromValue(QOpcUaExpandedNodeId(QStringLiteral("ns=1;i=2"))),
        QVariant::fromValue(QOpcUaRange(0.0, 1.0)),
        QVariant::fromValue(QOpcUaEUInformation()),
        QVariant::fromValue(QOpcUaComplexNumber(1.0f, 2.0f)),
        QVariant::fromValue(QOpcUaDoubleComplexNumber(1.0, 2.0)),
        QVariant::fromValue(QOpcUaXValue(1.0, 2.0f)),
        QVariant::fromValue(QOpcUaAxisInformation()),
        QVariant::fromValue(QOpcUaArgument()),
        QVariant::fromValue(QOpcUaMultiDimensionalArray(QVariantList{1, 2}, {2})),
        QVariant::fromValue(QOpcUaExtensionObject()),
        QVariant::fromValue(QOpcUaVariant(QOpcUaVariant::ValueType::Int32, 5)),
        QVariant::fromValue(QOpcUaDataValue()),
        QVariant::fromValue(QOpcUaDiagnosticInfo()),
        QVariant::fromValue(QOpcUaEnumField()),
        QVariant::fromValue(QOpcUaEnumDefinition()),
        QVariant::fromValue(QOpcUaStructureField()),
        QVariant::fromValue(QOpcUaStructureDefinition()),
        QVariant::fromValue(QOpcUaSimpleAttributeOperand()),
        QVariant::fromValue(QOpcUaLiteralOperand()),
        QVariant::fromValue(QOpcUaElementOperand()),
        QVariant::fromValue(QOpcUaRelativePathElement()),
        QVariant::fromValue(QOpcUaContentFilterElement()),
        QVariant::fromValue(QOpcUaMonitoringParameters::EventFilter()),
    };

    for (const QVariant &value : values) {
        QVERIFY2(!displayValue(value).isEmpty(), value.typeName());
        QVERIFY2(!displayValue(QVariant(QVariantList{value})).isEmpty(), value.typeName());
    }
}

void TestAttributeFormatterValues::formatAttributeDispatchesPerAttribute()
{
    OpcUaNodeAttribute nodeId;
    formatAttribute(&nodeId, QOpcUa::NodeAttribute::NodeId,
                    QVariant(QStringLiteral("ns=1;i=84")), QOpcUa::Types::Undefined);
    QCOMPARE(nodeId.displayValue, QStringLiteral("ns=1;i=84"));
    QCOMPARE(nodeId.children.size(), 3);

    OpcUaNodeAttribute nodeClass;
    formatAttribute(&nodeClass, QOpcUa::NodeAttribute::NodeClass,
                    QVariant(static_cast<int>(QOpcUa::NodeClass::Variable)),
                    QOpcUa::Types::Undefined);
    QCOMPARE(nodeClass.displayValue, QStringLiteral("Variable"));

    OpcUaNodeAttribute rank;
    formatAttribute(&rank, QOpcUa::NodeAttribute::ValueRank,
                    QVariant(-1), QOpcUa::Types::Undefined);
    QCOMPARE(rank.displayValue, QStringLiteral("-1 (Scalar)"));

    OpcUaNodeAttribute access;
    formatAttribute(&access, QOpcUa::NodeAttribute::AccessLevel,
                    QVariant(static_cast<uint>(QOpcUa::AccessLevelBit::CurrentRead)),
                    QOpcUa::Types::Undefined);
    QCOMPARE(access.displayValue, QStringLiteral("CurrentRead"));

    OpcUaNodeAttribute browseName;
    QOpcUaQualifiedName qualified;
    qualified.setNamespaceIndex(2);
    qualified.setName(QStringLiteral("Temperature"));
    formatAttribute(&browseName, QOpcUa::NodeAttribute::BrowseName,
                    QVariant::fromValue(qualified), QOpcUa::Types::Undefined);
    QCOMPARE(browseName.displayValue, QStringLiteral("2, \"Temperature\""));

    OpcUaNodeAttribute displayName;
    QOpcUaLocalizedText localized;
    localized.setLocale(QStringLiteral("en"));
    localized.setText(QStringLiteral("Sensor"));
    formatAttribute(&displayName, QOpcUa::NodeAttribute::DisplayName,
                    QVariant::fromValue(localized), QOpcUa::Types::Undefined);
    QCOMPARE(displayName.displayValue, QStringLiteral("\"en\", \"Sensor\""));

    OpcUaNodeAttribute dataType;
    formatAttribute(&dataType, QOpcUa::NodeAttribute::DataType,
                    QVariant(QStringLiteral("ns=0;i=11")), QOpcUa::Types::Undefined);
    QCOMPARE(dataType.displayValue, QStringLiteral("Double"));

    OpcUaNodeAttribute dimensions;
    formatAttribute(&dimensions, QOpcUa::NodeAttribute::ArrayDimensions,
                    QVariant(QVariantList{3, 4}), QOpcUa::Types::Undefined);
    QCOMPARE(dimensions.displayValue, QStringLiteral("UInt32 Array[2]"));
    QCOMPARE(dimensions.children.size(), 2);
    QCOMPARE(dimensions.children.at(1).displayValue, QStringLiteral("4"));

    OpcUaNodeAttribute writeMask;
    formatAttribute(&writeMask, QOpcUa::NodeAttribute::WriteMask,
                    QVariant(static_cast<uint>(QOpcUa::WriteMaskBit::DisplayName)),
                    QOpcUa::Types::Undefined);
    QCOMPARE(writeMask.displayValue, QStringLiteral("DisplayName"));

    // The Value attribute is special-cased to show the type, not the data.
    OpcUaNodeAttribute scalarValue;
    formatAttribute(&scalarValue, QOpcUa::NodeAttribute::Value,
                    QVariant(5), QOpcUa::Types::Int32);
    QCOMPARE(scalarValue.displayValue, QStringLiteral("Int32"));

    OpcUaNodeAttribute arrayValue;
    formatAttribute(&arrayValue, QOpcUa::NodeAttribute::Value,
                    QVariant(QVariantList{1, 2, 3}), QOpcUa::Types::Int32);
    QCOMPARE(arrayValue.displayValue, QStringLiteral("Int32 Array[3]"));

    // A value of a standard non-builtin type is labelled with that type, not "Undefined".
    OpcUaNodeAttribute enumValue;
    formatAttribute(&enumValue, QOpcUa::NodeAttribute::Value, QVariant(0),
                    QOpcUa::Types::Undefined, QStringLiteral("ns=0;i=852"));
    QCOMPARE(enumValue.displayValue, QStringLiteral("ServerState"));
}

QTEST_GUILESS_MAIN(TestAttributeFormatterValues)

#include "test_attributeformatter_values.moc"
