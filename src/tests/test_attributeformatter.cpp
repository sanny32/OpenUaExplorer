// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_attributeformatter.cpp
/// \brief Unit tests for the pure OPC UA value/attribute formatting helpers.
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
/// \brief Unit tests for OpcUaFormat, which needs no live connection.
///
class TestAttributeFormatter : public QObject
{
    Q_OBJECT

private slots:
    void isValueArrayClassifiesValues();
    void displayValueFormatsScalarsAndArrays();
    void securityModeNameCoversAllModes();
    void statusFormatting();
    void statusSeverityClassifiesQuality();
    void isoTimestampWithZoneRoundTrips();
    void valueTypeNameKnownAndUnknown();
    void dataTypeDisplayNamesBuiltIns();
    void dataTypeDisplayNamesStandardEnumsAndStructures();
    void valueTypeDisplayFallsBackToDataType();
    void standardNodeDisplayNameNamesKnownNodes();
    void nodeClassNameKnownAndUnknown();
    void accessLevelDisplayDecodesFlags();
    void writeMaskDisplayDecodesFlags();
    void eventNotifierDisplayDecodesFlags();
    void valueRankDisplayKnownAndNumeric();
    void identifierTypeNameKnownAndUnknown();
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
    void builtinTypesFormatWithRulesOfTheirOwn();
    void undecodedExtensionObjectsNameTheirEncoding();
    void everyTransportedTypeRendersAsText();
    void formatAttributeDispatchesPerAttribute();
    void attributeAppliesToNodeClassMatrix();
    void valueTypeForDataTypeMapping();
    void scalarFromTextConvertsSupportedTypes();
    void scalarFromTextRangeChecksIntegers();
    void scalarFromTextRejectsMalformedInput();
};

void TestAttributeFormatter::isValueArrayClassifiesValues()
{
    QVERIFY(!isValueArray(QVariant(QStringLiteral("text"))));
    QVERIFY(!isValueArray(QVariant(QByteArray("\x01\x02"))));
    QVERIFY(!isValueArray(QVariant(42)));
    QVERIFY(isValueArray(QVariant(QVariantList{1, 2, 3})));
}

void TestAttributeFormatter::displayValueFormatsScalarsAndArrays()
{
    QCOMPARE(displayValue(QVariant()), QString());
    QCOMPARE(displayValue(QVariant(42)), QStringLiteral("42"));
    QCOMPARE(displayValue(QVariant(QStringLiteral("abc"))), QStringLiteral("abc"));
    QCOMPARE(displayValue(QVariant(QByteArray("\x01\xAB"))), QStringLiteral("01 ab"));
    QCOMPARE(displayValue(QVariant::fromValue<quint8>(255)), QStringLiteral("255"));
    QCOMPARE(displayValue(QVariant::fromValue<qint8>(-7)), QStringLiteral("-7"));

    const QDateTime dt(QDate(2024, 1, 2), QTime(3, 4, 5, 6));
    QCOMPARE(displayValue(QVariant(dt)), dt.toString(Qt::ISODateWithMs));

    QCOMPARE(displayValue(QVariant(QVariantList{1, 2, 3})),
             QStringLiteral("[1, 2, 3]"));
}

void TestAttributeFormatter::securityModeNameCoversAllModes()
{
    QCOMPARE(securityModeName(QOpcUaEndpointDescription::None), QStringLiteral("None"));
    QCOMPARE(securityModeName(QOpcUaEndpointDescription::Sign), QStringLiteral("Sign"));
    QCOMPARE(securityModeName(QOpcUaEndpointDescription::SignAndEncrypt),
             QStringLiteral("Sign & Encrypt"));
    QCOMPARE(securityModeName(QOpcUaEndpointDescription::Invalid),
             QStringLiteral("Invalid"));
}

void TestAttributeFormatter::statusFormatting()
{
    QCOMPARE(statusName(QOpcUa::UaStatusCode::Good),
             QOpcUa::statusToString(QOpcUa::UaStatusCode::Good));
    QCOMPARE(statusDisplay(QOpcUa::UaStatusCode::Good),
             QStringLiteral("%1 (0x00000000)")
                 .arg(statusName(QOpcUa::UaStatusCode::Good)));
}

