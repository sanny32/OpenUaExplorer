// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include "opcuabackend.h"
#include "opcuaclientservice.h"
#include "qtopcuabackend.h"

OpcUaClientService::OpcUaClientService(QObject *parent)
    : OpcUaClientService(new QtOpcUaBackend, parent)
{
    _backend->setParent(this);
    _ownsBackend = true;
}

OpcUaClientService::OpcUaClientService(OpcUaBackend *backend, QObject *parent)
    : QObject(parent)
    , _backend(backend)
    , _ownsBackend(false)
{
    Q_ASSERT(_backend);
    connect(_backend, &OpcUaBackend::stateChanged,
            this, &OpcUaClientService::stateChanged);
    connect(_backend, &OpcUaBackend::errorOccurred,
            this, &OpcUaClientService::errorOccurred);
    connect(_backend, &OpcUaBackend::endpointsDiscovered,
            this, &OpcUaClientService::endpointsDiscovered);
    connect(_backend, &OpcUaBackend::browseFinished,
            this, &OpcUaClientService::browseFinished);
    connect(_backend, &OpcUaBackend::nodeDetailsReady,
            this, &OpcUaClientService::nodeDetailsReady);
    connect(_backend, &OpcUaBackend::dataValuesReady,
            this, &OpcUaClientService::dataValuesReady);
    connect(_backend, &OpcUaBackend::writeFinished,
            this, &OpcUaClientService::writeFinished);
}

OpcUaClientService::~OpcUaClientService()
{
    if (_ownsBackend && !_backend->parent())
        delete _backend;
}

bool OpcUaClientService::isAvailable() const
{
    return _backend->isAvailable();
}

QStringList OpcUaClientService::availableBackends() const
{
    return _backend->availableBackends();
}

OpcUaConnectionState OpcUaClientService::state() const
{
    return _backend->state();
}

QString OpcUaClientService::lastError() const
{
    return _backend->lastError();
}

void OpcUaClientService::setCertificateTrustDecider(CertificateTrustDecider *decider)
{
    _backend->setCertificateTrustDecider(decider);
}

void OpcUaClientService::discoverEndpoints(const QString &url, const QString &backend)
{
    discoverEndpointsWithTimeout(url, backend, 10000);
}

void OpcUaClientService::discoverEndpointsWithTimeout(const QString &url,
                                                      const QString &backend,
                                                      int timeoutMs)
{
    _backend->discoverEndpoints(url, backend, timeoutMs);
}

void OpcUaClientService::connectToEndpoint(const ConnectionProfile &profile,
                                           const QString &password,
                                           const QString &privateKeyPassword)
{
    _requestTimeoutMs = qMax(1000, profile.requestTimeoutMs);
    _backend->connectToEndpoint(profile, password, privateKeyPassword);
}

void OpcUaClientService::disconnectFromEndpoint()
{
    _backend->disconnectFromEndpoint();
}

void OpcUaClientService::browse(const QString &nodeId)
{
    _backend->browse(nodeId, _requestTimeoutMs);
}

void OpcUaClientService::readNode(const QString &nodeId)
{
    _backend->readNode(nodeId, _requestTimeoutMs);
}

void OpcUaClientService::readValues(const QStringList &nodeIds)
{
    _backend->readValues(nodeIds, _requestTimeoutMs);
}

void OpcUaClientService::writeValue(const QString &nodeId, const QVariant &value,
                                    int valueType)
{
    _backend->writeValue(nodeId, value, valueType, _requestTimeoutMs);
}
