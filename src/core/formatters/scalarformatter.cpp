// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file scalarformatter.cpp
/// \brief Implements OPC UA scalar text conversion helpers.
///

#include "attributeformatter.h"

#include <limits>

#include <QDateTime>
#include <QUuid>

namespace OpcUaFormat {

///
/// \brief Converts text to a typed scalar, range-checking integral types.
/// \param text Source text.
/// \param type Target OPC UA value type.
/// \param ok Receives the conversion status; must not be null.
/// \return Converted scalar, or an invalid variant on failure.
///
QVariant scalarFromText(const QString &text, QOpcUa::Types type, bool *ok)
{
    *ok = true;
    switch (type) {
    case QOpcUa::Types::Boolean:
        if (text.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0 || text == QLatin1String("1"))
            return true;
        if (text.compare(QLatin1String("false"), Qt::CaseInsensitive) == 0 || text == QLatin1String("0"))
            return false;
        break;
    case QOpcUa::Types::SByte: {
        const int value = text.toInt(ok);
        *ok = *ok && value >= std::numeric_limits<qint8>::min()
            && value <= std::numeric_limits<qint8>::max();
        return QVariant::fromValue(static_cast<qint8>(value));
    }
    case QOpcUa::Types::Byte: {
        const uint value = text.toUInt(ok);
        *ok = *ok && value <= std::numeric_limits<quint8>::max();
        return QVariant::fromValue(static_cast<quint8>(value));
    }
    case QOpcUa::Types::Int16: {
        const int value = text.toInt(ok);
        *ok = *ok && value >= std::numeric_limits<qint16>::min()
            && value <= std::numeric_limits<qint16>::max();
        return QVariant::fromValue(static_cast<qint16>(value));
    }
    case QOpcUa::Types::UInt16: {
        const uint value = text.toUInt(ok);
        *ok = *ok && value <= std::numeric_limits<quint16>::max();
        return QVariant::fromValue(static_cast<quint16>(value));
    }
    case QOpcUa::Types::Int32: return text.toInt(ok);
    case QOpcUa::Types::UInt32:
    case QOpcUa::Types::StatusCode: return text.toUInt(ok);
    case QOpcUa::Types::Int64: return text.toLongLong(ok);
    case QOpcUa::Types::UInt64: return text.toULongLong(ok);
    case QOpcUa::Types::Float: return text.toFloat(ok);
    case QOpcUa::Types::Double: return text.toDouble(ok);
    case QOpcUa::Types::DateTime: {
        const QDateTime dateTime = QDateTime::fromString(text, Qt::ISODateWithMs);
        *ok = dateTime.isValid();
        return dateTime;
    }
    case QOpcUa::Types::Guid: {
        const QUuid uuid(text);
        *ok = !uuid.isNull();
        return uuid;
    }
    case QOpcUa::Types::ByteString:
        return QByteArray::fromBase64(text.toLatin1());
    case QOpcUa::Types::String:
    case QOpcUa::Types::XmlElement:
    case QOpcUa::Types::NodeId:
    case QOpcUa::Types::ExpandedNodeId:
    case QOpcUa::Types::LocalizedText:
    case QOpcUa::Types::QualifiedName:
    case QOpcUa::Types::Undefined:
        return text;
    default:
        break;
    }
    *ok = false;
    return {};
}

} // namespace OpcUaFormat
