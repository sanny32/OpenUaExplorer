// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_attributeformatter_attributes.cpp
/// \brief Tests formatted attributes, arrays, images, and structures.
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
class TestAttributeFormatterAttributes : public QObject
{
    Q_OBJECT

private slots:
    void childAttributeStoresFields();
    void nodeIdAttributeParsesParts();
    void dataTypeAttributeUsesBuiltInName();
    void valueAttributeScalarAndArray();
    void valueElementsSplitArraysAndHonourTheLimit();
    void valueSummaryNamesArraysAndPassesScalarsThrough();
    void imageDataTypesAreRecognisedFromTheirNodeId();
    void imageValuesSummariseInsteadOfDumpingHex();
    void imageValueAttributeCarriesItsBytes();
    void structValuesExpandIntoTheirDeclaredFields();
};

void TestAttributeFormatterAttributes::childAttributeStoresFields()
{
    const OpcUaNodeAttribute child =
        childAttribute(QStringLiteral("Name"), QStringLiteral("Value"));
    QCOMPARE(child.name, QStringLiteral("Name"));
    QCOMPARE(child.displayValue, QStringLiteral("Value"));
    QVERIFY(child.children.isEmpty());
}

void TestAttributeFormatterAttributes::nodeIdAttributeParsesParts()
{
    OpcUaNodeAttribute attribute;
    formatNodeIdAttribute(&attribute, QStringLiteral("ns=2;i=42"));
    QCOMPARE(attribute.displayValue, QStringLiteral("ns=2;i=42"));
    QCOMPARE(attribute.children.size(), 3);
    QCOMPARE(attribute.children.at(0).displayValue, QStringLiteral("2"));
    QCOMPARE(attribute.children.at(1).displayValue, QStringLiteral("Numeric"));
    QCOMPARE(attribute.children.at(2).displayValue, QStringLiteral("42"));

    // A malformed NodeId leaves the value but adds no parsed children.
    OpcUaNodeAttribute invalid;
    formatNodeIdAttribute(&invalid, QStringLiteral("not-a-node-id"));
    QCOMPARE(invalid.displayValue, QStringLiteral("not-a-node-id"));
    QVERIFY(invalid.children.isEmpty());
}

void TestAttributeFormatterAttributes::dataTypeAttributeUsesBuiltInName()
{
    OpcUaNodeAttribute builtIn;
    formatDataTypeAttribute(&builtIn, QStringLiteral("ns=0;i=6"));
    QCOMPARE(builtIn.displayValue, QStringLiteral("Int32"));
    QCOMPARE(builtIn.children.size(), 3);

    // A non-builtin DataType keeps the raw NodeId as its display value.
    OpcUaNodeAttribute custom;
    formatDataTypeAttribute(&custom, QStringLiteral("ns=3;i=5001"));
    QCOMPARE(custom.displayValue, QStringLiteral("ns=3;i=5001"));
}

void TestAttributeFormatterAttributes::valueAttributeScalarAndArray()
{
    const OpcUaNodeAttribute scalar = valueAttribute(QVariant(7), QOpcUa::Types::Int32);
    QCOMPARE(scalar.name, QStringLiteral("Value"));
    QCOMPARE(scalar.displayValue, QStringLiteral("7"));
    QVERIFY(scalar.children.isEmpty());

    const OpcUaNodeAttribute array =
        valueAttribute(QVariant(QVariantList{10, 20}), QOpcUa::Types::Int32);
    QCOMPARE(array.displayValue, QStringLiteral("Int32 Array[2]"));

    QCOMPARE(array.children.size(), 2);
    QCOMPARE(array.children.at(0).name, QStringLiteral("[0]"));
    QCOMPARE(array.children.at(1).displayValue, QStringLiteral("20"));

    const OpcUaNodeAttribute structures =
        valueAttribute(QVariant(QVariantList{1, 2}), QOpcUa::Types::Undefined,
                       QStringLiteral("ns=0;i=338"));
    QCOMPARE(structures.displayValue, QStringLiteral("BuildInfo Array[2]"));
}

