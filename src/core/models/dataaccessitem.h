// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dataaccessitem.h
/// \brief Declares data access item data types.
///

#pragma once

#include <QDateTime>
#include <QString>
#include <QVariant>

///
/// \brief Describes one OPC UA data access item row.
///
struct DataAccessItem
{
    /// \brief NodeId string.
    QString nodeId;
    /// \brief DisplayName text.
    QString displayName;
    /// \brief Formatted value shown in the table.
    QString value;
    /// \brief Formatted data type name.
    QString dataType;
    /// \brief Raw source timestamp, formatted on demand by the model.
    QDateTime sourceTimestamp;
    /// \brief Status display text.
    QString status;
    /// \brief Assigned subscription name.
    QString subscriptionName;
    /// \brief Raw typed value used by writes.
    QVariant typedValue;
    /// \brief QOpcUa::Types numeric value.
    int valueType = 0;
    /// \brief DataType NodeId string.
    QString dataTypeId;
    /// \brief Server timestamp.
    QDateTime serverTimestamp;
    /// \brief UserAccessLevel bit mask.
    quint8 userAccessLevel = 0;
};