void TestAttributeFormatter::statusSeverityClassifiesQuality()
{
    QCOMPARE(statusSeverity(statusName(QOpcUa::UaStatusCode::Good)), StatusSeverity::Good);
    QCOMPARE(statusSeverity(QStringLiteral("GoodClamped")), StatusSeverity::Good);
    QCOMPARE(statusSeverity(QStringLiteral("UncertainLastUsableValue")),
             StatusSeverity::Uncertain);
    QCOMPARE(statusSeverity(statusName(QOpcUa::UaStatusCode::BadNodeIdUnknown)),
             StatusSeverity::Bad);
    QCOMPARE(statusSeverity(QString()), StatusSeverity::Unknown);
    QCOMPARE(statusSeverity(QStringLiteral("Pending…")), StatusSeverity::Unknown);
}

void TestAttributeFormatter::isoTimestampWithZoneRoundTrips()
{
    QCOMPARE(isoTimestampWithZone(QDateTime()), QString());
    QCOMPARE(isoTimestampWithZone(QDateTime(), TimestampMode::Utc), QString());

    const QDateTime dt(QDate(2024, 12, 31), QTime(23, 59, 58, 123), QTimeZone::UTC);
    const QDateTime local = dt.toLocalTime();
    const QString localExpected = local.toOffsetFromUtc(local.offsetFromUtc())
                                      .toString(Qt::ISODateWithMs)
                                      .replace(QLatin1Char('T'), QLatin1Char(' '));
    QCOMPARE(isoTimestampWithZone(dt), localExpected);
    QCOMPARE(isoTimestampWithZone(dt, TimestampMode::LocalTime), localExpected);

    const QString utc = isoTimestampWithZone(dt, TimestampMode::Utc);
    QCOMPARE(utc, dt.toUTC().toString(Qt::ISODateWithMs).replace(QLatin1Char('T'), QLatin1Char(' ')));
    QVERIFY(utc.endsWith(QLatin1Char('Z')));
    QVERIFY(!utc.contains(QLatin1Char('T')));
}

void TestAttributeFormatter::valueTypeNameKnownAndUnknown()
{
    QCOMPARE(valueTypeName(QOpcUa::Types::Int32), QStringLiteral("Int32"));
    QCOMPARE(valueTypeName(QOpcUa::Types::Double), QStringLiteral("Double"));
    // An out-of-range value has no enum key and falls back to "Unknown".
    QCOMPARE(valueTypeName(static_cast<QOpcUa::Types>(9999)), QStringLiteral("Unknown"));
}

void TestAttributeFormatter::dataTypeDisplayNamesBuiltIns()
{
    QCOMPARE(dataTypeDisplay(QStringLiteral("ns=0;i=11")), QStringLiteral("Double"));
    QCOMPARE(dataTypeDisplay(QStringLiteral("ns=3;i=5001")), QStringLiteral("ns=3;i=5001"));
    // A DataType is named by its BrowseName, not by the variant type its values travel in.
    QCOMPARE(dataTypeDisplay(QStringLiteral("ns=0;i=22")), QStringLiteral("Structure"));
    QCOMPARE(valueTypeDisplay(QOpcUa::Types::ExtensionObject, QStringLiteral("ns=0;i=22")),
             QStringLiteral("Structure"));
    QCOMPARE(valueTypeDisplay(QOpcUa::Types::ExtensionObject, QString()),
             QStringLiteral("ExtensionObject"));
}

void TestAttributeFormatter::dataTypeDisplayNamesStandardEnumsAndStructures()
{
    // Standard types without a built-in value type are named by their BrowseName.
    QCOMPARE(dataTypeDisplay(QStringLiteral("ns=0;i=852")), QStringLiteral("ServerState"));
    QCOMPARE(dataTypeDisplay(QStringLiteral("ns=0;i=338")), QStringLiteral("BuildInfo"));
    QCOMPARE(dataTypeDisplay(QStringLiteral("i=884")), QStringLiteral("Range"));
    // Server-defined types keep the NodeId, since their name lives on the server.
    QCOMPARE(dataTypeDisplay(QStringLiteral("ns=2;i=3002")), QStringLiteral("ns=2;i=3002"));
}

