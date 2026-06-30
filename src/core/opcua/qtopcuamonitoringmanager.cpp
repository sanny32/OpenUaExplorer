// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include "qtopcuamonitoringmanager.h"

#include <QOpcUaClient>
#include <QOpcUaMonitoringParameters>
#include <QOpcUaNode>

#include "attributeformatter.h"
#include "qtopcuaresultmapper.h"

using namespace OpcUaFormat;

///
/// \brief Constructs an empty monitoring manager.
/// \param parent Parent object.
///
QtOpcUaMonitoringManager::QtOpcUaMonitoringManager(QObject *parent)
    : QObject(parent)
{
}

///
/// \brief Deletes all active monitored nodes.
///
QtOpcUaMonitoringManager::~QtOpcUaMonitoringManager()
{
    clear();
}

///
/// \brief Sets the client used to create monitored nodes.
/// \param client Active Qt OPC UA client.
///
void QtOpcUaMonitoringManager::setClient(QOpcUaClient *client)
{
    _client = client;
}

///
/// \brief Deletes all nodes retained for active monitoring.
///
void QtOpcUaMonitoringManager::clear()
{
    qDeleteAll(_valueNodes);
    _valueNodes.clear();
    qDeleteAll(_eventNodes);
    _eventNodes.clear();
}

///
/// \brief Enables Value monitoring for a node.
/// \param nodeId Node to monitor.
/// \param publishingInterval Publishing interval in milliseconds.
///
void QtOpcUaMonitoringManager::subscribeValue(const QString &nodeId, double publishingInterval)
{
    if (!_client) {
        emit monitoringFinished(nodeId, true, false, tr("The OPC UA client is not connected."));
        return;
    }
    if (_valueNodes.contains(nodeId)) {
        QOpcUaNode *monitored = _valueNodes.take(nodeId);
        connect(monitored, &QOpcUaNode::disableMonitoringFinished, this,
                [this, monitored, nodeId, publishingInterval](QOpcUa::NodeAttribute attribute,
                                                              QOpcUa::UaStatusCode status) {
            if (attribute != QOpcUa::NodeAttribute::Value)
                return;
            monitored->deleteLater();
            if (QOpcUa::isSuccessStatus(status))
                subscribeValue(nodeId, publishingInterval);
            else
                emit monitoringFinished(nodeId, true, false, statusName(status));
        });
        if (!monitored->disableMonitoring(QOpcUa::NodeAttribute::Value)) {
            _valueNodes.insert(nodeId, monitored);
            emit monitoringFinished(nodeId, true, false,
                                    tr("The backend rejected the monitoring update request."));
        }
        return;
    }

    QOpcUaNode *node = _client->node(nodeId);
    if (!node) {
        emit monitoringFinished(nodeId, true, false, tr("Could not create node %1.").arg(nodeId));
        return;
    }
    connect(node, &QOpcUaNode::attributeUpdated, this,
            [this, node, nodeId](QOpcUa::NodeAttribute attribute, const QVariant &value) {
        if (attribute != QOpcUa::NodeAttribute::Value)
            return;
        OpcUaDataValue dataValue;
        dataValue.nodeId = nodeId;
        dataValue.value = value;
        dataValue.status = statusName(node->attributeError(attribute));
        dataValue.sourceTimestamp = node->sourceTimestamp(attribute);
        dataValue.serverTimestamp = node->serverTimestamp(attribute);
        emit dataValuesReady({dataValue}, QString());
    });
    connect(node, &QOpcUaNode::enableMonitoringFinished, this,
            [this, node, nodeId](QOpcUa::NodeAttribute attribute,
                                QOpcUa::UaStatusCode status) {
        if (attribute != QOpcUa::NodeAttribute::Value)
            return;
        const bool success = QOpcUa::isSuccessStatus(status);
        if (success)
            _valueNodes.insert(nodeId, node);
        else
            node->deleteLater();
        emit monitoringFinished(nodeId, true, success,
                                success ? QString() : statusName(status));
    });

    QOpcUaMonitoringParameters parameters(publishingInterval);
    parameters.setSamplingInterval(publishingInterval);
    if (!node->enableMonitoring(QOpcUa::NodeAttribute::Value, parameters)) {
        node->deleteLater();
        emit monitoringFinished(nodeId, true, false,
                                tr("The backend rejected the monitoring request."));
    }
}

