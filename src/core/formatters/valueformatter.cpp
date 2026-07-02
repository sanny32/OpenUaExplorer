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

namespace OpcUaFormat {

namespace {

///
/// \brief Formats a UTC offset as an ISO 8601 zone suffix ("Z" at zero, otherwise "+/-HH:mm").
/// \param offsetSeconds Offset from UTC in seconds.
/// \return Trailing zone indicator matching Qt::ISODate.
///
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
/// \return The constructed Value attribute.
///
OpcUaNodeAttribute valueAttribute(const QVariant &value, QOpcUa::Types type)
{
    OpcUaNodeAttribute result = childAttribute(QObject::tr("Value"), displayValue(value));
    if (!isValueArray(value))
        return result;

    const QVariantList values = value.toList();
    result.displayValue = QStringLiteral("%1 Array[%2]")
                              .arg(valueTypeName(type))
                              .arg(values.size());
    for (int index = 0; index < values.size(); ++index) {
        result.children.append(
            childAttribute(QStringLiteral("[%1]").arg(index), displayValue(values.at(index))));
    }
    return result;
}

} // namespace OpcUaFormat