void TestAttributeFormatter::valueTypeDisplayFallsBackToDataType()
{
    QCOMPARE(valueTypeDisplay(QOpcUa::Types::Int32, QStringLiteral("ns=0;i=6")),
             QStringLiteral("Int32"));
    QCOMPARE(valueTypeDisplay(QOpcUa::Types::Undefined, QStringLiteral("ns=0;i=852")),
             QStringLiteral("ServerState"));
    QCOMPARE(valueTypeDisplay(QOpcUa::Types::Undefined, QString()),
             QStringLiteral("Undefined"));
}

void TestAttributeFormatter::standardNodeDisplayNameNamesKnownNodes()
{
    QCOMPARE(standardNodeDisplayName(QStringLiteral("ns=0;i=9482")),
             QStringLiteral("ExclusiveLevelAlarmType"));
    QCOMPARE(standardNodeDisplayName(QStringLiteral("i=2041")),
             QStringLiteral("BaseEventType"));
    QCOMPARE(standardNodeDisplayName(QStringLiteral("ns=0;i=11")),
             QStringLiteral("Double"));
    QCOMPARE(standardNodeDisplayName(QStringLiteral("ns=2;s=CustomAlarmType")),
             QStringLiteral("ns=2;s=CustomAlarmType"));
}

void TestAttributeFormatter::nodeClassNameKnownAndUnknown()
{
    QCOMPARE(nodeClassName(QOpcUa::NodeClass::Variable), QStringLiteral("Variable"));
    QCOMPARE(nodeClassName(QOpcUa::NodeClass::Object), QStringLiteral("Object"));
    // No enum key -> numeric fallback.
    QCOMPARE(nodeClassName(static_cast<QOpcUa::NodeClass>(123)), QStringLiteral("123"));
}

void TestAttributeFormatter::accessLevelDisplayDecodesFlags()
{
    QCOMPARE(accessLevelDisplay(0), QStringLiteral("None"));
    QCOMPARE(accessLevelDisplay(static_cast<quint32>(QOpcUa::AccessLevelBit::CurrentRead)),
             QStringLiteral("CurrentRead"));
    const quint32 readWrite =
        static_cast<quint32>(QOpcUa::AccessLevelBit::CurrentRead)
        | static_cast<quint32>(QOpcUa::AccessLevelBit::CurrentWrite);
    QCOMPARE(accessLevelDisplay(readWrite),
             QStringLiteral("CurrentRead | CurrentWrite"));
}

void TestAttributeFormatter::writeMaskDisplayDecodesFlags()
{
    QCOMPARE(writeMaskDisplay(0), QStringLiteral("0"));
    const quint32 mask =
        static_cast<quint32>(QOpcUa::WriteMaskBit::DisplayName);
    QCOMPARE(writeMaskDisplay(mask), QStringLiteral("DisplayName"));
}

void TestAttributeFormatter::eventNotifierDisplayDecodesFlags()
{
    QCOMPARE(eventNotifierDisplay(0), QStringLiteral("None"));
    QCOMPARE(eventNotifierDisplay(
                 static_cast<quint8>(QOpcUa::EventNotifierBit::SubscribeToEvents)),
             QStringLiteral("SubscribeToEvents"));
    const quint8 historyReadWrite =
        static_cast<quint8>(QOpcUa::EventNotifierBit::HistoryRead)
        | static_cast<quint8>(QOpcUa::EventNotifierBit::HistoryWrite);
    QCOMPARE(eventNotifierDisplay(historyReadWrite),
             QStringLiteral("HistoryRead | HistoryWrite"));
}

void TestAttributeFormatter::valueRankDisplayKnownAndNumeric()
{
    QCOMPARE(valueRankDisplay(-3), QStringLiteral("-3 (ScalarOrOneDimension)"));
    QCOMPARE(valueRankDisplay(-2), QStringLiteral("-2 (Any)"));
    QCOMPARE(valueRankDisplay(-1), QStringLiteral("-1 (Scalar)"));
    QCOMPARE(valueRankDisplay(0), QStringLiteral("0 (OneOrMoreDimensions)"));
    QCOMPARE(valueRankDisplay(1), QStringLiteral("1 (OneDimension)"));
    QCOMPARE(valueRankDisplay(2), QStringLiteral("2 (TwoDimensions)"));
    QCOMPARE(valueRankDisplay(7), QStringLiteral("7"));
}

