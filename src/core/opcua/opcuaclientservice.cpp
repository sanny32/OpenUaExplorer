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
    connect(_backend, &OpcUaBackend::serversDiscovered,
            this, &OpcUaClientService::serversDiscovered);
    connect(_backend, &OpcUaBackend::browseFinished,
            this, &OpcUaClientService::browseFinished);
    connect(_backend, &OpcUaBackend::referencesBrowseFinished,
            this, &OpcUaClientService::referencesBrowseFinished);
    connect(_backend, &OpcUaBackend::nodeDetailsReady,
            this, &OpcUaClientService::nodeDetailsReady);
    connect(_backend, &OpcUaBackend::dataValuesReady,
            this, &OpcUaClientService::dataValuesReady);
    connect(_backend, &OpcUaBackend::historyDataReady,
            this, &OpcUaClientService::historyDataReady);
    connect(_backend, &OpcUaBackend::historyEventsReady,
            this, &OpcUaClientService::historyEventsReady);
    connect(_backend, &OpcUaBackend::writeFinished,
            this, &OpcUaClientService::writeFinished);
    connect(_backend, &OpcUaBackend::methodInfoReady,
            this, &OpcUaClientService::methodInfoReady);
    connect(_backend, &OpcUaBackend::methodCallFinished,
            this, &OpcUaClientService::methodCallFinished);
    connect(_backend, &OpcUaBackend::monitoringFinished,
            this, &OpcUaClientService::monitoringFinished);
    connect(_backend, &OpcUaBackend::eventsReady,
            this, &OpcUaClientService::eventsReady);
    connect(_backend, &OpcUaBackend::eventMonitoringFinished,
            this, &OpcUaClientService::eventMonitoringFinished);
    connect(_backend, &OpcUaBackend::serverSessionNameResolved,
            this, &OpcUaClientService::serverSessionNameResolved);
    connect(_backend, &OpcUaBackend::namespacesReady,
            this, &OpcUaClientService::namespacesReady);
    connect(_backend, &OpcUaBackend::namespaceStatisticsProgress,
            this, &OpcUaClientService::namespaceStatisticsProgress);
    connect(_backend, &OpcUaBackend::namespaceStatisticsReady,
            this, &OpcUaClientService::namespaceStatisticsReady);
    connect(_backend, &OpcUaBackend::nodeSearchProgress,
            this, &OpcUaClientService::nodeSearchProgress);
    connect(_backend, &OpcUaBackend::nodeSearchFinished,
            this, &OpcUaClientService::nodeSearchFinished);
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
/// \brief Returns the DER-encoded certificate of the endpoint in use, or empty.
/// \return DER-encoded server certificate, or an empty array.
///
QByteArray OpcUaClientService::activeServerCertificate() const
{
    return _backend->activeServerCertificate();
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
/// \brief Lists the servers registered with a discovery server, with the default timeout.
/// \param url Discovery server URL.
/// \param backend Backend name to use.
///
void OpcUaClientService::findServers(const QString &url, const QString &backend)
{
    findServersWithTimeout(url, backend, 10000);
}

///
/// \brief Lists the servers registered with a discovery server, bounded by a timeout.
/// \param url Discovery server URL.
/// \param backend Backend name to use.
/// \param timeoutMs Request timeout in milliseconds.
///
void OpcUaClientService::findServersWithTimeout(const QString &url, const QString &backend,
                                                int timeoutMs)
{
    _backend->findServers(url, backend, timeoutMs);
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
/// \brief Browses the forward references of a node using the cached request timeout.
/// \param nodeId Node to browse.
///
void OpcUaClientService::browseReferences(const QString &nodeId)
{
    _backend->browseReferences(nodeId, _requestTimeoutMs);
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
/// \brief Reads the raw history of a node's Value using the cached request timeout.
/// \param nodeId Node whose history is read.
/// \param start Inclusive range start.
/// \param end Inclusive range end.
/// \param numValuesPerNode Maximum samples to return, or 0 for no limit.
///
void OpcUaClientService::readHistoryRaw(const QString &nodeId, const QDateTime &start,
                                        const QDateTime &end, quint32 numValuesPerNode)
{
    _backend->readHistoryRaw(nodeId, start, end, numValuesPerNode, _requestTimeoutMs);
}

///
/// \brief Reads historical events for a node using the cached request timeout.
/// \param nodeId Node whose event history is read.
/// \param start Inclusive range start.
/// \param end Inclusive range end.
/// \param numValuesPerNode Maximum events to return, or 0 for no limit.
///
void OpcUaClientService::readHistoryEvents(const QString &nodeId, const QDateTime &start,
                                           const QDateTime &end, quint32 numValuesPerNode)
{
    _backend->readHistoryEvents(nodeId, start, end, numValuesPerNode, _requestTimeoutMs);
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

///
/// \brief Reads a method's argument metadata using the cached request timeout.
/// \param methodNodeId Method node whose InputArguments/OutputArguments are read.
///
void OpcUaClientService::readMethodInfo(const QString &methodNodeId)
{
    _backend->readMethodInfo(methodNodeId, _requestTimeoutMs);
}

///
/// \brief Calls a method on its owning object using the cached request timeout.
/// \param objectNodeId Object node that owns the method.
/// \param methodNodeId Method node to call.
/// \param args Input argument values in call order.
/// \param argTypes QOpcUa::Types numeric values matching \a args positionally.
///
void OpcUaClientService::callMethod(const QString &objectNodeId, const QString &methodNodeId,
                                    const QVariantList &args, const QList<int> &argTypes)
{
    _backend->callMethod(objectNodeId, methodNodeId, args, argTypes, _requestTimeoutMs);
}

///
/// \brief Enables Value monitoring, or re-applies the interval to an already monitored node.
/// \param nodeId Node to monitor.
/// \param publishingInterval Publishing interval in milliseconds.
///
void OpcUaClientService::subscribe(const QString &nodeId, double publishingInterval)
{
    _backend->subscribe(nodeId, publishingInterval);
}

///
/// \brief Disables Value monitoring for a node.
/// \param nodeId Monitored node.
///
void OpcUaClientService::unsubscribe(const QString &nodeId)
{
    _backend->unsubscribe(nodeId);
}

///
/// \brief Enables event monitoring for a node with an EventNotifier.
/// \param nodeId Node to monitor for events.
/// \param publishingInterval Publishing interval in milliseconds.
///
void OpcUaClientService::subscribeEvents(const QString &nodeId, double publishingInterval)
{
    _backend->subscribeEvents(nodeId, publishingInterval);
}

///
/// \brief Disables event monitoring for a node.
/// \param nodeId Node being monitored for events.
///
void OpcUaClientService::unsubscribeEvents(const QString &nodeId)
{
    _backend->unsubscribeEvents(nodeId);
}

///
/// \brief Resolves this client's session name from the server diagnostics.
///
void OpcUaClientService::readServerSessionName()
{
    _backend->readServerSessionName(_requestTimeoutMs);
}

///
/// \brief Reads the server NamespaceArray using the cached request timeout.
///
void OpcUaClientService::requestNamespaces()
{
    _backend->requestNamespaces(_requestTimeoutMs);
}

///
/// \brief Crawls the address space to count nodes per namespace, using the cached request timeout.
///
void OpcUaClientService::requestNamespaceStatistics()
{
    _backend->requestNamespaceStatistics(_requestTimeoutMs);
}

///
/// \brief Cancels an in-progress namespace statistics crawl, if any.
///
void OpcUaClientService::cancelNamespaceStatistics()
{
    _backend->cancelNamespaceStatistics();
}

///
/// \brief Searches a subtree for a display name, using the cached request timeout.
/// \param startNodeId Node whose subtree is searched.
/// \param pattern Case-insensitive substring matched against display names.
///
void OpcUaClientService::searchNode(const QString &startNodeId, const QString &pattern)
{
    _backend->searchNode(startNodeId, pattern, _requestTimeoutMs);
}

///
/// \brief Cancels an in-progress node search, if any.
///
void OpcUaClientService::cancelNodeSearch()
{
    _backend->cancelNodeSearch();
}
