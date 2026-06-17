// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include "opcuabackend.h"
#include "opcuaclientservice.h"
#include "qtopcuabackend.h"

///
/// \brief Constructs the service over a freshly created Qt OPC UA backend.
/// \param parent Owning QObject.
///
OpcUaClientService::OpcUaClientService(QObject *parent)
    : OpcUaClientService(new QtOpcUaBackend, parent)
{
    _backend->setParent(this);
    _ownsBackend = true;
}

///
/// \brief Constructs the service over an injected backend and forwards its signals.
/// \param backend Backend implementation to wrap.
/// \param parent Owning QObject.
///
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

///
/// \brief Destroys the service, deleting the backend only when it was self-created and unparented.
///
OpcUaClientService::~OpcUaClientService()
{
    if (_ownsBackend && !_backend->parent())
        delete _backend;
}

///
/// \brief Reports whether an OPC UA backend is available.
/// \return True when OPC UA support is usable.
///
bool OpcUaClientService::isAvailable() const
{
    return _backend->isAvailable();
}

///
/// \brief Lists the OPC UA backend plugins available at runtime.
/// \return Backend names.
///
QStringList OpcUaClientService::availableBackends() const
{
    return _backend->availableBackends();
}

///
/// \brief Returns the current connection state.
/// \return The backend's connection state.
///
OpcUaConnectionState OpcUaClientService::state() const
{
    return _backend->state();
}

///
/// \brief Returns the last error reported by the backend.
/// \return Last error message, or empty.
///
QString OpcUaClientService::lastError() const
{
    return _backend->lastError();
}

///
/// \brief Sets the delegate consulted for server-certificate trust decisions.
/// \param decider Trust decider forwarded to the backend.
///
void OpcUaClientService::setCertificateTrustDecider(CertificateTrustDecider *decider)
{
    _backend->setCertificateTrustDecider(decider);
}

///
/// \brief Discovers endpoints with the default timeout.
/// \param url Discovery URL.
/// \param backend Backend name to use.
///
void OpcUaClientService::discoverEndpoints(const QString &url, const QString &backend)
{
    discoverEndpointsWithTimeout(url, backend, 10000);
}

///
/// \brief Discovers the endpoints offered by a server, bounded by a timeout.
/// \param url Discovery URL.
/// \param backend Backend name to use.
/// \param timeoutMs Discovery timeout in milliseconds.
///
void OpcUaClientService::discoverEndpointsWithTimeout(const QString &url,
                                                      const QString &backend,
                                                      int timeoutMs)
{
    _backend->discoverEndpoints(url, backend, timeoutMs);
}

///
/// \brief Connects to the endpoint described by a profile, caching its request timeout.
/// \param profile Connection profile.
/// \param password User password, if any.
/// \param privateKeyPassword Private-key password, if any.
///
void OpcUaClientService::connectToEndpoint(const ConnectionProfile &profile,
                                           const QString &password,
                                           const QString &privateKeyPassword)
{
    _requestTimeoutMs = qMax(1000, profile.requestTimeoutMs);
    _backend->connectToEndpoint(profile, password, privateKeyPassword);
}

///
/// \brief Disconnects from the current endpoint.
///
void OpcUaClientService::disconnectFromEndpoint()
{
    _backend->disconnectFromEndpoint();
}

///
/// \brief Browses the children of a node using the cached request timeout.
/// \param nodeId Node to browse.
///
void OpcUaClientService::browse(const QString &nodeId)
{
    _backend->browse(nodeId, _requestTimeoutMs);
}

///
/// \brief Reads a node's attributes using the cached request timeout.
/// \param nodeId Node to read.
///
void OpcUaClientService::readNode(const QString &nodeId)
{
    _backend->readNode(nodeId, _requestTimeoutMs);
}

///
/// \brief Reads the values of several nodes using the cached request timeout.
/// \param nodeIds Nodes to read.
///
void OpcUaClientService::readValues(const QStringList &nodeIds)
{
    _backend->readValues(nodeIds, _requestTimeoutMs);
}

///
/// \brief Writes a value to a node using the cached request timeout.
/// \param nodeId Target node.
/// \param value Value to write.
/// \param valueType OPC UA type of the value.
///
void OpcUaClientService::writeValue(const QString &nodeId, const QVariant &value,
                                    int valueType)
{
    _backend->writeValue(nodeId, value, valueType, _requestTimeoutMs);
}
