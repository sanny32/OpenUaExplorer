// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include "qtopcuaresultmapper.h"

#include <QHash>
#include <QOpcUaLocalizedText>
#include <QOpcUaSimpleAttributeOperand>
#include <QVariant>

#include "formatters/attributeformatter.h"

namespace QtOpcUaResultMapper {

///
/// \brief Keeps only endpoints whose security policy the active backend can use.
/// \param endpoints Endpoints returned by the discovery request.
/// \param supportedPolicies Security policy URIs reported by the backend.
/// \return Endpoints whose security policy is supported by the backend.
///
QVector<QOpcUaEndpointDescription> endpointsWithSupportedPolicy(
    const QVector<QOpcUaEndpointDescription> &endpoints,
    const QStringList &supportedPolicies)
{
    QVector<QOpcUaEndpointDescription> filtered;
    filtered.reserve(endpoints.size());
    for (const QOpcUaEndpointDescription &endpoint : endpoints) {
        if (supportedPolicies.contains(endpoint.securityPolicy()))
            filtered.append(endpoint);
    }
    return filtered;
}

///
/// \brief Fills browsed children with EventNotifier/Historizing values from a batch read.
/// \param nodes Children to update in place, addressed by NodeId.
/// \param results Read results carrying each node's requested attribute.
///
void applyBrowseAttributeResults(QVector<OpcUaNodeInfo> *nodes,
                                 const QVector<QOpcUaReadResult> &results)
{
    if (!nodes)
        return;

    QHash<QString, quint8> eventNotifiers;
    QHash<QString, bool> historizing;
    for (const QOpcUaReadResult &result : results) {
        if (!QOpcUa::isSuccessStatus(result.statusCode()))
            continue;
        if (result.attribute() == QOpcUa::NodeAttribute::EventNotifier)
            eventNotifiers.insert(result.nodeId(), static_cast<quint8>(result.value().toUInt()));
        else if (result.attribute() == QOpcUa::NodeAttribute::Historizing)
            historizing.insert(result.nodeId(), result.value().toBool());
    }
    for (OpcUaNodeInfo &node : *nodes) {
        if (const auto it = eventNotifiers.constFind(node.nodeId); it != eventNotifiers.constEnd())
            node.eventNotifier = it.value();
        if (const auto it = historizing.constFind(node.nodeId); it != historizing.constEnd())
            node.historizing = it.value();
    }
}

///
/// \brief Maps Qt attribute read results into transport-neutral data values.
/// \param results Attribute read results.
/// \return Data values in result order.
///
QVector<OpcUaDataValue> dataValues(const QVector<QOpcUaReadResult> &results)
{
    QVector<OpcUaDataValue> values;
    values.reserve(results.size());
    for (const QOpcUaReadResult &result : results) {
        OpcUaDataValue value;
        value.nodeId = result.nodeId();
        value.value = result.value();
        value.status = OpcUaFormat::statusName(result.statusCode());
        value.sourceTimestamp = result.sourceTimestamp();
        value.serverTimestamp = result.serverTimestamp();
        values.append(value);
    }
    return values;
}

///
/// \brief Maps a Qt history result into transport-neutral history samples.
/// \param history History result for a single node.
/// \return Samples in time order.
///
QVector<OpcUaHistoryValue> historyValues(const QOpcUaHistoryData &history)
{
    QVector<OpcUaHistoryValue> values;
    const QList<QOpcUaDataValue> results = history.result();
    values.reserve(results.size());
    for (const QOpcUaDataValue &result : results) {
        OpcUaHistoryValue value;
        value.nodeId = history.nodeId();
        value.value = result.value();
        value.status = OpcUaFormat::statusName(result.statusCode());
        value.sourceTimestamp = result.sourceTimestamp();
        value.serverTimestamp = result.serverTimestamp();
        values.append(value);
    }
    return values;
}

///
/// \brief Builds the BaseEventType select clause shared by event reads and subscriptions.
/// \return Select clause covering Time, Severity, SourceName, Message and EventType.
///
QOpcUaMonitoringParameters::EventFilter baseEventFilter()
{
    QOpcUaMonitoringParameters::EventFilter filter;
    filter << QOpcUaSimpleAttributeOperand(QStringLiteral("Time"))
           << QOpcUaSimpleAttributeOperand(QStringLiteral("Severity"))
           << QOpcUaSimpleAttributeOperand(QStringLiteral("SourceName"))
           << QOpcUaSimpleAttributeOperand(QStringLiteral("Message"))
           << QOpcUaSimpleAttributeOperand(QStringLiteral("EventType"));
    return filter;
}

///
/// \brief Builds an OpcUaEvent from the select-clause field values of one notification.
/// \param nodeId Monitored node that produced the event.
/// \param fields Field values in baseEventFilter() order.
/// \return Parsed event record.
///
OpcUaEvent eventFromFields(const QString &nodeId, const QVariantList &fields)
{
    OpcUaEvent event;
    event.sourceNodeId = nodeId;
    if (fields.size() > 0)
        event.time = fields.at(0).toDateTime();
    if (fields.size() > 1)
        event.severity = static_cast<quint16>(fields.at(1).toUInt());
    if (fields.size() > 2)
        event.sourceName = fields.at(2).toString();
    if (fields.size() > 3)
        event.message = fields.at(3).value<QOpcUaLocalizedText>().text();
    if (fields.size() > 4)
        event.eventType = fields.at(4).toString();
    for (const QVariant &field : fields)
        event.fields.append(OpcUaFormat::displayValue(field));
    return event;
}

///
/// \brief Maps a Qt event-history result into transport-neutral events.
/// \param history Event-history result for a single node.
/// \return Events in server order.
///
QVector<OpcUaEvent> historyEvents(const QOpcUaHistoryEvent &history)
{
    QVector<OpcUaEvent> events;
    const QList<QVariantList> results = history.events();
    events.reserve(results.size());
    for (const QVariantList &fields : results)
        events.append(eventFromFields(history.nodeId(), fields));
    return events;
}

} // namespace QtOpcUaResultMapper