void TestAttributeFormatter::identifierTypeNameKnownAndUnknown()
{
    QCOMPARE(identifierTypeName('i'), QStringLiteral("Numeric"));
    QCOMPARE(identifierTypeName('s'), QStringLiteral("String"));
    QCOMPARE(identifierTypeName('g'), QStringLiteral("Guid"));
    QCOMPARE(identifierTypeName('b'), QStringLiteral("ByteString"));
    QCOMPARE(identifierTypeName('z'), QStringLiteral("Unknown"));
}

void TestAttributeFormatter::childAttributeStoresFields()
{
    const OpcUaNodeAttribute child =
        childAttribute(QStringLiteral("Name"), QStringLiteral("Value"));
    QCOMPARE(child.name, QStringLiteral("Name"));
    QCOMPARE(child.displayValue, QStringLiteral("Value"));
    QVERIFY(child.children.isEmpty());
}

void TestAttributeFormatter::nodeIdAttributeParsesParts()
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

void TestAttributeFormatter::dataTypeAttributeUsesBuiltInName()
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

void TestAttributeFormatter::valueAttributeScalarAndArray()
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

void TestAttributeFormatter::valueElementsSplitArraysAndHonourTheLimit()
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

void TestAttributeFormatter::valueSummaryNamesArraysAndPassesScalarsThrough()
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

void TestAttributeFormatter::imageDataTypesAreRecognisedFromTheirNodeId()
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

void TestAttributeFormatter::imageValuesSummariseInsteadOfDumpingHex()
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

void TestAttributeFormatter::imageValueAttributeCarriesItsBytes()
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

void TestAttributeFormatter::structValuesExpandIntoTheirDeclaredFields()
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

void TestAttributeFormatter::builtinTypesFormatWithRulesOfTheirOwn()
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

void TestAttributeFormatter::undecodedExtensionObjectsNameTheirEncoding()
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

void TestAttributeFormatter::everyTransportedTypeRendersAsText()
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

void TestAttributeFormatter::formatAttributeDispatchesPerAttribute()
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

void TestAttributeFormatter::attributeAppliesToNodeClassMatrix()
{
    QVERIFY(attributeAppliesToNodeClass(QOpcUa::NodeAttribute::Value,
                                        QOpcUa::NodeClass::Variable));
    QVERIFY(!attributeAppliesToNodeClass(QOpcUa::NodeAttribute::Value,
                                         QOpcUa::NodeClass::Object));
    QVERIFY(attributeAppliesToNodeClass(QOpcUa::NodeAttribute::Executable,
                                        QOpcUa::NodeClass::Method));
    QVERIFY(!attributeAppliesToNodeClass(QOpcUa::NodeAttribute::Executable,
                                         QOpcUa::NodeClass::Variable));
    QVERIFY(attributeAppliesToNodeClass(QOpcUa::NodeAttribute::EventNotifier,
                                        QOpcUa::NodeClass::View));
    QVERIFY(attributeAppliesToNodeClass(QOpcUa::NodeAttribute::AccessLevel,
                                        QOpcUa::NodeClass::Variable));
    QVERIFY(attributeAppliesToNodeClass(QOpcUa::NodeAttribute::IsAbstract,
                                        QOpcUa::NodeClass::ObjectType));
    QVERIFY(attributeAppliesToNodeClass(QOpcUa::NodeAttribute::Symmetric,
                                        QOpcUa::NodeClass::ReferenceType));
    QVERIFY(attributeAppliesToNodeClass(QOpcUa::NodeAttribute::ContainsNoLoops,
                                        QOpcUa::NodeClass::View));
    // Attributes common to every node class fall through to the default branch.
    QVERIFY(attributeAppliesToNodeClass(QOpcUa::NodeAttribute::BrowseName,
                                        QOpcUa::NodeClass::Object));
}

