// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_attributeformatter_input.cpp
/// \brief Tests attribute applicability, type mapping, and text conversion.
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
class TestAttributeFormatterInput : public QObject
{
    Q_OBJECT

private slots:
    void attributeAppliesToNodeClassMatrix();
    void valueTypeForDataTypeMapping();
    void scalarFromTextConvertsSupportedTypes();
    void scalarFromTextRangeChecksIntegers();
    void scalarFromTextRejectsMalformedInput();
};

void TestAttributeFormatterInput::attributeAppliesToNodeClassMatrix()
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

void TestAttributeFormatterInput::valueTypeForDataTypeMapping()
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

void TestAttributeFormatterInput::scalarFromTextConvertsSupportedTypes()
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

void TestAttributeFormatterInput::scalarFromTextRangeChecksIntegers()
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

void TestAttributeFormatterInput::scalarFromTextRejectsMalformedInput()
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

QTEST_GUILESS_MAIN(TestAttributeFormatterInput)

#include "test_attributeformatter_input.moc"
