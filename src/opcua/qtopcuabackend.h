// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "opcuabackend.h"

class QtOpcUaBackend : public OpcUaBackend
{
    Q_OBJECT

public:
    explicit QtOpcUaBackend(QObject *parent = nullptr);
    ~QtOpcUaBackend() override;

    bool isAvailable() const override;
    QStringList availableBackends() const override;
    OpcUaConnectionState state() const override;
    QString lastError() const override;
    void setCertificateTrustDecider(CertificateTrustDecider *decider) override;

    void discoverEndpoints(const QString &url, const QString &backend,
                           int timeoutMs) override;
    void connectToEndpoint(const ConnectionProfile &profile,
                           const QString &password,
                           const QString &privateKeyPassword) override;
    void disconnectFromEndpoint() override;
    void browse(const QString &nodeId, int timeoutMs) override;
    void readNode(const QString &nodeId, int timeoutMs) override;
    void readValues(const QStringList &nodeIds, int timeoutMs) override;
    void writeValue(const QString &nodeId, const QVariant &value,
                    int valueType, int timeoutMs) override;

private:
    class Private;
    Private *_d;
};