void TestAttributeFormatter::valueTypeForDataTypeMapping()
{
    QVector<QPair<int, QOpcUa::Types>> mapping = {
        {1, QOpcUa::Types::Boolean},      {2, QOpcUa::Types::SByte},
        {3, QOpcUa::Types::Byte},         {4, QOpcUa::Types::Int16},
        {5, QOpcUa::Types::UInt16},       {6, QOpcUa::Types::Int32},
        {7, QOpcUa::Types::UInt32},       {8, QOpcUa::Types::Int64},
        {9, QOpcUa::Types::UInt64},       {10, QOpcUa::Types::Float},
        {11, QOpcUa::Types::Double},      {12, QOpcUa::Types::String},
        {13, QOpcUa::Types::DateTime},    {14, QOpcUa::Types::Guid},
        {15, QOpcUa::Types::ByteString},  {16, QOpcUa::Types::XmlElement},
        {17, QOpcUa::Types::NodeId},      {18, QOpcUa::Types::ExpandedNodeId},
        {19, QOpcUa::Types::StatusCode},  {20, QOpcUa::Types::QualifiedName},
        {21, QOpcUa::Types::LocalizedText}, {22, QOpcUa::Types::ExtensionObject}
    };
    mapping.append({25, QOpcUa::Types::DiagnosticInfo});
    for (const auto &entry : mapping) {
        QCOMPARE(valueTypeForDataType(QStringLiteral("ns=0;i=%1").arg(entry.first)),
                 entry.second);
    }

    // Unknown identifier, or a non-zero namespace, maps to Undefined.
    QCOMPARE(valueTypeForDataType(QStringLiteral("ns=0;i=9999")), QOpcUa::Types::Undefined);
    QCOMPARE(valueTypeForDataType(QStringLiteral("ns=2;i=6")), QOpcUa::Types::Undefined);
}

void TestAttributeFormatter::scalarFromTextConvertsSupportedTypes()
{
    bool ok = false;

    QCOMPARE(scalarFromText(QStringLiteral("true"), QOpcUa::Types::Boolean, &ok), QVariant(true));
    QVERIFY(ok);
    QCOMPARE(scalarFromText(QStringLiteral("TRUE"), QOpcUa::Types::Boolean, &ok), QVariant(true));
    QVERIFY(ok);
    QCOMPARE(scalarFromText(QStringLiteral("1"), QOpcUa::Types::Boolean, &ok), QVariant(true));
    QVERIFY(ok);
    QCOMPARE(scalarFromText(QStringLiteral("false"), QOpcUa::Types::Boolean, &ok), QVariant(false));
    QVERIFY(ok);
    QCOMPARE(scalarFromText(QStringLiteral("0"), QOpcUa::Types::Boolean, &ok), QVariant(false));
    QVERIFY(ok);

    QCOMPARE(scalarFromText(QStringLiteral("-5"), QOpcUa::Types::SByte, &ok).value<qint8>(), qint8(-5));
    QVERIFY(ok);
    QCOMPARE(scalarFromText(QStringLiteral("200"), QOpcUa::Types::Byte, &ok).value<quint8>(), quint8(200));
    QVERIFY(ok);
    QCOMPARE(scalarFromText(QStringLiteral("-1000"), QOpcUa::Types::Int16, &ok).value<qint16>(), qint16(-1000));
    QVERIFY(ok);
    QCOMPARE(scalarFromText(QStringLiteral("60000"), QOpcUa::Types::UInt16, &ok).value<quint16>(), quint16(60000));
    QVERIFY(ok);
    QCOMPARE(scalarFromText(QStringLiteral("-2147483648"), QOpcUa::Types::Int32, &ok).toInt(), -2147483647 - 1);
    QVERIFY(ok);
    QCOMPARE(scalarFromText(QStringLiteral("42"), QOpcUa::Types::UInt32, &ok).toUInt(), 42u);
    QVERIFY(ok);
    QCOMPARE(scalarFromText(QStringLiteral("7"), QOpcUa::Types::StatusCode, &ok).toUInt(), 7u);
    QVERIFY(ok);
    QCOMPARE(scalarFromText(QStringLiteral("-5000000000"), QOpcUa::Types::Int64, &ok).toLongLong(),
             Q_INT64_C(-5000000000));
    QVERIFY(ok);
    QCOMPARE(scalarFromText(QStringLiteral("5000000000"), QOpcUa::Types::UInt64, &ok).toULongLong(),
             Q_UINT64_C(5000000000));
    QVERIFY(ok);
    QCOMPARE(scalarFromText(QStringLiteral("1.5"), QOpcUa::Types::Float, &ok).toFloat(), 1.5f);
    QVERIFY(ok);
    QCOMPARE(scalarFromText(QStringLiteral("2.25"), QOpcUa::Types::Double, &ok).toDouble(), 2.25);
    QVERIFY(ok);

    const QDateTime when(QDate(2024, 1, 2), QTime(3, 4, 5, 6), QTimeZone::UTC);
    QCOMPARE(scalarFromText(when.toString(Qt::ISODateWithMs), QOpcUa::Types::DateTime, &ok).toDateTime(),
             when);
    QVERIFY(ok);

    const QUuid uuid = QUuid::createUuid();
    QCOMPARE(scalarFromText(uuid.toString(), QOpcUa::Types::Guid, &ok).toUuid(), uuid);
    QVERIFY(ok);

    // Bytes go in as the hex displayValue() shows, spacing optional.
    const QByteArray bytes("\x8a\x39\x32", 3);
    QCOMPARE(scalarFromText(displayValue(bytes), QOpcUa::Types::ByteString, &ok).toByteArray(),
             bytes);
    QVERIFY(ok);
    QCOMPARE(scalarFromText(QStringLiteral("8A3932"), QOpcUa::Types::ByteString, &ok).toByteArray(),
             bytes);
    QVERIFY(ok);
    QVERIFY(scalarFromText(QString(), QOpcUa::Types::ByteString, &ok).toByteArray().isEmpty());
    QVERIFY(ok);

    scalarFromText(QStringLiteral("AQID"), QOpcUa::Types::ByteString, &ok);
    QVERIFY(!ok);
    scalarFromText(QStringLiteral("8a3"), QOpcUa::Types::ByteString, &ok);
    QVERIFY(!ok);

    QCOMPARE(scalarFromText(QStringLiteral("hello"), QOpcUa::Types::String, &ok).toString(),
             QStringLiteral("hello"));
    QVERIFY(ok);
    QCOMPARE(scalarFromText(QStringLiteral("ns=1;s=x"), QOpcUa::Types::NodeId, &ok).toString(),
             QStringLiteral("ns=1;s=x"));
    QVERIFY(ok);
}

