// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file opcuaclientservice.h
/// \brief Declares the application OPC UA client service.
///

#pragma once

#include <QObject>
#include <QStringList>

#include "connectionprofile.h"
#include "opcuatypes.h"

///
/// \brief Owns one QOpcUaClient and exposes transport-neutral asynchronous operations.
///
class OpcUaClientService : public QObject
{
    Q_OBJECT

public:
    enum CertificateDecision {
        RejectCertificate = 0,
        TrustCertificateOnce,
        TrustCertificatePermanently
    };

    explicit OpcUaClientService(QObject *parent = nullptr);
    ~OpcUaClientService() override;

    bool isAvailable() const;
    QStringList availableBackends() const;
    OpcUaConnectionState state() const;
    QString lastError() const;

    void discoverEndpoints(const QString &url, const QString &backend = QStringLiteral("open62541"));
    void connectToEndpoint(const ConnectionProfile &profile,
                           const QString &password = QString(),
                           const QString &privateKeyPassword = QString());
    void disconnectFromEndpoint();

    void browse(const QString &nodeId);
    void readNode(const QString &nodeId);
    void readValues(const QStringList &nodeIds);
    void writeValue(const QString &nodeId, const QVariant &value, int valueType);

signals:
    void stateChanged(OpcUaConnectionState state);
    void errorOccurred(QString message);
    void endpointsDiscovered(QList<EndpointInfo> endpoints, QString error);
    void browseFinished(QString parentNodeId, QVector<OpcUaNodeInfo> children, QString error);
    void nodeDetailsReady(OpcUaNodeDetails details, QString error);
    void dataValuesReady(QVector<OpcUaDataValue> values, QString error);
    void writeFinished(QString nodeId, bool success, QString error);
    void certificateValidationRequired(QByteArray certificate,
                                       QString message,
                                       int *decision);

private:
    class Private;
    Private *_d;
};