void TestAttributeFormatterAttributes::valueElementsSplitArraysAndHonourTheLimit()
{
    QVERIFY(!hasValueElements(QVariant(7)));
    QVERIFY(!hasValueElements(QVariant(QVariantList{})));
    QVERIFY(hasValueElements(QVariant(QVariantList{1})));

    int total = 0;
    const QVector<ValueElement> elements =
        valueElements(QVariant(QVariantList{10, 20, QVariantList{30}}), -1, &total);
    QCOMPARE(total, 3);
    QCOMPARE(elements.size(), 3);
    QCOMPARE(elements.at(0).label, QStringLiteral("[0]"));
    QCOMPARE(elements.at(1).text, QStringLiteral("20"));
    QVERIFY(!elements.at(1).hasChildren);
    QVERIFY(elements.at(2).hasChildren);
    QCOMPARE(elements.at(2).value.toList().size(), 1);

    // The limit truncates the elements but still reports how many there were.
    const QVector<ValueElement> capped =
        valueElements(QVariant(QVariantList{1, 2, 3}), 2, &total);
    QCOMPARE(capped.size(), 2);
    QCOMPARE(total, 3);
}

void TestAttributeFormatterAttributes::valueSummaryNamesArraysAndPassesScalarsThrough()
{
    QCOMPARE(valueSummary(QVariant(7), QOpcUa::Types::Int32), QStringLiteral("7"));

    // A short array of scalars is spelled out; a longer one is named by type and size.
    QCOMPARE(valueSummary(QVariant(QVariantList{1, 2}), QOpcUa::Types::Int32),
             QStringLiteral("[1, 2]"));
    QVariantList ten;
    QVariantList eleven;
    for (int index = 0; index < 11; ++index) {
        if (index < 10)
            ten.append(index);
        eleven.append(index);
    }
    QCOMPARE(valueSummary(QVariant(ten), QOpcUa::Types::Int32),
             QStringLiteral("[0, 1, 2, 3, 4, 5, 6, 7, 8, 9]"));
    QCOMPARE(valueSummary(QVariant(eleven), QOpcUa::Types::Int32), QStringLiteral("Int32[11]"));

    // An array whose elements expand on their own keeps the short summary.
    QCOMPARE(valueSummary(QVariant(QVariantList{QVariant(QVariantList{1, 2})}),
                          QOpcUa::Types::Int32),
             QStringLiteral("Int32[1]"));
    QCOMPARE(valueSummary(QVariant(QVariantList{1}), QOpcUa::Types::Undefined,
                          QStringLiteral("ns=0;i=338")),
             QStringLiteral("[1]"));
}

void TestAttributeFormatterAttributes::imageDataTypesAreRecognisedFromTheirNodeId()
{
    QCOMPARE(imageEncodingForDataType(QStringLiteral("ns=0;i=30")), ImageEncoding::Any);
    QCOMPARE(imageEncodingForDataType(QStringLiteral("ns=0;i=2000")), ImageEncoding::Bmp);
    QCOMPARE(imageEncodingForDataType(QStringLiteral("ns=0;i=2001")), ImageEncoding::Gif);
    QCOMPARE(imageEncodingForDataType(QStringLiteral("ns=0;i=2002")), ImageEncoding::Jpeg);
    QCOMPARE(imageEncodingForDataType(QStringLiteral("ns=0;i=2003")), ImageEncoding::Png);

    // Plain ByteString, a server-defined type reusing the identifier, and no type at all.
    QCOMPARE(imageEncodingForDataType(QStringLiteral("ns=0;i=15")), ImageEncoding::None);
    QCOMPARE(imageEncodingForDataType(QStringLiteral("ns=3;i=2003")), ImageEncoding::None);
    QCOMPARE(imageEncodingForDataType(QString()), ImageEncoding::None);
}

void TestAttributeFormatterAttributes::imageValuesSummariseInsteadOfDumpingHex()
{
    const QByteArray png = encodedPng(QSize(2, 3));
    QVERIFY(!png.isEmpty());

    const std::optional<ImageValueInfo> info =
        imageValue(QVariant(png), QStringLiteral("ns=0;i=2003"));
    QVERIFY(info.has_value());
    QCOMPARE(info->formatName, QStringLiteral("PNG"));
    QCOMPARE(info->size, QSize(2, 3));
    QCOMPARE(info->data, png);

    const QString summary = valueSummary(QVariant(png), QOpcUa::Types::ByteString,
                                         QStringLiteral("ns=0;i=2003"));
    QVERIFY2(summary.startsWith(QStringLiteral("PNG 2×3, ")), qPrintable(summary));
    QVERIFY2(summary.endsWith(QStringLiteral("· 89 50 4e 47 0d 0a 1a 0a…")), qPrintable(summary));

    // The DataType names the format, so a picture stays labelled without its image plugin.
    const QByteArray gif = QByteArrayLiteral("GIF89a\x04\x00\x02\x00\xf0\x00\x00");
    const std::optional<ImageValueInfo> gifInfo =
        imageValue(QVariant(gif), QStringLiteral("ns=0;i=2001"));
    QVERIFY(gifInfo.has_value());
    QCOMPARE(gifInfo->formatName, QStringLiteral("GIF"));

    // Every other ByteString keeps its hex dump, so writing one back still parses.
    QVERIFY(!imageValue(QVariant(png), QStringLiteral("ns=0;i=15")).has_value());
    QCOMPARE(valueSummary(QVariant(QByteArrayLiteral("\x01\x02")), QOpcUa::Types::ByteString,
                          QStringLiteral("ns=0;i=15")),
             QStringLiteral("01 02"));
    QCOMPARE(displayValue(QVariant(png)), QString::fromLatin1(png.toHex(' ')));

    // An empty ByteString is nothing to show.
    QVERIFY(!imageValue(QVariant(QByteArray()), QStringLiteral("ns=0;i=2003")).has_value());
}