void TestAttributeFormatter::scalarFromTextRangeChecksIntegers()
{
    bool ok = false;

    scalarFromText(QStringLiteral("128"), QOpcUa::Types::SByte, &ok);
    QVERIFY(!ok);
    scalarFromText(QStringLiteral("-129"), QOpcUa::Types::SByte, &ok);
    QVERIFY(!ok);
    scalarFromText(QStringLiteral("256"), QOpcUa::Types::Byte, &ok);
    QVERIFY(!ok);
    scalarFromText(QStringLiteral("32768"), QOpcUa::Types::Int16, &ok);
    QVERIFY(!ok);
    scalarFromText(QStringLiteral("-32769"), QOpcUa::Types::Int16, &ok);
    QVERIFY(!ok);
    scalarFromText(QStringLiteral("65536"), QOpcUa::Types::UInt16, &ok);
    QVERIFY(!ok);
}

void TestAttributeFormatter::scalarFromTextRejectsMalformedInput()
{
    bool ok = false;

    scalarFromText(QStringLiteral("maybe"), QOpcUa::Types::Boolean, &ok);
    QVERIFY(!ok);
    scalarFromText(QStringLiteral("not-a-number"), QOpcUa::Types::Int32, &ok);
    QVERIFY(!ok);
    scalarFromText(QStringLiteral("not-a-date"), QOpcUa::Types::DateTime, &ok);
    QVERIFY(!ok);
    scalarFromText(QStringLiteral("not-a-guid"), QOpcUa::Types::Guid, &ok);
    QVERIFY(!ok);

    const QVariant unsupported = scalarFromText(QStringLiteral("x"), QOpcUa::Types::ExtensionObject, &ok);
    QVERIFY(!ok);
    QVERIFY(!unsupported.isValid());
}

QTEST_GUILESS_MAIN(TestAttributeFormatter)

#include "test_attributeformatter.moc"
