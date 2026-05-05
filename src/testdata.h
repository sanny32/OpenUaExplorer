#pragma once

#include <QPair>
#include <QStringList>
#include <QVector>

#include "widgets/addressspaceitem.h"
#include "widgets/dataaccessitem.h"
#include "widgets/logitem.h"
#include "widgets/nodeitem.h"

namespace TestData {

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
    return {
        {"ns=2;s=Device1.Measurements.Temperature", "Temperature", "23.45",  "Double",  "12:15:23.250", "Good", "Default"},
        {"ns=2;s=Device1.Measurements.Pressure",    "Pressure",    "1.013",  "Double",  "12:15:23.250", "Good", "Default"},
        {"ns=2;s=Device1.Measurements.Humidity",    "Humidity",    "45.2",   "Double",  "12:15:23.250", "Good", "Fast"},
        {"ns=2;s=Device1.Measurements.FlowRate",    "FlowRate",    "12.4",   "Double",  "12:15:23.250", "Good", ""},
        {"ns=2;s=Device1.Status.Running",           "Running",     "true",   "Boolean", "12:15:23.250", "Good", ""},
        {"ns=2;s=Device1.Status.ErrorCode",         "ErrorCode",   "0",      "UInt32",  "12:15:23.250", "Good", ""}
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
/// \brief Returns a set of sample log entries covering Info, Warning and Error levels.
/// \return List of LogItem entries.
///
inline QVector<LogItem> logItems()
{
    using L = LogItem::Level;
    return {
        {"12:14:58.123", L::Info,    "Client", "Connected to opc.tcp://localhost:4840"},
        {"12:14:58.456", L::Info,    "Client", "Browse completed in 120 ms"},
        {"12:15:01.789", L::Info,    "Client", "Subscription created"},
        {"12:15:02.001", L::Info,    "Client", "Monitored items: 6"},
        {"12:15:05.512", L::Warning, "Client", "Response timeout on ns=2;s=Device1.Sensors.Pressure"},
        {"12:15:08.900", L::Error,   "Client", "Read failed: ns=2;s=Device1.Sensors.Flow — BadNodeIdUnknown"},
        {"12:15:10.234", L::Info,    "Client", "Write succeeded: ns=2;s=Device1.Commands.Start = true"},
        {"12:15:10.235", L::Info,    "Client", "Write succeeded: ns=2;s=Device1.Commands.Start = false"},
        {"12:15:23.250", L::Info,    "Client", "Data change: Temperature = 23.45"}
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

} // namespace TestData
