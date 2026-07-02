// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file bitmaskformatter.cpp
/// \brief Implements OPC UA bitmask display helpers.
///

#include "attributeformatter.h"

#include <QMetaEnum>

namespace OpcUaFormat {

///
/// \brief Decodes an access-level bitmask into a pipe-separated list of flag names.
/// \param accessLevel Access-level bits.
/// \return Flag names, or "None" when no bits are set.
///
QString accessLevelDisplay(quint32 accessLevel)
{
    const QMetaEnum metaEnum = QMetaEnum::fromType<QOpcUa::AccessLevelBit>();
    QStringList names;
    for (int index = 0; index < metaEnum.keyCount(); ++index) {
        const quint32 flag = static_cast<quint32>(metaEnum.value(index));
        if (flag && (accessLevel & flag) == flag)
            names.append(QString::fromLatin1(metaEnum.key(index)));
    }
    return names.isEmpty() ? QStringLiteral("None") : names.join(QStringLiteral(" | "));
}

///
/// \brief Decodes a write-mask bitmask into a pipe-separated list of flag names.
/// \param writeMask Write-mask bits.
/// \return Flag names, or the numeric value when no known bits match.
///
QString writeMaskDisplay(quint32 writeMask)
{
    if (!writeMask)
        return QStringLiteral("0");
    const QMetaEnum metaEnum = QMetaEnum::fromType<QOpcUa::WriteMaskBit>();
    QStringList names;
    for (int index = 0; index < metaEnum.keyCount(); ++index) {
        const quint32 flag = static_cast<quint32>(metaEnum.value(index));
        if (flag && (writeMask & flag) == flag)
            names.append(QString::fromLatin1(metaEnum.key(index)));
    }
    return names.isEmpty() ? QString::number(writeMask)
                           : names.join(QStringLiteral(" | "));
}

///
/// \brief Decodes an event-notifier bitmask into a pipe-separated list of flag names.
/// \param eventNotifier Event-notifier bits.
/// \return Flag names, or "None" when no bits are set.
///
QString eventNotifierDisplay(quint8 eventNotifier)
{
    const QMetaEnum metaEnum = QMetaEnum::fromType<QOpcUa::EventNotifierBit>();
    QStringList names;
    for (int index = 0; index < metaEnum.keyCount(); ++index) {
        const quint8 flag = static_cast<quint8>(metaEnum.value(index));
        if (flag && (eventNotifier & flag) == flag)
            names.append(QString::fromLatin1(metaEnum.key(index)));
    }
    return names.isEmpty() ? QStringLiteral("None") : names.join(QStringLiteral(" | "));
}

} // namespace OpcUaFormat
