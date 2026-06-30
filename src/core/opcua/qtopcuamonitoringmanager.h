// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QHash>
#include <QObject>

#include "opcuatypes.h"

class QOpcUaClient;
class QOpcUaNode;

///
/// \brief Owns active Qt OPC UA value and event monitored items.
///
class QtOpcUaMonitoringManager : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs an empty monitoring manager.
    /// \param parent Parent object.
    ///
    explicit QtOpcUaMonitoringManager(QObject *parent = nullptr);

    ///
    /// \brief Deletes all active monitored nodes.
    ///
    ~QtOpcUaMonitoringManager() override;

    ///
    /// \brief Sets the client used to create monitored nodes.
    /// \param client Active Qt OPC UA client.
    ///
    void setClient(QOpcUaClient *client);

    ///
    /// \brief Deletes all nodes retained for active monitoring.
    ///
    void clear();

    ///
    /// \brief Enables Value monitoring for a node.
    /// \param nodeId Node to monitor.
    /// \param publishingInterval Publishing interval in milliseconds.
    ///
    void subscribeValue(const QString &nodeId, double publishingInterval);

    ///
    /// \brief Disables Value monitoring for a node.
    /// \param nodeId Monitored node.
    ///
    void unsubscribeValue(const QString &nodeId);

    ///
    /// \brief Enables event monitoring for a node with an EventNotifier.
    /// \param nodeId Node to monitor for events.
    /// \param publishingInterval Publishing interval in milliseconds.
    ///
    void subscribeEvents(const QString &nodeId, double publishingInterval);

    ///
    /// \brief Disables event monitoring for a node.
    /// \param nodeId Node being monitored for events.
    ///
    void unsubscribeEvents(const QString &nodeId);

signals:
    void dataValuesReady(QVector<OpcUaDataValue> values, QString error);
    void monitoringFinished(QString nodeId, bool subscribed, bool success, QString error);
    void eventsReady(QString nodeId, QVector<OpcUaEvent> events, QString error);
    void eventMonitoringFinished(QString nodeId, bool subscribed, bool success, QString error);

private:
    QOpcUaClient *_client = nullptr;
    QHash<QString, QOpcUaNode *> _valueNodes;
    QHash<QString, QOpcUaNode *> _eventNodes;
};