void TestAttributeFormatterAttributes::imageValueAttributeCarriesItsBytes()
{
    const QByteArray png = encodedPng(QSize(4, 4));
    QVERIFY(!png.isEmpty());

    const OpcUaNodeAttribute attribute =
        valueAttribute(QVariant(png), QOpcUa::Types::ByteString, QStringLiteral("ns=0;i=2003"));
    QVERIFY(attribute.isImage);
    QCOMPARE(attribute.value.toByteArray(), png);
    QVERIFY2(attribute.displayValue.startsWith(QStringLiteral("PNG 4×4, ")),
             qPrintable(attribute.displayValue));
    QVERIFY(attribute.children.isEmpty());

    // A ByteString of any other DataType is left exactly as it was.
    const OpcUaNodeAttribute plain =
        valueAttribute(QVariant(png), QOpcUa::Types::ByteString, QStringLiteral("ns=0;i=15"));
    QVERIFY(!plain.isImage);
    QVERIFY(plain.value.isNull());
    QCOMPARE(plain.displayValue, QString::fromLatin1(png.toHex(' ')));
}

void TestAttributeFormatterAttributes::structValuesExpandIntoTheirDeclaredFields()
{
    QOpcUaStructureField low;
    low.setName(QStringLiteral("Low"));
    low.setDataType(QStringLiteral("ns=0;i=11"));
    QOpcUaStructureField high;
    high.setName(QStringLiteral("High"));
    high.setDataType(QStringLiteral("ns=0;i=11"));

    QOpcUaStructureDefinition definition;
    definition.setFields({low, high});

    const QOpcUaGenericStructValue range(QStringLiteral("Range"), QStringLiteral("ns=0;i=884"),
                                         definition,
                                         {{QStringLiteral("High"), 100.0},
                                          {QStringLiteral("Low"), 0.0}});
    const QVariant value = QVariant::fromValue(range);

    QVERIFY(hasValueElements(value));
    QCOMPARE(valueSummary(value, QOpcUa::Types::ExtensionObject), QStringLiteral("Range"));
    QCOMPARE(displayValue(value), QStringLiteral("Range {Low: 0, High: 100}"));

    // The attribute tree lists the fields the way it lists array elements.
    const OpcUaNodeAttribute attribute = valueAttribute(value, QOpcUa::Types::ExtensionObject);
    QCOMPARE(attribute.displayValue, QStringLiteral("Range"));
    QCOMPARE(attribute.children.size(), 2);
    QCOMPARE(attribute.children.at(0).name, QStringLiteral("Low"));
    QCOMPARE(attribute.children.at(1).displayValue, QStringLiteral("100"));

    OpcUaNodeAttribute valueRow;
    formatAttribute(&valueRow, QOpcUa::NodeAttribute::Value, value,
                    QOpcUa::Types::ExtensionObject, QStringLiteral("ns=0;i=22"));
    QCOMPARE(valueRow.displayValue, QStringLiteral("Range"));

    // The fields arrive in a hash, so the definition decides the order they are listed in.
    const QVector<ValueElement> fields = valueElements(value);
    QCOMPARE(fields.size(), 2);
    QCOMPARE(fields.at(0).label, QStringLiteral("Low"));
    QCOMPARE(fields.at(0).text, QStringLiteral("0"));
    QCOMPARE(fields.at(0).typeName, QStringLiteral("Double"));
    QCOMPARE(fields.at(1).label, QStringLiteral("High"));
    QVERIFY(!fields.at(1).hasChildren);
}

QTEST_GUILESS_MAIN(TestAttributeFormatterAttributes)

#include "test_attributeformatter_attributes.moc"
