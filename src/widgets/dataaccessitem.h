// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dataaccessitem.h
/// \brief Declares data access item data types.
///

#pragma once

#include <QString>

///
/// \brief Describes one OPC UA data access item row.
///
struct DataAccessItem
{
    QString nodeId;
    QString displayName;
    QString value;
    QString dataType;
    QString sourceTimestamp;
    QString status;
    QString subscriptionName;
};
