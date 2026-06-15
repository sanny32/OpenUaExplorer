// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>
#include <QStringList>

#include "connectionprofile.h"
#include "opcuatypes.h"

class CertificateTrustDecider;

class OpcUaBackend : public QObject
{
    Q_OBJECT

public:
    explicit OpcUaBackend(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    ~OpcUaBackend() override = default;

    virtual bool isAvailable() const = 0;
    virtual QStringList availableBackends() const = 0;
    virtual OpcUaConnectionState state() const = 0;
    virtual QString lastError() const = 0;
    virtual void setCertificateTrustDecider(CertificateTrustDecider *decider) = 0;

    virtual void discoverEndpoints(const QString &url, const QString &backend,
                                   int timeoutMs) = 0;
    virtual void connectToEndpoint(const ConnectionProfile &profile,
                                   const QString &password,
                                   const QString &privateKeyPassword) = 0;
    virtual void disconnectFromEndpoint() = 0;
    virtual void browse(const QString &nodeId, int timeoutMs) = 0;
    virtual void readNode(const QString &nodeId, int timeoutMs) = 0;
    virtual void readValues(const QStringList &nodeIds, int timeoutMs) = 0;
    virtual void writeValue(const QString &nodeId, const QVariant &value,
                            int valueType, int timeoutMs) = 0;

signals:
    void stateChanged(OpcUaConnectionState state);
    void errorOccurred(QString message);
    void endpointsDiscovered(QList<EndpointInfo> endpoints, QString error);
    void browseFinished(QString parentNodeId, QVector<OpcUaNodeInfo> children, QString error);
    void nodeDetailsReady(OpcUaNodeDetails details, QString error);
    void dataValuesReady(QVector<OpcUaDataValue> values, QString error);
    void writeFinished(QString nodeId, bool success, QString error);
};
