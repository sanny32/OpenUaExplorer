// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_attributeformatter_names.cpp
/// \brief Tests OPC UA names, statuses, and scalar classification.
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
class TestAttributeFormatterNames : public QObject
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
};

void TestAttributeFormatterNames::isValueArrayClassifiesValues()
{
    QVERIFY(!isValueArray(QVariant(QStringLiteral("text"))));
    QVERIFY(!isValueArray(QVariant(QByteArray("\x01\x02"))));
    QVERIFY(!isValueArray(QVariant(42)));
    QVERIFY(isValueArray(QVariant(QVariantList{1, 2, 3})));
}

void TestAttributeFormatterNames::displayValueFormatsScalarsAndArrays()
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

void TestAttributeFormatterNames::securityModeNameCoversAllModes()
{
    QCOMPARE(securityModeName(QOpcUaEndpointDescription::None), QStringLiteral("None"));
    QCOMPARE(securityModeName(QOpcUaEndpointDescription::Sign), QStringLiteral("Sign"));
    QCOMPARE(securityModeName(QOpcUaEndpointDescription::SignAndEncrypt),
             QStringLiteral("Sign & Encrypt"));
    QCOMPARE(securityModeName(QOpcUaEndpointDescription::Invalid),
             QStringLiteral("Invalid"));
}

void TestAttributeFormatterNames::statusFormatting()
{
    QCOMPARE(statusName(QOpcUa::UaStatusCode::Good),
             QOpcUa::statusToString(QOpcUa::UaStatusCode::Good));
    QCOMPARE(statusDisplay(QOpcUa::UaStatusCode::Good),
             QStringLiteral("%1 (0x00000000)")
                 .arg(statusName(QOpcUa::UaStatusCode::Good)));
}

void TestAttributeFormatterNames::statusSeverityClassifiesQuality()
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

void TestAttributeFormatterNames::isoTimestampWithZoneRoundTrips()
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

void TestAttributeFormatterNames::valueTypeNameKnownAndUnknown()
{
    QCOMPARE(valueTypeName(QOpcUa::Types::Int32), QStringLiteral("Int32"));
    QCOMPARE(valueTypeName(QOpcUa::Types::Double), QStringLiteral("Double"));
    // An out-of-range value has no enum key and falls back to "Unknown".
    QCOMPARE(valueTypeName(static_cast<QOpcUa::Types>(9999)), QStringLiteral("Unknown"));
}

void TestAttributeFormatterNames::dataTypeDisplayNamesBuiltIns()
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

void TestAttributeFormatterNames::dataTypeDisplayNamesStandardEnumsAndStructures()
{
    // Standard types without a built-in value type are named by their BrowseName.
    QCOMPARE(dataTypeDisplay(QStringLiteral("ns=0;i=852")), QStringLiteral("ServerState"));
    QCOMPARE(dataTypeDisplay(QStringLiteral("ns=0;i=338")), QStringLiteral("BuildInfo"));
    QCOMPARE(dataTypeDisplay(QStringLiteral("i=884")), QStringLiteral("Range"));
    // Server-defined types keep the NodeId, since their name lives on the server.
    QCOMPARE(dataTypeDisplay(QStringLiteral("ns=2;i=3002")), QStringLiteral("ns=2;i=3002"));
}

void TestAttributeFormatterNames::valueTypeDisplayFallsBackToDataType()
{
    QCOMPARE(valueTypeDisplay(QOpcUa::Types::Int32, QStringLiteral("ns=0;i=6")),
             QStringLiteral("Int32"));
    QCOMPARE(valueTypeDisplay(QOpcUa::Types::Undefined, QStringLiteral("ns=0;i=852")),
             QStringLiteral("ServerState"));
    QCOMPARE(valueTypeDisplay(QOpcUa::Types::Undefined, QString()),
             QStringLiteral("Undefined"));
}

void TestAttributeFormatterNames::standardNodeDisplayNameNamesKnownNodes()
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

void TestAttributeFormatterNames::nodeClassNameKnownAndUnknown()
{
    QCOMPARE(nodeClassName(QOpcUa::NodeClass::Variable), QStringLiteral("Variable"));
    QCOMPARE(nodeClassName(QOpcUa::NodeClass::Object), QStringLiteral("Object"));
    // No enum key -> numeric fallback.
    QCOMPARE(nodeClassName(static_cast<QOpcUa::NodeClass>(123)), QStringLiteral("123"));
}

void TestAttributeFormatterNames::accessLevelDisplayDecodesFlags()
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

void TestAttributeFormatterNames::writeMaskDisplayDecodesFlags()
{
    QCOMPARE(writeMaskDisplay(0), QStringLiteral("0"));
    const quint32 mask =
        static_cast<quint32>(QOpcUa::WriteMaskBit::DisplayName);
    QCOMPARE(writeMaskDisplay(mask), QStringLiteral("DisplayName"));
}

void TestAttributeFormatterNames::eventNotifierDisplayDecodesFlags()
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

void TestAttributeFormatterNames::valueRankDisplayKnownAndNumeric()
{
    QCOMPARE(valueRankDisplay(-3), QStringLiteral("-3 (ScalarOrOneDimension)"));
    QCOMPARE(valueRankDisplay(-2), QStringLiteral("-2 (Any)"));
    QCOMPARE(valueRankDisplay(-1), QStringLiteral("-1 (Scalar)"));
    QCOMPARE(valueRankDisplay(0), QStringLiteral("0 (OneOrMoreDimensions)"));
    QCOMPARE(valueRankDisplay(1), QStringLiteral("1 (OneDimension)"));
    QCOMPARE(valueRankDisplay(2), QStringLiteral("2 (TwoDimensions)"));
    QCOMPARE(valueRankDisplay(7), QStringLiteral("7"));
}

void TestAttributeFormatterNames::identifierTypeNameKnownAndUnknown()
{
    QCOMPARE(identifierTypeName('i'), QStringLiteral("Numeric"));
    QCOMPARE(identifierTypeName('s'), QStringLiteral("String"));
    QCOMPARE(identifierTypeName('g'), QStringLiteral("Guid"));
    QCOMPARE(identifierTypeName('b'), QStringLiteral("ByteString"));
    QCOMPARE(identifierTypeName('z'), QStringLiteral("Unknown"));
}

QTEST_GUILESS_MAIN(TestAttributeFormatterNames)

#include "test_attributeformatter_names.moc"
