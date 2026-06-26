// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file testdata.h
/// \brief Provides sample data for the application UI.
///

#pragma once

#include <QDateTime>
#include <QPair>
#include <QStringList>
#include <QVector>

#include "models/addressspaceitem.h"
#include "models/dataaccessitem.h"
#include "models/logitem.h"
#include "models/nodeitem.h"
#include "models/subscriptionitem.h"
#include "opcua/opcuatypes.h"

namespace TestData {

///
/// \brief Log entry for test data — level, source and message without timestamp.
///
struct LogEntry {
    /// \brief Severity level.
    LogItem::Level level;
    /// \brief Log source name.
    const char    *source;
    /// \brief Log message text.
    const char    *message;
};

///
/// \brief Returns a set of sample log entries covering Info, Warning and Error levels.
/// \return List of LogEntry entries.
///
inline QVector<LogEntry> logItems()
{
    using L = LogItem::Level;
    return {
        {L::Info,    "App",     "Application started"},
        {L::Info,    "Client",  "Connected to opc.tcp://localhost:4840"},
        {L::Info,    "Client",  "Browse completed in 120 ms"},
        {L::Info,    "Session", "Subscription created"},
        {L::Info,    "Session", "Monitored items: 6"},
        {L::Warning, "Client",  "Response timeout on ns=2;s=Device1.Sensors.Pressure"},
        {L::Error,   "Client",  "Read failed: ns=2;s=Device1.Sensors.Flow — BadNodeIdUnknown"},
        {L::Info,    "Session", "Write succeeded: ns=2;s=Device1.Commands.Start = true"},
        {L::Info,    "Session", "Write succeeded: ns=2;s=Device1.Commands.Start = false"},
        {L::Info,    "Client",  "Data change: Temperature = 23.45"},
        {L::Warning, "App",     "Configuration file not found, using defaults"},
    };
}

///
/// \brief Returns the list of available subscription names for use in DataAccessModel.
/// \return List of subscription names.
///
inline QStringList subscriptionNames()
{
    return {"Default", "Fast"};
}

///
/// \brief Returns a set of sample OPC UA monitored items for use in DataAccessModel.
/// \return List of DataAccessItem entries.
///
inline QVector<DataAccessItem> dataAccessItems()
{
    const QDateTime timestamp(QDate(2024, 1, 1), QTime(12, 15, 23, 250), Qt::UTC);
    return {
        {"ns=2;s=Device1.Measurements.Temperature", "Temperature", "23.45",  "Double",  timestamp, "Good", "Default"},
        {"ns=2;s=Device1.Measurements.Pressure",    "Pressure",    "1.013",  "Double",  timestamp, "Good", "Default"},
        {"ns=2;s=Device1.Measurements.Humidity",    "Humidity",    "45.2",   "Double",  timestamp, "Good", "Fast"},
        {"ns=2;s=Device1.Measurements.FlowRate",    "FlowRate",    "12.4",   "Double",  timestamp, "Good", ""},
        {"ns=2;s=Device1.Status.Running",           "Running",     "true",   "Boolean", timestamp, "Good", ""},
        {"ns=2;s=Device1.Status.ErrorCode",         "ErrorCode",   "0",      "UInt32",  timestamp, "Good", ""}
    };
}

///
/// \brief Returns a set of sample OPC UA node attributes for use in AttributesWidget.
/// \return List of attribute name/value pairs.
///
inline QVector<QPair<QString, QString>> attributeItems()
{
    return {
        {"NodeId",           "ns=2;s=Device1.Measurements.Temperature"},
        {"NamespaceIndex",   "2"},
        {"IdentifierType",   "String"},
        {"Identifier",       "Device1.Measurements.Temperature"},
        {"NodeClass",        "Variable"},
        {"BrowseName",       "2, \"Temperature\""},
        {"DisplayName",      "\"Temperature\""},
        {"Description",      "\"Temperature of device 1\""},
        {"WriteMask",        "0"},
        {"UserWriteMask",    "0"},
        {"DataType",         "Double"},
        {"ValueRank",        "-1 (Scalar)"},
        {"ArrayDimensions",  ""},
        {"AccessLevel",      "Read | Write"},
        {"UserAccessLevel",  "Read | Write"}
    };
}

///
/// \brief Returns a sample OPC UA address space tree for use in AddressSpaceModel.
/// \return List of top-level AddressSpaceItem entries with nested children.
///
inline QVector<AddressSpaceItem> addressSpaceItems()
{
    using T = AddressSpaceItem::NodeType;
    return {
        {"Root", T::Folder, {
            {"Objects", T::Folder, {
                {"DeviceSet", T::Node, {
                    {"Device1", T::Node, {
                        {"Status", T::Folder, {
                            {"Running",   T::Variable, {}},
                            {"ErrorCode", T::Variable, {}}
                        }},
                        {"Measurements", T::Folder, {
                            {"Temperature", T::Variable, {}},
                            {"Pressure",    T::Variable, {}},
                            {"Humidity",    T::Variable, {}},
                            {"FlowRate",    T::Variable, {}}
                        }},
                        {"Commands", T::Folder, {
                            {"Start", T::Method, {}},
                            {"Stop",  T::Method, {}},
                            {"Reset", T::Method, {}}
                        }}
                    }},
                    {"Device2", T::Node, {}},
                    {"Device3", T::Node, {}}
                }}
            }},
            {"Types", T::Folder, {}},
            {"Views", T::Folder, {}}
        }}
    };
}

///
/// \brief Returns sample node info entries for the selected node in AddressSpaceWidget.
/// \return List of NodeInfoItem entries.
///
inline QVector<NodeInfoItem> nodeInfoItems()
{
    return {
        {"NodeId:",           "ns=2;s=Device1.Measurements.Temperature"},
        {"Namespace:",        "2"},
        {"Identifier Type:",  "String"},
        {"Data Type:",        "Double"},
        {"Value Rank:",       "-1 (Scalar)"},
        {"Access Level:",     "Read | Write"},
        {"User Access Level:", "Read | Write"},
        {"Description:",      "Temperature of device 1"}
    };
}

///
/// \brief Returns sample OPC UA reference entries for the selected node in AddressSpaceWidget.
/// \return List of ReferenceItem entries.
///
inline QVector<ReferenceItem> referenceItems()
{
    return {
        {"HasTypeDefinition", "BaseDataVariableType"},
        {"HasComponent",      "ns=2;s=Device1.Measurements"},
        {"HasProperty",       "ns=2;s=Device1.Measurements.Temperature.EURange"},
        {"HasProperty",       "ns=2;s=Device1.Measurements.Temperature.EngineeringUnits"}
    };
}

///
/// \brief Returns sample OPC UA subscription entries for use in SubscriptionsModel.
/// \return List of SubscriptionItem entries.
///
inline QVector<SubscriptionItem> subscriptionItems()
{
    return {
        {"Default", 500.0, 0},
        {"Fast",    100.0, 1}
    };
}

///
/// \brief Returns sample OPC UA event entries for use in EventsModel.
/// \return List of EventItem entries.
///
inline QVector<EventItem> eventItems()
{
    return {
        {"12:15:01.100", "100", "Server",  "Server started",                          "SystemStatusChangeEventType"},
        {"12:15:03.450", "100", "Device1", "Node ns=2;s=Device1 modified",            "GeneralModelChangeEventType"},
        {"12:15:08.900", "200", "Device1", "ns=2;s=Device1.Commands.Start = true",    "AuditWriteUpdateEventType"},
        {"12:15:21.300", "700", "Device1", "Temperature exceeded threshold (23.45 > 23.0)", "AlarmConditionType"}
    };
}

///
/// \brief Returns sample OPC UA history samples for use in HistoryModel.
/// \return List of OpcUaHistoryValue entries.
///
inline QVector<OpcUaHistoryValue> historyItems()
{
    const QDateTime base(QDate(2026, 6, 25), QTime(12, 10, 0), Qt::UTC);
    QVector<OpcUaHistoryValue> samples;
    for (int i = 0; i < 3; ++i) {
        OpcUaHistoryValue sample;
        sample.nodeId = QStringLiteral("ns=2;s=Device1.Measurements.Temperature");
        sample.value = 21.0 + i;
        sample.valueType = 11;
        sample.status = QStringLiteral("Good");
        sample.sourceTimestamp = base.addSecs(i);
        sample.serverTimestamp = base.addSecs(i);
        samples.append(sample);
    }
    return samples;
}

} // namespace TestData
