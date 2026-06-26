// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_attributeformatter.cpp
/// \brief Unit tests for the pure OPC UA value/attribute formatting helpers.
///

#include <QDateTime>
#include <QTest>
#include <QVariant>

#include <QOpcUaLocalizedText>
#include <QOpcUaQualifiedName>

#include "opcua/attributeformatter.h"

using namespace OpcUaFormat;

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
    void isoTimestampWithZoneRoundTrips();
    void valueTypeNameKnownAndUnknown();
    void dataTypeDisplayNamesBuiltIns();
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
    void formatAttributeDispatchesPerAttribute();
    void attributeAppliesToNodeClassMatrix();
    void valueTypeForDataTypeMapping();
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

void TestAttributeFormatter::isoTimestampWithZoneRoundTrips()
{
    QCOMPARE(isoTimestampWithZone(QDateTime()), QString());
    QCOMPARE(isoTimestampWithZone(QDateTime(), TimestampMode::Utc), QString());

    const QDateTime dt(QDate(2024, 12, 31), QTime(23, 59, 58, 123), Qt::UTC);
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

QTEST_GUILESS_MAIN(TestAttributeFormatter)

#include "test_attributeformatter.moc"