///
/// \brief Disables Value monitoring for a node.
/// \param nodeId Monitored node.
///
void QtOpcUaMonitoringManager::unsubscribeValue(const QString &nodeId)
{
    QOpcUaNode *node = _valueNodes.take(nodeId);
    if (!node) {
        emit monitoringFinished(nodeId, false, true, QString());
        return;
    }
    connect(node, &QOpcUaNode::disableMonitoringFinished, this,
            [this, node, nodeId](QOpcUa::NodeAttribute attribute,
                                QOpcUa::UaStatusCode status) {
        if (attribute != QOpcUa::NodeAttribute::Value)
            return;
        const bool success = QOpcUa::isSuccessStatus(status);
        if (!success)
            _valueNodes.insert(nodeId, node);
        else
            node->deleteLater();
        emit monitoringFinished(nodeId, false, success,
                                success ? QString() : statusName(status));
    });
    if (!node->disableMonitoring(QOpcUa::NodeAttribute::Value)) {
        _valueNodes.insert(nodeId, node);
        emit monitoringFinished(nodeId, false, false,
                                tr("The backend rejected the unmonitoring request."));
    }
}

///
/// \brief Enables event monitoring for a node with an EventNotifier.
/// \param nodeId Node to monitor for events.
/// \param publishingInterval Publishing interval in milliseconds.
///
void QtOpcUaMonitoringManager::subscribeEvents(const QString &nodeId, double publishingInterval)
{
    if (!_client) {
        emit eventMonitoringFinished(nodeId, true, false, tr("The OPC UA client is not connected."));
        return;
    }
    if (_eventNodes.contains(nodeId)) {
        QOpcUaNode *monitored = _eventNodes.take(nodeId);
        connect(monitored, &QOpcUaNode::disableMonitoringFinished, this,
                [this, monitored, nodeId, publishingInterval](QOpcUa::NodeAttribute attribute,
                                                              QOpcUa::UaStatusCode status) {
            if (attribute != QOpcUa::NodeAttribute::EventNotifier)
                return;
            monitored->deleteLater();
            if (QOpcUa::isSuccessStatus(status))
                subscribeEvents(nodeId, publishingInterval);
            else
                emit eventMonitoringFinished(nodeId, true, false, statusName(status));
        });
        if (!monitored->disableMonitoring(QOpcUa::NodeAttribute::EventNotifier)) {
            _eventNodes.insert(nodeId, monitored);
            emit eventMonitoringFinished(nodeId, true, false,
                                         tr("The backend rejected the monitoring update request."));
        }
        return;
    }

    QOpcUaNode *node = _client->node(nodeId);
    if (!node) {
        emit eventMonitoringFinished(nodeId, true, false,
                                     tr("Could not create node %1.").arg(nodeId));
        return;
    }
    connect(node, &QOpcUaNode::eventOccurred, this,
            [this, nodeId](const QVariantList &eventFields) {
        emit eventsReady(nodeId, {QtOpcUaResultMapper::eventFromFields(nodeId, eventFields)},
                         QString());
    });
    connect(node, &QOpcUaNode::enableMonitoringFinished, this,
            [this, node, nodeId](QOpcUa::NodeAttribute attribute,
                                QOpcUa::UaStatusCode status) {
        if (attribute != QOpcUa::NodeAttribute::EventNotifier)
            return;
        const bool success = QOpcUa::isSuccessStatus(status);
        if (success)
            _eventNodes.insert(nodeId, node);
        else
            node->deleteLater();
        emit eventMonitoringFinished(nodeId, true, success,
                                     success ? QString() : statusName(status));
    });

    QOpcUaMonitoringParameters parameters(publishingInterval);
    parameters.setFilter(QtOpcUaResultMapper::baseEventFilter());
    if (!node->enableMonitoring(QOpcUa::NodeAttribute::EventNotifier, parameters)) {
        node->deleteLater();
        emit eventMonitoringFinished(nodeId, true, false,
                                     tr("The backend rejected the monitoring request."));
    }
}

///
/// \brief Disables event monitoring for a node.
/// \param nodeId Node being monitored for events.
///
void QtOpcUaMonitoringManager::unsubscribeEvents(const QString &nodeId)
{
    QOpcUaNode *node = _eventNodes.take(nodeId);
    if (!node) {
        emit eventMonitoringFinished(nodeId, false, true, QString());
        return;
    }
    connect(node, &QOpcUaNode::disableMonitoringFinished, this,
            [this, node, nodeId](QOpcUa::NodeAttribute attribute,
                                QOpcUa::UaStatusCode status) {
        if (attribute != QOpcUa::NodeAttribute::EventNotifier)
            return;
        const bool success = QOpcUa::isSuccessStatus(status);
        if (!success)
            _eventNodes.insert(nodeId, node);
        else
            node->deleteLater();
        emit eventMonitoringFinished(nodeId, false, success,
                                     success ? QString() : statusName(status));
    });
    if (!node->disableMonitoring(QOpcUa::NodeAttribute::EventNotifier)) {
        _eventNodes.insert(nodeId, node);
        emit eventMonitoringFinished(nodeId, false, false,
                                     tr("The backend rejected the unmonitoring request."));
    }
}
