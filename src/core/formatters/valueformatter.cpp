// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file valueformatter.cpp
/// \brief Implements OPC UA value and timestamp formatting helpers.
///

#include "attributeformatter.h"

#include <QDateTime>
#include <QMetaEnum>
#include <QObject>
#include <QOpcUaGenericStructValue>
#include <QOpcUaStructureDefinition>
#include <QOpcUaStructureField>

namespace OpcUaFormat {

namespace {

///
/// \brief Formats a UTC offset as an ISO 8601 zone suffix ("Z" at zero, otherwise "+/-HH:mm").
/// \param offsetSeconds Offset from UTC in seconds.
/// \return Trailing zone indicator matching Qt::ISODate.
///
///
/// \brief Reports whether a value is a structure decoded from an ExtensionObject.
/// \param value Variant to inspect.
/// \return True when the value carries named fields.
///
bool isStructValue(const QVariant &value)
{
    return value.userType() == qMetaTypeId<QOpcUaGenericStructValue>();
}

///
/// \brief Lists a structure's fields in the order its type defines them.
/// \param value Decoded structure.
/// \return Field names, definition order first and any undeclared field appended.
///
/// The decoded fields arrive in a hash, so the order has to come from the type definition;
/// a field the definition does not mention would otherwise be dropped.
///
QStringList structFieldNames(const QOpcUaGenericStructValue &value)
{
    QStringList names;
    const QHash<QString, QVariant> fields = value.fields();
    const QList<QOpcUaStructureField> declared = value.structureDefinition().fields();
    names.reserve(fields.size());
    for (const QOpcUaStructureField &field : declared) {
        if (fields.contains(field.name()))
            names.append(field.name());
    }
    for (auto it = fields.cbegin(); it != fields.cend(); ++it) {
        if (!names.contains(it.key()))
            names.append(it.key());
    }
    return names;
}

///
/// \brief Names the type of one structure field.
/// \param structValue Structure owning the field.
/// \param name Field name.
/// \param field Field value, used to size arrays and to name undeclared fields.
/// \return Type name, with the element count appended for an array field.
///
QString fieldTypeName(const QOpcUaGenericStructValue &structValue, const QString &name,
                      const QVariant &field)
{
    QString typeName;
    for (const QOpcUaStructureField &declared : structValue.structureDefinition().fields()) {
        if (declared.name() == name) {
            typeName = dataTypeDisplay(declared.dataType());
            break;
        }
    }
    if (typeName.isEmpty() && field.userType() == qMetaTypeId<QOpcUaGenericStructValue>())
        typeName = field.value<QOpcUaGenericStructValue>().typeName();
    if (typeName.isEmpty())
        typeName = QString::fromLatin1(field.metaType().name());

    if (isValueArray(field))
        return QStringLiteral("%1[%2]").arg(typeName).arg(field.toList().size());
    return typeName;
}

QString zoneSuffix(int offsetSeconds)
{
    if (offsetSeconds == 0)
        return QStringLiteral("Z");
    const int minutes = qAbs(offsetSeconds) / 60;
    return QStringLiteral("%1%2:%3")
        .arg(offsetSeconds < 0 ? QLatin1Char('-') : QLatin1Char('+'))
        .arg(minutes / 60, 2, 10, QLatin1Char('0'))
        .arg(minutes % 60, 2, 10, QLatin1Char('0'));
}

} // namespace

///
/// \brief Reports whether a value is an array, treating strings and byte arrays as scalars.
/// \param value Variant to inspect.
/// \return True when the value should be rendered as a list.
///
bool isValueArray(const QVariant &value)
{
    return value.userType() != QMetaType::QString
        && value.userType() != QMetaType::QByteArray
        && value.canConvert<QVariantList>();
}

///
/// \brief Renders a value for display, recursing into arrays and hex-encoding byte strings.
/// \param value Variant to format.
/// \return Human-readable representation.
///
QString displayValue(const QVariant &value)
{
    if (!value.isValid())
        return QString();
    if (value.userType() == QMetaType::QByteArray)
        return QString::fromLatin1(value.toByteArray().toHex(' '));
    if (value.userType() == QMetaType::UChar)
        return QString::number(value.toUInt());
    if (value.userType() == QMetaType::SChar || value.userType() == QMetaType::Char)
        return QString::number(value.toInt());
    if (value.userType() == QMetaType::QDateTime)
        return value.toDateTime().toString(Qt::ISODateWithMs);
    if (isStructValue(value)) {
        const QOpcUaGenericStructValue structValue = value.value<QOpcUaGenericStructValue>();
        const QHash<QString, QVariant> fields = structValue.fields();
        QStringList parts;
        const QStringList names = structFieldNames(structValue);
        parts.reserve(names.size());
        for (const QString &name : names)
            parts.append(QStringLiteral("%1: %2").arg(name, displayValue(fields.value(name))));
        return QStringLiteral("%1 {%2}")
            .arg(structValue.typeName(), parts.join(QStringLiteral(", ")));
    }
    if (isValueArray(value)) {
        const QVariantList list = value.toList();
        QStringList parts;
        parts.reserve(list.size());
        for (const QVariant &entry : list)
            parts.append(displayValue(entry));
        return QStringLiteral("[%1]").arg(parts.join(QStringLiteral(", ")));
    }
    return value.toString();
}

///
/// \brief Reports whether a value expands into elements of its own.
/// \param value Variant to inspect.
/// \return True for arrays; strings and byte arrays stay scalar.
///
bool hasValueElements(const QVariant &value)
{
    if (isStructValue(value))
        return !value.value<QOpcUaGenericStructValue>().fields().isEmpty();
    return isValueArray(value) && !value.toList().isEmpty();
}

///
/// \brief Splits a composite value into its elements.
/// \param value Variant to split.
/// \param limit Largest number of elements to return; negative returns all of them.
/// \param totalCount Receives the untruncated element count when not null.
/// \return Elements in their natural order, at most \a limit of them.
///
QVector<ValueElement> valueElements(const QVariant &value, int limit, int *totalCount)
{
    QVector<ValueElement> elements;

    if (isStructValue(value)) {
        const QOpcUaGenericStructValue structValue = value.value<QOpcUaGenericStructValue>();
        const QHash<QString, QVariant> fields = structValue.fields();
        const QStringList names = structFieldNames(structValue);
        if (totalCount)
            *totalCount = names.size();

        const int count = limit < 0 ? names.size() : qMin(limit, names.size());
        elements.reserve(count);
        for (int index = 0; index < count; ++index) {
            const QVariant &field = fields[names.at(index)];
            ValueElement element;
            element.label = names.at(index);
            element.text = displayValue(field);
            element.typeName = fieldTypeName(structValue, names.at(index), field);
            element.value = field;
            element.hasChildren = hasValueElements(field);
            elements.append(element);
        }
        return elements;
    }

    if (!isValueArray(value)) {
        if (totalCount)
            *totalCount = 0;
        return elements;
    }

    const QVariantList list = value.toList();
    if (totalCount)
        *totalCount = list.size();

    const int count = limit < 0 ? list.size() : qMin(limit, list.size());
    elements.reserve(count);
    for (int index = 0; index < count; ++index) {
        const QVariant &entry = list.at(index);
        ValueElement element;
        element.label = QStringLiteral("[%1]").arg(index);
        element.text = displayValue(entry);
        element.value = entry;
        element.hasChildren = hasValueElements(entry);
        if (isStructValue(entry))
            element.typeName = entry.value<QOpcUaGenericStructValue>().typeName();
        elements.append(element);
    }
    return elements;
}

///
/// \brief Renders a composite value as a short summary naming its type and size.
/// \param value Variant to describe.
/// \param type Declared value type, used to name array elements.
/// \param dataTypeId DataType NodeId string, used to name types that are not built-in.
/// \return Summary such as "Int16[3]", or the plain display value for scalars.
///
/// The elements carry the values themselves once the row is expanded, so the cell of a
/// composite value is better spent on the facts a truncated element list would hide.
///
QString valueSummary(const QVariant &value, QOpcUa::Types type, const QString &dataTypeId)
{
    if (isStructValue(value))
        return value.value<QOpcUaGenericStructValue>().typeName();
    if (!isValueArray(value))
        return displayValue(value);

    const QVariantList list = value.toList();
    const QString typeName = !list.isEmpty() && isStructValue(list.constFirst())
        ? list.constFirst().value<QOpcUaGenericStructValue>().typeName()
        : valueTypeDisplay(type, dataTypeId);
    return QStringLiteral("%1[%2]").arg(typeName).arg(list.size());
}

///
/// \brief Returns the translated name of a message security mode.
/// \param mode Security mode to name.
/// \return Localised mode name.
///
QString securityModeName(QOpcUaEndpointDescription::MessageSecurityMode mode)
{
    switch (mode) {
    case QOpcUaEndpointDescription::None: return QObject::tr("None");
    case QOpcUaEndpointDescription::Sign: return QObject::tr("Sign");
    case QOpcUaEndpointDescription::SignAndEncrypt: return QObject::tr("Sign & Encrypt");
    default: return QObject::tr("Invalid");
    }
}

///
/// \brief Returns the textual name of an OPC UA status code.
/// \param status Status code to name.
/// \return Status code name.
///
QString statusName(QOpcUa::UaStatusCode status)
{
    return QOpcUa::statusToString(status);
}

///
/// \brief Formats a status code as name plus zero-padded hexadecimal value.
/// \param status Status code to format.
/// \return Combined name and hex representation.
///
QString statusDisplay(QOpcUa::UaStatusCode status)
{
    return QStringLiteral("%1 (0x%2)")
        .arg(statusName(status))
        .arg(static_cast<quint32>(status), 8, 16, QLatin1Char('0'));
}

///
/// \brief Classifies a status-code name into its OPC UA quality class.
/// \param statusName Status-code name as produced by statusName().
/// \return Matching severity; Unknown for empty or unrecognised names.
///
StatusSeverity statusSeverity(const QString &statusName)
{
    if (statusName.startsWith(QLatin1String("Good")))
        return StatusSeverity::Good;
    if (statusName.startsWith(QLatin1String("Uncertain")))
        return StatusSeverity::Uncertain;
    if (statusName.startsWith(QLatin1String("Bad")))
        return StatusSeverity::Bad;
    return StatusSeverity::Unknown;
}

///
/// \brief Formats a timestamp as a date-time with a zone indicator, or empty when invalid.
/// \param timestamp Timestamp to format.
/// \param mode Local time (trailing UTC offset) or UTC (trailing "Z").
/// \return Space-separated date and time with millisecond precision and a trailing zone indicator.
///
QString isoTimestampWithZone(const QDateTime &timestamp, TimestampMode mode)
{
    if (!timestamp.isValid())
        return QString();
    static const QString format = QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz");
    if (mode == TimestampMode::Utc)
        return timestamp.toUTC().toString(format) + zoneSuffix(0);
    const QDateTime local = timestamp.toLocalTime();
    return local.toString(format) + zoneSuffix(local.offsetFromUtc());
}

///
/// \brief Returns the enum-key name of an OPC UA value type.
/// \param type Value type to name.
/// \return Type name, or "Unknown" when unrecognised.
///
QString valueTypeName(QOpcUa::Types type)
{
    const char *key = QMetaEnum::fromType<QOpcUa::Types>().valueToKey(type);
    return key ? QString::fromLatin1(key) : QObject::tr("Unknown");
}

///
/// \brief Builds the Value attribute, expanding arrays into indexed child rows.
/// \param value Node value.
/// \param type Declared value type, used to label arrays.
/// \param dataTypeId DataType NodeId string, used to name types that are not built-in.
/// \return The constructed Value attribute.
///
OpcUaNodeAttribute valueAttribute(const QVariant &value, QOpcUa::Types type,
                                  const QString &dataTypeId)
{
    OpcUaNodeAttribute result = childAttribute(QStringLiteral("Value"), displayValue(value));
    if (!isValueArray(value))
        return result;

    const QVariantList values = value.toList();
    result.displayValue = QStringLiteral("%1 Array[%2]")
                              .arg(valueTypeDisplay(type, dataTypeId))
                              .arg(values.size());
    for (int index = 0; index < values.size(); ++index) {
        result.children.append(
            childAttribute(QStringLiteral("[%1]").arg(index), displayValue(values.at(index))));
    }
    return result;
}

} // namespace OpcUaFormat
