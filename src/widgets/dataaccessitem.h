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
