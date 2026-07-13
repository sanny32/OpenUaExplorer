// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file qtopcuabackend.cpp
/// \brief Implements the Qt OPC UA transport backend.
///

#include <array>
#include <functional>
#include <memory>

#include <QTimer>
#include <QPointer>
#include <QUrl>

#include <QStringList>

#include <QOpcUaApplicationDescription>
#include <QOpcUaArgument>
#include <QOpcUaBrowseRequest>
#include <QOpcUaClient>
#include <QOpcUaEndpointDescription>
#include <QOpcUaHistoryData>
#include <QOpcUaHistoryEvent>
#include <QOpcUaHistoryReadRawRequest>
#include <QOpcUaHistoryReadResponse>
#include <QOpcUaNode>
#include <QOpcUaMonitoringParameters>
#include <QOpcUaReadItem>
#include <QOpcUaReadResult>
#include <QOpcUaReferenceDescription>

#include "formatters/attributeformatter.h"
#include "loggingcategories.h"
#include "namespacecrawler.h"
#include "nodesearchcrawler.h"
#include "pkimanager.h"
#include "qtopcuabackend.h"
#include "qtopcuaconnectionmanager.h"
#include "qtopcuamonitoringmanager.h"
#include "qtopcuarequestcoordinator.h"
#include "qtopcuaresultmapper.h"
#include "qtopcuatypemapper.h"
#include "standardnodeid.h"

using namespace OpcUaFormat;

///
/// \brief Private implementation holding the Qt OPC UA client and request bookkeeping.
///
class QtOpcUaBackend::Private
{
public:
    ///
    /// \brief Constructs the private state and arms the connection watchdog timer.
    /// \param owner Backend that owns this state.
    ///
    explicit Private(QtOpcUaBackend *owner)
        : q(owner)
        , connection(owner)
        , monitoring(owner)
    {
        QObject::connect(&connection, &QtOpcUaConnectionManager::stateChanged,
                         q, &QtOpcUaBackend::stateChanged);
        QObject::connect(&connection, &QtOpcUaConnectionManager::errorOccurred,
                         q, &QtOpcUaBackend::errorOccurred);
        QObject::connect(&connection, &QtOpcUaConnectionManager::clientInvalidated,
                          q, [this]() {
            cancelRequests();
            if (namespaceCrawler)
                namespaceCrawler->cancel();
            if (nodeSearchCrawler)
                nodeSearchCrawler->cancel();
            monitoring.clear();
            monitoring.setClient(nullptr);
        });
        QObject::connect(&connection, &QtOpcUaConnectionManager::stateChanged,
                         q, [this]() {
            monitoring.setClient(connection.client());
        });
        QObject::connect(&monitoring, &QtOpcUaMonitoringManager::dataValuesReady,
                         q, &QtOpcUaBackend::dataValuesReady);
        QObject::connect(&monitoring, &QtOpcUaMonitoringManager::monitoringFinished,
                         q, &QtOpcUaBackend::monitoringFinished);
        QObject::connect(&monitoring, &QtOpcUaMonitoringManager::eventsReady,
                         q, &QtOpcUaBackend::eventsReady);
        QObject::connect(&monitoring, &QtOpcUaMonitoringManager::eventMonitoringFinished,
                         q, &QtOpcUaBackend::eventMonitoringFinished);
    }

    ///
    /// \brief Invalidates in-flight requests by bumping every operation generation counter.
    ///
    void cancelRequests()
    {
        requests.cancelAll();
        for (QPointer<QOpcUaNode> &node : activeNodes) {
            if (node)
                node->deleteLater();
            node.clear();
        }
        for (QMetaObject::Connection &signalConnection : activeConnections) {
            QObject::disconnect(signalConnection);
            signalConnection = {};
        }
    }

    ///
    /// \brief Starts a client-level request and disconnects its superseded callback.
    /// \param operation Operation category to start.
    /// \return Token for the new request.
    ///
    QtOpcUaRequestCoordinator::Token beginConnectionRequest(
        QtOpcUaRequestCoordinator::Operation operation)
    {
        const std::size_t index = static_cast<std::size_t>(operation);
        QObject::disconnect(activeConnections.at(index));
        activeConnections.at(index) = {};
        return requests.begin(operation);
    }

    ///
    /// \brief Stores the temporary callback connection for an active request.
    /// \param operation Request category.
    /// \param signalConnection Connection to own until the request settles.
    ///
    void trackConnection(QtOpcUaRequestCoordinator::Operation operation,
                         const QMetaObject::Connection &signalConnection)
    {
        activeConnections.at(static_cast<std::size_t>(operation)) = signalConnection;
    }

    ///
    /// \brief Releases the temporary callback connection for a settled request.
    /// \param operation Request category.
    ///
    void clearConnection(QtOpcUaRequestCoordinator::Operation operation)
    {
        activeConnections.at(static_cast<std::size_t>(operation)) = {};
    }

    ///
    /// \brief Wires the shared lifecycle of a single-node request: completion guard, timeout, cleanup.
    /// \param node Freshly created node the request runs on; deleted on every settle path.
    /// \param operation Operation category that supersedes earlier requests of the same kind.
    /// \param timeoutMs Request timeout in milliseconds (floored at 1000).
    /// \param signal Node completion signal to await.
    /// \param onFinished Success handler, run once while the request is still current.
    /// \param start Starts the underlying request; returns false when the backend rejects it.
    /// \param onTimeout Failure emitter used when the request times out.
    /// \param onRejected Failure emitter used when \a start is rejected.
    ///
    /// The generation snapshot makes stale completions (after a reconnect or a newer request)
    /// no-ops, so each request settles exactly once.
    ///
    template <typename Signal, typename Finished, typename Start>
    void runNodeRequest(QOpcUaNode *node, QtOpcUaRequestCoordinator::Operation operation,
                        int timeoutMs, Signal signal,
                        Finished onFinished, Start start,
                        const std::function<void()> &onTimeout,
                        const std::function<void()> &onRejected)
    {
        const std::size_t operationIndex = static_cast<std::size_t>(operation);
        if (activeNodes.at(operationIndex))
            activeNodes.at(operationIndex)->deleteLater();
        activeNodes.at(operationIndex) = node;
        const QtOpcUaRequestCoordinator::Token token = requests.begin(operation);
        QObject::connect(node, signal, q,
                         [this, node, token, operationIndex, onFinished]
                         (auto &&...args) {
            if (!requests.settle(token)) {
                node->deleteLater();
                return;
            }
            activeNodes.at(operationIndex).clear();
            onFinished(std::forward<decltype(args)>(args)...);
            node->deleteLater();
        });
        QTimer::singleShot(QtOpcUaRequestCoordinator::boundedTimeout(timeoutMs), q,
                           [this, node, token, operationIndex, onTimeout]() {
            if (!requests.settle(token))
                return;
            activeNodes.at(operationIndex).clear();
            node->deleteLater();
            onTimeout();
        });
        if (!start()) {
            if (requests.settle(token)) {
                activeNodes.at(operationIndex).clear();
                node->deleteLater();
                onRejected();
            }
        }
    }

    QtOpcUaBackend *q;
    QtOpcUaConnectionManager connection;
    QtOpcUaMonitoringManager monitoring;
    QtOpcUaRequestCoordinator requests;
    std::array<QPointer<QOpcUaNode>,
               static_cast<std::size_t>(QtOpcUaRequestCoordinator::Operation::Count)> activeNodes{};
    std::array<QMetaObject::Connection,
               static_cast<std::size_t>(QtOpcUaRequestCoordinator::Operation::Count)> activeConnections{};
    QPointer<NamespaceCrawler> namespaceCrawler;
    QPointer<NodeSearchCrawler> nodeSearchCrawler;
};

///
/// \brief Constructs the backend and registers its transferable metatypes.
/// \param parent Parent object.
///
QtOpcUaBackend::QtOpcUaBackend(QObject *parent)
    : OpcUaBackend(parent)
    , _d(new Private(this))
{
    qRegisterMetaType<EndpointInfo>();
    qRegisterMetaType<ServerInfo>();
    qRegisterMetaType<OpcUaNodeInfo>();
    qRegisterMetaType<OpcUaMethodArgument>();
    qRegisterMetaType<QVector<OpcUaMethodArgument>>();
    qRegisterMetaType<OpcUaNodeDetails>();
    qRegisterMetaType<OpcUaDataValue>();
    qRegisterMetaType<OpcUaEvent>();
    qRegisterMetaType<OpcUaHistoryValue>();
    qRegisterMetaType<OpcUaNamespaceNodeCounts>();
}

///
/// \brief Destroys the backend, tearing down the client and private state.
///
QtOpcUaBackend::~QtOpcUaBackend()
{
    delete _d;
}

///
/// \brief Reports whether at least one Qt OPC UA backend plugin is installed.
/// \return True when Qt OpcUa and at least one backend are available.
///
bool QtOpcUaBackend::isAvailable() const
{
    return _d->connection.isAvailable();
}

///
/// \brief Lists the installed Qt OPC UA backend plugins.
/// \return Installed Qt OPC UA backend names.
///
QStringList QtOpcUaBackend::availableBackends() const
{
    return _d->connection.availableBackends();
}

///
/// \brief Returns the current connection state.
/// \return Current connection state.
///
OpcUaConnectionState QtOpcUaBackend::state() const
{
    return _d->connection.state();
}

///
/// \brief Returns the most recent error reported by the backend.
/// \return Most recent service error.
///
QString QtOpcUaBackend::lastError() const
{
    return _d->connection.lastError();
}

///
/// \brief Sets the delegate consulted during server-certificate validation.
/// \param decider Trust decider, or nullptr to reject untrusted certificates.
///
void QtOpcUaBackend::setCertificateTrustDecider(CertificateTrustDecider *decider)
{
    _d->connection.setCertificateTrustDecider(decider);
}

///
/// \brief Returns the DER-encoded certificate of the endpoint in use, or empty.
/// \return DER-encoded server certificate, or an empty array.
///
QByteArray QtOpcUaBackend::activeServerCertificate() const
{
    return _d->connection.activeServerCertificate();
}

///
/// \brief Requests the server's endpoint list, emitting endpointsDiscovered() with the result.
/// \param url Discovery URL (must be opc.tcp).
/// \param backend Preferred backend.
/// \param timeoutMs Discovery timeout in milliseconds.
///
void QtOpcUaBackend::discoverEndpoints(const QString &url, const QString &backend,
                                       int timeoutMs)
{
    if (!_d->connection.prepareDiscovery(backend)) {
        emit endpointsDiscovered({}, _d->connection.lastError());
        return;
    }
    const QUrl discoveryUrl(url);
    if (!discoveryUrl.isValid() || discoveryUrl.scheme() != QLatin1String("opc.tcp")) {
        const QString message = tr("Invalid OPC UA endpoint URL: %1").arg(url);
        _d->connection.setError(message);
        emit endpointsDiscovered({}, message);
        return;
    }
    const QString discoveryScheme = discoveryUrl.scheme();
    _d->connection.setState(OpcUaConnectionState::Discovering);
    constexpr auto operation = QtOpcUaRequestCoordinator::Operation::Discovery;
    const auto token = _d->beginConnectionRequest(operation);
    auto connection = std::make_shared<QMetaObject::Connection>();
    *connection = connect(
        _d->connection.client(), &QOpcUaClient::endpointsRequestFinished, this,
        [this, connection, token, operation, discoveryScheme](
            const QVector<QOpcUaEndpointDescription> &result,
            QOpcUa::UaStatusCode status, const QUrl &) {
        disconnect(*connection);
        QStringList supportedPolicies;
        if (QOpcUaClient *client = _d->connection.client())
            supportedPolicies = client->supportedSecurityPolicies();
        _d->clearConnection(operation);
        if (!_d->requests.settle(token))
            return;
        const QVector<QOpcUaEndpointDescription> usable = QOpcUa::isSuccessStatus(status)
            ? QtOpcUaResultMapper::endpointsWithSupportedPolicy(result, supportedPolicies,
                                                                discoveryScheme)
            : QVector<QOpcUaEndpointDescription>();
        _d->connection.finishDiscovery(usable);
        const QList<EndpointInfo> endpoints = QtOpcUaTypeMapper::endpointInfos(usable);
        const QString message = QOpcUa::isSuccessStatus(status)
            ? QString()
            : tr("Endpoint discovery failed: %1").arg(statusName(status));
        emit endpointsDiscovered(endpoints, message);
    });
    _d->trackConnection(operation, *connection);
    QTimer::singleShot(qMax(1000, timeoutMs), this,
                       [this, connection, token, operation]() {
        disconnect(*connection);
        _d->clearConnection(operation);
        if (!_d->requests.settle(token))
            return;
        const QString message = tr("Endpoint discovery timed out.");
        _d->connection.setError(message);
        _d->connection.finishDiscovery({});
        emit endpointsDiscovered({}, message);
    });
    if (!_d->connection.client()->requestEndpoints(discoveryUrl)) {
        disconnect(*connection);
        _d->clearConnection(operation);
        _d->requests.settle(token);
        const QString message = tr("The backend rejected the endpoint discovery request.");
        _d->connection.setError(message);
        _d->connection.finishDiscovery({});
        emit endpointsDiscovered({}, message);
    }
}

///
/// \brief Lists the servers registered with a discovery server, emitting serversDiscovered().
/// \param url Discovery server URL (must be opc.tcp).
/// \param backend Preferred backend.
/// \param timeoutMs Request timeout in milliseconds.
///
void QtOpcUaBackend::findServers(const QString &url, const QString &backend, int timeoutMs)
{
    if (_d->connection.state() != OpcUaConnectionState::Disconnected
        && _d->connection.state() != OpcUaConnectionState::Unavailable) {
        const QString message = tr("Finding servers requires an idle client.");
        _d->connection.setError(message);
        emit serversDiscovered({}, message);
        return;
    }
    if (!_d->connection.prepareDiscovery(backend)) {
        emit serversDiscovered({}, _d->connection.lastError());
        return;
    }
    const QUrl serverUrl(url);
    if (!serverUrl.isValid() || serverUrl.scheme() != QLatin1String("opc.tcp")) {
        const QString message = tr("Invalid OPC UA discovery server URL: %1").arg(url);
        _d->connection.setError(message);
        emit serversDiscovered({}, message);
        return;
    }
    _d->connection.setState(OpcUaConnectionState::Discovering);
    constexpr auto operation = QtOpcUaRequestCoordinator::Operation::FindServers;
    const auto token = _d->beginConnectionRequest(operation);
    auto connection = std::make_shared<QMetaObject::Connection>();
    *connection = connect(
        _d->connection.client(), &QOpcUaClient::findServersFinished, this,
        [this, connection, token, operation](
            const QVector<QOpcUaApplicationDescription> &result,
            QOpcUa::UaStatusCode status, const QUrl &) {
        disconnect(*connection);
        _d->clearConnection(operation);
        if (!_d->requests.settle(token))
            return;
        _d->connection.finishServerLookup();
        const QList<ServerInfo> servers = QOpcUa::isSuccessStatus(status)
            ? QtOpcUaTypeMapper::serverInfos(result)
            : QList<ServerInfo>();
        const QString message = QOpcUa::isSuccessStatus(status)
            ? QString()
            : tr("Finding servers failed: %1").arg(statusName(status));
        emit serversDiscovered(servers, message);
    });
    _d->trackConnection(operation, *connection);
    QTimer::singleShot(qMax(1000, timeoutMs), this,
                       [this, connection, token, operation]() {
        disconnect(*connection);
        _d->clearConnection(operation);
        if (!_d->requests.settle(token))
            return;
        const QString message = tr("Finding servers timed out.");
        _d->connection.setError(message);
        _d->connection.finishServerLookup();
        emit serversDiscovered({}, message);
    });
    if (!_d->connection.client()->findServers(serverUrl)) {
        disconnect(*connection);
        _d->clearConnection(operation);
        _d->requests.settle(token);
        const QString message = tr("The backend rejected the find servers request.");
        _d->connection.setError(message);
        _d->connection.finishServerLookup();
        emit serversDiscovered({}, message);
    }
}

///
/// \brief Connects to the discovered endpoint that matches the profile's URL, policy, and mode.
/// \param profile Connection settings.
/// \param password Username password.
/// \param privateKeyPassword Private key password (unsupported; non-empty values are rejected).
///
void QtOpcUaBackend::connectToEndpoint(const ConnectionProfile &profile,
                                        const QString &password,
                                        const QString &privateKeyPassword)
{
    _d->connection.connectToEndpoint(profile, password, privateKeyPassword);
}

///
/// \brief Cancels pending requests and disconnects from the endpoint.
///
void QtOpcUaBackend::disconnectFromEndpoint()
{
    _d->connection.disconnectFromEndpoint();
}

///
/// \brief Browses a node's children, emitting browseFinished() with the references or an error.
/// \param nodeId Node to browse.
///
void QtOpcUaBackend::browse(const QString &nodeId)
{
    const int timeoutMs = requestTimeout();
    if (!_d->connection.client() || _d->connection.state() != OpcUaConnectionState::Connected) {
        emit browseFinished(nodeId, {}, tr("The OPC UA client is not connected."));
        return;
    }
    QOpcUaNode *node = _d->connection.client()->node(nodeId);
    if (!node) {
        emit browseFinished(nodeId, {}, tr("Could not create node %1.").arg(nodeId));
        return;
    }
    _d->runNodeRequest(node, QtOpcUaRequestCoordinator::Operation::Browse,
        timeoutMs, &QOpcUaNode::browseFinished,
        [this, nodeId, timeoutMs](const QVector<QOpcUaReferenceDescription> &references,
                                  QOpcUa::UaStatusCode status) {
            if (QOpcUa::isSuccessStatus(status))
                enrichAndFinishBrowse(nodeId, QtOpcUaTypeMapper::nodeInfos(references), timeoutMs);
            else
                emit browseFinished(nodeId, {},
                                    tr("Browse failed for %1: %2").arg(nodeId, statusName(status)));
        },
        [node]() { return node->browseChildren(); },
        [this, nodeId]() { emit browseFinished(nodeId, {}, tr("Browse request timed out.")); },
        [this, nodeId]() {
            emit browseFinished(nodeId, {}, tr("The backend rejected the browse request."));
        });
}

///
/// \brief Reads EventNotifier/Historizing of browsed children, then emits browseFinished().
/// \param parentNodeId Browsed node whose children are being delivered.
/// \param nodes Browsed children to enrich with event-source and history-read capability.
/// \param timeoutMs Request timeout in milliseconds.
///
void QtOpcUaBackend::enrichAndFinishBrowse(const QString &parentNodeId,
                                           QVector<OpcUaNodeInfo> nodes, int timeoutMs)
{
    // Browse references omit EventNotifier (Object) and Historizing (Variable); read them so
    // drop targets can filter on event-source and history-read capability.
    QVector<QOpcUaReadItem> items;
    for (const OpcUaNodeInfo &node : nodes) {
        if (node.nodeClass == OpcUa::Object)
            items.append(QOpcUaReadItem(node.nodeId, QOpcUa::NodeAttribute::EventNotifier));
        else if (OpcUa::isVariable(node.nodeClass))
            items.append(QOpcUaReadItem(node.nodeId, QOpcUa::NodeAttribute::Historizing));
    }
    if (items.isEmpty() || !_d->connection.client()) {
        emit browseFinished(parentNodeId, nodes, QString());
        return;
    }

    constexpr auto operation = QtOpcUaRequestCoordinator::Operation::BrowseAttributeRead;
    const auto token = _d->beginConnectionRequest(operation);
    auto enriched = std::make_shared<QVector<OpcUaNodeInfo>>(std::move(nodes));
    auto connection = std::make_shared<QMetaObject::Connection>();
    *connection = connect(_d->connection.client(), &QOpcUaClient::readNodeAttributesFinished, this,
                         [this, connection, token, operation, parentNodeId, enriched](
                             const QVector<QOpcUaReadResult> &results, QOpcUa::UaStatusCode) {
        // readNodeAttributesFinished is connection-wide; value reads carry the Value attribute,
        // this enrichment never does, so use that to ignore other readers' completions.
        if (!results.isEmpty() && results.constFirst().attribute() == QOpcUa::NodeAttribute::Value)
            return;
        disconnect(*connection);
        _d->clearConnection(operation);
        if (!_d->requests.settle(token))
            return;
        QtOpcUaResultMapper::applyBrowseAttributeResults(enriched.get(), results);
        emit browseFinished(parentNodeId, *enriched, QString());
    });
    _d->trackConnection(operation, *connection);
    QTimer::singleShot(QtOpcUaRequestCoordinator::boundedTimeout(timeoutMs), this,
                       [this, connection, token, operation, parentNodeId, enriched]() {
        disconnect(*connection);
        _d->clearConnection(operation);
        if (!_d->requests.settle(token))
            return;
        // Best-effort enrichment: still deliver the children without the extra attributes.
        emit browseFinished(parentNodeId, *enriched, QString());
    });
    if (!_d->connection.client()->readNodeAttributes(items)) {
        disconnect(*connection);
        _d->clearConnection(operation);
        if (_d->requests.settle(token))
            emit browseFinished(parentNodeId, *enriched, QString());
    }
}

///
/// \brief Browses a node's forward references, emitting referencesBrowseFinished().
/// \param nodeId Node to browse.
///
void QtOpcUaBackend::browseReferences(const QString &nodeId)
{
    const int timeoutMs = requestTimeout();
    if (!_d->connection.client() || _d->connection.state() != OpcUaConnectionState::Connected) {
        emit referencesBrowseFinished(nodeId, {},
                                      tr("The OPC UA client is not connected."));
        return;
    }
    QOpcUaNode *node = _d->connection.client()->node(nodeId);
    if (!node) {
        emit referencesBrowseFinished(nodeId, {},
                                      tr("Could not create node %1.").arg(nodeId));
        return;
    }

    QOpcUaBrowseRequest request;
    request.setBrowseDirection(QOpcUaBrowseRequest::BrowseDirection::Forward);
    request.setReferenceTypeId(QOpcUa::ReferenceTypeId::References);
    request.setIncludeSubtypes(true);

    _d->runNodeRequest(node, QtOpcUaRequestCoordinator::Operation::ReferencesBrowse, timeoutMs,
        &QOpcUaNode::browseFinished,
        [this, nodeId](const QVector<QOpcUaReferenceDescription> &references,
                       QOpcUa::UaStatusCode status) {
            if (QOpcUa::isSuccessStatus(status))
                emit referencesBrowseFinished(nodeId, QtOpcUaTypeMapper::nodeInfos(references), QString());
            else
                emit referencesBrowseFinished(nodeId, {},
                                              tr("Browse failed for %1: %2").arg(nodeId, statusName(status)));
        },
        [node, request]() { return node->browse(request); },
        [this, nodeId]() { emit referencesBrowseFinished(nodeId, {}, tr("Browse request timed out.")); },
        [this, nodeId]() {
            emit referencesBrowseFinished(nodeId, {}, tr("The backend rejected the browse request."));
        });
}

///
/// \brief Reads a node's full attribute set, emitting nodeDetailsReady() with the formatted result.
/// \param nodeId Node to read.
///
void QtOpcUaBackend::readNode(const QString &nodeId)
{
    const int timeoutMs = requestTimeout();
    if (!_d->connection.client() || _d->connection.state() != OpcUaConnectionState::Connected) {
        emit nodeDetailsReady({}, tr("The OPC UA client is not connected."));
        return;
    }
    QOpcUaNode *node = _d->connection.client()->node(nodeId);
    if (!node) {
        emit nodeDetailsReady({}, tr("Could not create node %1.").arg(nodeId));
        return;
    }
    const QOpcUa::NodeAttributes attributes = QtOpcUaTypeMapper::nodeDetailAttributes();
    _d->runNodeRequest(node, QtOpcUaRequestCoordinator::Operation::NodeRead,
        timeoutMs, &QOpcUaNode::attributeRead,
        [this, node, nodeId, attributes](QOpcUa::NodeAttributes) {
            emit nodeDetailsReady(QtOpcUaTypeMapper::nodeDetails(
                node, nodeId, attributes, [this](const char *text) { return tr(text); }),
                QString());
        },
        [node, attributes]() { return node->readAttributes(attributes); },
        [this]() { emit nodeDetailsReady({}, tr("Node read timed out.")); },
        [this]() { emit nodeDetailsReady({}, tr("The backend rejected the read request.")); });
}

///
/// \brief Batch-reads the Value attribute of several nodes, emitting dataValuesReady().
/// \param nodeIds Nodes whose Value attributes should be read.
///
void QtOpcUaBackend::readValues(const QStringList &nodeIds)
{
    const int timeoutMs = requestTimeout();
    if (!_d->connection.client() || _d->connection.state() != OpcUaConnectionState::Connected) {
        emit dataValuesReady({}, tr("The OPC UA client is not connected."));
        return;
    }
    QVector<QOpcUaReadItem> items;
    items.reserve(nodeIds.size());
    for (const QString &nodeId : nodeIds)
        items.append(QOpcUaReadItem(nodeId, QOpcUa::NodeAttribute::Value));
    constexpr auto operation = QtOpcUaRequestCoordinator::Operation::ValueRead;
    const auto token = _d->beginConnectionRequest(operation);
    auto connection = std::make_shared<QMetaObject::Connection>();
    *connection = connect(_d->connection.client(), &QOpcUaClient::readNodeAttributesFinished, this,
                         [this, connection, token, operation](
                             const QVector<QOpcUaReadResult> &results,
                             QOpcUa::UaStatusCode serviceResult) {
        // readNodeAttributesFinished is connection-wide; ignore completions of other reads
        // (e.g. browse-time EventNotifier reads) that target a different attribute.
        if (!results.isEmpty() && results.constFirst().attribute() != QOpcUa::NodeAttribute::Value)
            return;
        disconnect(*connection);
        _d->clearConnection(operation);
        if (!_d->requests.settle(token))
            return;
        const QVector<OpcUaDataValue> values = QtOpcUaResultMapper::dataValues(results);
        const QString error = QOpcUa::isSuccessStatus(serviceResult)
            ? QString()
            : tr("Read service failed: %1").arg(statusName(serviceResult));
        emit dataValuesReady(values, error);
    });
    _d->trackConnection(operation, *connection);
    QTimer::singleShot(qMax(1000, timeoutMs), this,
                       [this, connection, token, operation]() {
        disconnect(*connection);
        _d->clearConnection(operation);
        if (!_d->requests.settle(token))
            return;
        emit dataValuesReady({}, tr("Value read timed out."));
    });
    if (!_d->connection.client()->readNodeAttributes(items)) {
        disconnect(*connection);
        _d->clearConnection(operation);
        _d->requests.settle(token);
        emit dataValuesReady({}, tr("The backend rejected the batch read request."));
    }
}

///
/// \brief Reads the raw history of a node's Value, emitting historyDataReady() with the samples.
/// \param nodeId Node whose history is read.
/// \param start Inclusive range start.
/// \param end Inclusive range end.
/// \param numValuesPerNode Maximum samples to return, or 0 for no limit.
///
void QtOpcUaBackend::readHistoryRaw(const QString &nodeId, const QDateTime &start,
                                    const QDateTime &end, quint32 numValuesPerNode)
{
    const int timeoutMs = requestTimeout();
    if (!_d->connection.client() || _d->connection.state() != OpcUaConnectionState::Connected) {
        emit historyDataReady(nodeId, {}, tr("The OPC UA client is not connected."));
        return;
    }
    QOpcUaHistoryReadRawRequest request({ QOpcUaReadItem(nodeId) }, start, end,
                                        numValuesPerNode, false);
    request.setTimestampsToReturn(QOpcUa::TimestampsToReturn::Both);
    qCInfo(lcClient).noquote()
        << QStringLiteral("HistoryRead request: node=%1 startUtc=%2 endUtc=%3 numValues=%4")
               .arg(nodeId, start.toUTC().toString(Qt::ISODateWithMs),
                    end.toUTC().toString(Qt::ISODateWithMs))
               .arg(numValuesPerNode);
    QOpcUaHistoryReadResponse *response = _d->connection.client()->readHistoryData(request);
    if (!response) {
        emit historyDataReady(nodeId, {}, tr("The backend rejected the history read request."));
        return;
    }
    response->setParent(this);
    const auto token = _d->requests.begin(QtOpcUaRequestCoordinator::Operation::HistoryRead, nodeId);
    connect(response, &QOpcUaHistoryReadResponse::readHistoryDataFinished, this,
            [this, response, nodeId, token](const QList<QOpcUaHistoryData> &results,
                                            QOpcUa::UaStatusCode serviceResult) {
        if (!_d->requests.isCurrent(token)) {
            response->deleteLater();
            return;
        }
        qCInfo(lcClient).noquote()
            << QStringLiteral("HistoryRead reply: node=%1 serviceResult=%2 results=%3 "
                              "nodeStatus=%4 count=%5 state=%6")
                   .arg(nodeId, statusName(serviceResult))
                   .arg(results.size())
                   .arg(results.isEmpty() ? QStringLiteral("-")
                                          : statusName(results.first().statusCode()))
                   .arg(results.isEmpty() ? 0 : results.first().count())
                   .arg(static_cast<int>(response->state()));
        if (QOpcUa::isSuccessStatus(serviceResult) && response->hasMoreData()
            && response->readMoreData())
            return;
        if (!_d->requests.settle(token)) {
            response->deleteLater();
            return;
        }
        QString error;
        QVector<OpcUaHistoryValue> values;
        if (!QOpcUa::isSuccessStatus(serviceResult)) {
            error = tr("History read failed: %1").arg(statusName(serviceResult));
        } else if (!results.isEmpty()) {
            const QOpcUaHistoryData &nodeResult = results.first();
            if (nodeResult.count() == 0 && !QOpcUa::isSuccessStatus(nodeResult.statusCode()))
                error = tr("History read failed: %1").arg(statusName(nodeResult.statusCode()));
            else
                values = QtOpcUaResultMapper::historyValues(nodeResult);
        }
        emit historyDataReady(nodeId, values, error);
        response->deleteLater();
    });
    QTimer::singleShot(QtOpcUaRequestCoordinator::boundedTimeout(timeoutMs), this,
                       [this, response, nodeId, token]() {
        if (!_d->requests.settle(token))
            return;
        response->deleteLater();
        emit historyDataReady(nodeId, {}, tr("History read timed out."));
    });
}

///
/// \brief Reads the SessionDiagnosticsArray and resolves this client's session name.
///
void QtOpcUaBackend::readServerSessionName()
{
    const int timeoutMs = requestTimeout();
    if (!_d->connection.client() || _d->connection.state() != OpcUaConnectionState::Connected) {
        emit serverSessionNameResolved(QString());
        return;
    }
    QOpcUaNode *node = _d->connection.client()->node(
        QString::fromLatin1(StandardNodeId::SessionDiagnosticsArray));
    if (!node) {
        emit serverSessionNameResolved(QString());
        return;
    }
    _d->runNodeRequest(node, QtOpcUaRequestCoordinator::Operation::SessionNameRead, timeoutMs,
        &QOpcUaNode::attributeRead,
        [this, node](QOpcUa::NodeAttributes) {
            emit serverSessionNameResolved(
                QtOpcUaTypeMapper::ownSessionName(
                    node->valueAttribute(), PkiManager::applicationUri()));
        },
        [node]() { return node->readAttributes(QOpcUa::NodeAttribute::Value); },
        [this]() { emit serverSessionNameResolved(QString()); },
        [this]() { emit serverSessionNameResolved(QString()); });
}

///
/// \brief Reads the server NamespaceArray, emitting namespacesReady() with the URIs.
///
void QtOpcUaBackend::requestNamespaces()
{
    const int timeoutMs = requestTimeout();
    QOpcUaClient *client = _d->connection.client();
    if (!client || _d->connection.state() != OpcUaConnectionState::Connected) {
        emit namespacesReady({}, tr("The OPC UA client is not connected."));
        return;
    }
    const QStringList cached = client->namespaceArray();
    auto settled = std::make_shared<bool>(false);
    auto connection = std::make_shared<QMetaObject::Connection>();
    *connection = connect(client, &QOpcUaClient::namespaceArrayUpdated, this,
        [this, settled, connection](const QStringList &namespaces) {
            if (*settled)
                return;
            *settled = true;
            disconnect(*connection);
            emit namespacesReady(namespaces, QString());
        });
    QTimer::singleShot(QtOpcUaRequestCoordinator::boundedTimeout(timeoutMs), this,
        [this, settled, connection, cached]() {
            if (*settled)
                return;
            *settled = true;
            disconnect(*connection);
            if (!cached.isEmpty())
                emit namespacesReady(cached, QString());
            else
                emit namespacesReady({}, tr("Reading the server namespace table timed out."));
        });
    if (!client->updateNamespaceArray() && !*settled) {
        *settled = true;
        disconnect(*connection);
        if (!cached.isEmpty())
            emit namespacesReady(cached, QString());
        else
            emit namespacesReady({}, tr("Could not read the server namespace table."));
    }
}

///
/// \brief Crawls the address space, emitting namespaceStatisticsReady() with per-namespace counts.
///
void QtOpcUaBackend::requestNamespaceStatistics()
{
    const int timeoutMs = requestTimeout();
    QOpcUaClient *client = _d->connection.client();
    if (!client || _d->connection.state() != OpcUaConnectionState::Connected) {
        emit namespaceStatisticsReady({}, tr("The OPC UA client is not connected."));
        return;
    }
    if (_d->namespaceCrawler)
        _d->namespaceCrawler->deleteLater();
    _d->namespaceCrawler = new NamespaceCrawler(client, timeoutMs, this);
    connect(_d->namespaceCrawler, &NamespaceCrawler::progress,
            this, &QtOpcUaBackend::namespaceStatisticsProgress);
    connect(_d->namespaceCrawler, &NamespaceCrawler::finished, this,
            [this](const OpcUaNamespaceNodeCounts &counts, const QString &error) {
        emit namespaceStatisticsReady(counts, error);
        if (_d->namespaceCrawler)
            _d->namespaceCrawler->deleteLater();
    });
    _d->namespaceCrawler->start();
}

///
/// \brief Cancels an in-progress namespace statistics crawl, if any.
///
void QtOpcUaBackend::cancelNamespaceStatistics()
{
    if (_d->namespaceCrawler)
        _d->namespaceCrawler->cancel();
}

///
/// \brief Searches a subtree for a display name, emitting nodeSearchFinished() with the match.
///
/// Repeating the same request continues the previous crawl from the match it paused on,
/// which walks the remaining siblings before descending, rather than restarting at the top.
/// \param startNodeId Node whose subtree is searched.
/// \param pattern Case-insensitive substring matched against display names.
///
void QtOpcUaBackend::searchNode(const QString &startNodeId, const QString &pattern)
{
    const int timeoutMs = requestTimeout();
    QOpcUaClient *client = _d->connection.client();
    if (!client || _d->connection.state() != OpcUaConnectionState::Connected) {
        emit nodeSearchFinished({}, {}, tr("The OPC UA client is not connected."));
        return;
    }
    if (_d->nodeSearchCrawler && _d->nodeSearchCrawler->isPaused()
        && _d->nodeSearchCrawler->matches(startNodeId, pattern)) {
        _d->nodeSearchCrawler->resume();
        return;
    }
    if (_d->nodeSearchCrawler) {
        _d->nodeSearchCrawler->disconnect(this);
        _d->nodeSearchCrawler->cancel();
        _d->nodeSearchCrawler->deleteLater();
    }
    _d->nodeSearchCrawler = new NodeSearchCrawler(client, startNodeId, pattern, timeoutMs, this);
    connect(_d->nodeSearchCrawler, &NodeSearchCrawler::progress,
            this, &QtOpcUaBackend::nodeSearchProgress);
    connect(_d->nodeSearchCrawler, &NodeSearchCrawler::finished, this,
            [this](const QStringList &ancestorNodeIds, const QString &nodeId, const QString &error) {
        emit nodeSearchFinished(ancestorNodeIds, nodeId, error);
        if (_d->nodeSearchCrawler && !_d->nodeSearchCrawler->isPaused())
            _d->nodeSearchCrawler->deleteLater();
    });
    _d->nodeSearchCrawler->start();
}

///
/// \brief Cancels an in-progress node search, if any.
///
void QtOpcUaBackend::cancelNodeSearch()
{
    if (_d->nodeSearchCrawler)
        _d->nodeSearchCrawler->cancel();
}

///
/// \brief Writes a node's Value attribute, emitting writeFinished() with the outcome.
/// \param nodeId Node to write.
/// \param value Typed value.
/// \param valueType QOpcUa::Types numeric value or Undefined.
///
void QtOpcUaBackend::writeValue(const QString &nodeId, const QVariant &value, int valueType)
{
    const int timeoutMs = requestTimeout();
    if (!_d->connection.client() || _d->connection.state() != OpcUaConnectionState::Connected) {
        emit writeFinished(nodeId, false, tr("The OPC UA client is not connected."));
        return;
    }
    QOpcUaNode *node = _d->connection.client()->node(nodeId);
    if (!node) {
        emit writeFinished(nodeId, false, tr("Could not create node %1.").arg(nodeId));
        return;
    }
    constexpr auto operation = QtOpcUaRequestCoordinator::Operation::Write;
    const std::size_t operationIndex = static_cast<std::size_t>(operation);
    if (_d->activeNodes.at(operationIndex))
        _d->activeNodes.at(operationIndex)->deleteLater();
    _d->activeNodes.at(operationIndex) = node;
    const auto token = _d->requests.begin(operation);
    QTimer::singleShot(QtOpcUaRequestCoordinator::boundedTimeout(timeoutMs), this,
                       [this, node, nodeId, token, operationIndex]() {
        if (!_d->requests.settle(token))
            return;
        _d->activeNodes.at(operationIndex).clear();
        node->deleteLater();
        emit writeFinished(nodeId, false, tr("Write request timed out."));
    });
    connect(node, &QOpcUaNode::attributeWritten, this,
            [this, node, nodeId, token, operationIndex](
                QOpcUa::NodeAttribute attribute, QOpcUa::UaStatusCode status) {
        if (attribute != QOpcUa::NodeAttribute::Value)
            return;
        if (!_d->requests.settle(token)) {
            node->deleteLater();
            return;
        }
        _d->activeNodes.at(operationIndex).clear();
        const bool success = QOpcUa::isSuccessStatus(status);
        emit writeFinished(nodeId, success,
                           success ? QString() : statusName(status));
        node->deleteLater();
    });
    const QOpcUa::Types type = static_cast<QOpcUa::Types>(valueType);
    if (!node->writeValueAttribute(value, type)) {
        if (_d->requests.settle(token)) {
            _d->activeNodes.at(operationIndex).clear();
            node->deleteLater();
            emit writeFinished(nodeId, false, tr("The backend rejected the write request."));
        }
    }
}

///
/// \brief Maps a method argument-property Value to transport-neutral argument records.
/// \param value Property Value holding one or more QOpcUaArgument entries.
/// \return Arguments in declaration order.
///
static QVector<OpcUaMethodArgument> argumentsFromVariant(const QVariant &value)
{
    QVector<OpcUaMethodArgument> arguments;
    const auto append = [&arguments](const QOpcUaArgument &argument) {
        OpcUaMethodArgument entry;
        entry.name = argument.name();
        entry.dataTypeId = argument.dataTypeId();
        entry.valueType = static_cast<int>(valueTypeForDataType(argument.dataTypeId()));
        entry.valueRank = argument.valueRank();
        entry.description = argument.description().text();
        arguments.append(entry);
    };
    if (value.typeId() == QMetaType::QVariantList) {
        for (const QVariant &entry : value.toList()) {
            if (entry.canConvert<QOpcUaArgument>())
                append(entry.value<QOpcUaArgument>());
        }
    } else if (value.canConvert<QOpcUaArgument>()) {
        append(value.value<QOpcUaArgument>());
    }
    return arguments;
}

///
/// \brief Reads a method's InputArguments/OutputArguments, emitting methodInfoReady().
/// \param methodNodeId Method node whose argument metadata is read.
///
void QtOpcUaBackend::readMethodInfo(const QString &methodNodeId)
{
    const int timeoutMs = requestTimeout();
    if (!_d->connection.client() || _d->connection.state() != OpcUaConnectionState::Connected) {
        emit methodInfoReady(methodNodeId, {}, {}, tr("The OPC UA client is not connected."));
        return;
    }
    QOpcUaNode *node = _d->connection.client()->node(methodNodeId);
    if (!node) {
        emit methodInfoReady(methodNodeId, {}, {}, tr("Could not create node %1.").arg(methodNodeId));
        return;
    }

    QOpcUaBrowseRequest request;
    request.setBrowseDirection(QOpcUaBrowseRequest::BrowseDirection::Forward);
    request.setReferenceTypeId(QOpcUa::ReferenceTypeId::HasProperty);
    request.setIncludeSubtypes(true);

    _d->runNodeRequest(node, QtOpcUaRequestCoordinator::Operation::MethodInfo, timeoutMs,
        &QOpcUaNode::browseFinished,
        [this, methodNodeId, timeoutMs](const QVector<QOpcUaReferenceDescription> &references,
                                        QOpcUa::UaStatusCode status) {
            if (!QOpcUa::isSuccessStatus(status)) {
                emit methodInfoReady(methodNodeId, {}, {},
                    tr("Reading method arguments failed for %1: %2").arg(methodNodeId, statusName(status)));
                return;
            }
            QString inputArgumentsNodeId;
            QString outputArgumentsNodeId;
            for (const OpcUaNodeInfo &reference : QtOpcUaTypeMapper::nodeInfos(references)) {
                if (reference.browseName.endsWith(QLatin1String("InputArguments")))
                    inputArgumentsNodeId = reference.nodeId;
                else if (reference.browseName.endsWith(QLatin1String("OutputArguments")))
                    outputArgumentsNodeId = reference.nodeId;
            }
            if (inputArgumentsNodeId.isEmpty() && outputArgumentsNodeId.isEmpty()) {
                emit methodInfoReady(methodNodeId, {}, {}, QString());
                return;
            }
            readMethodArgumentValues(methodNodeId, inputArgumentsNodeId,
                                     outputArgumentsNodeId, timeoutMs);
        },
        [node, request]() { return node->browse(request); },
        [this, methodNodeId]() {
            emit methodInfoReady(methodNodeId, {}, {}, tr("Reading method arguments timed out."));
        },
        [this, methodNodeId]() {
            emit methodInfoReady(methodNodeId, {}, {},
                                 tr("The backend rejected the method argument request."));
        });
}

///
/// \brief Reads the Value of the located argument-property nodes and emits methodInfoReady().
/// \param methodNodeId Method whose metadata is being resolved.
/// \param inputArgumentsNodeId InputArguments property NodeId, or empty when absent.
/// \param outputArgumentsNodeId OutputArguments property NodeId, or empty when absent.
/// \param timeoutMs Request timeout in milliseconds.
///
void QtOpcUaBackend::readMethodArgumentValues(const QString &methodNodeId,
                                              const QString &inputArgumentsNodeId,
                                              const QString &outputArgumentsNodeId, int timeoutMs)
{
    QOpcUaClient *client = _d->connection.client();
    if (!client || _d->connection.state() != OpcUaConnectionState::Connected) {
        emit methodInfoReady(methodNodeId, {}, {}, tr("The OPC UA client is not connected."));
        return;
    }
    QVector<QOpcUaReadItem> items;
    if (!inputArgumentsNodeId.isEmpty())
        items.append(QOpcUaReadItem(inputArgumentsNodeId, QOpcUa::NodeAttribute::Value));
    if (!outputArgumentsNodeId.isEmpty())
        items.append(QOpcUaReadItem(outputArgumentsNodeId, QOpcUa::NodeAttribute::Value));

    constexpr auto operation = QtOpcUaRequestCoordinator::Operation::MethodInfo;
    const auto token = _d->beginConnectionRequest(operation);
    auto connection = std::make_shared<QMetaObject::Connection>();
    *connection = connect(client, &QOpcUaClient::readNodeAttributesFinished, this,
                         [this, connection, token, operation, methodNodeId,
                          inputArgumentsNodeId, outputArgumentsNodeId](
                             const QVector<QOpcUaReadResult> &results,
                             QOpcUa::UaStatusCode serviceResult) {
        // readNodeAttributesFinished is connection-wide; ignore completions of other reads.
        if (!results.isEmpty() && results.constFirst().attribute() != QOpcUa::NodeAttribute::Value)
            return;
        disconnect(*connection);
        _d->clearConnection(operation);
        if (!_d->requests.settle(token))
            return;
        if (!QOpcUa::isSuccessStatus(serviceResult)) {
            emit methodInfoReady(methodNodeId, {}, {},
                tr("Reading method arguments failed: %1").arg(statusName(serviceResult)));
            return;
        }
        QVector<OpcUaMethodArgument> inputs;
        QVector<OpcUaMethodArgument> outputs;
        for (const QOpcUaReadResult &result : results) {
            if (result.attribute() != QOpcUa::NodeAttribute::Value)
                continue;
            if (!inputArgumentsNodeId.isEmpty() && result.nodeId() == inputArgumentsNodeId)
                inputs = argumentsFromVariant(result.value());
            else if (!outputArgumentsNodeId.isEmpty() && result.nodeId() == outputArgumentsNodeId)
                outputs = argumentsFromVariant(result.value());
        }
        emit methodInfoReady(methodNodeId, inputs, outputs, QString());
    });
    _d->trackConnection(operation, *connection);
    QTimer::singleShot(QtOpcUaRequestCoordinator::boundedTimeout(timeoutMs), this,
                       [this, connection, token, operation, methodNodeId]() {
        disconnect(*connection);
        _d->clearConnection(operation);
        if (!_d->requests.settle(token))
            return;
        emit methodInfoReady(methodNodeId, {}, {}, tr("Reading method arguments timed out."));
    });
    if (!client->readNodeAttributes(items)) {
        disconnect(*connection);
        _d->clearConnection(operation);
        if (_d->requests.settle(token))
            emit methodInfoReady(methodNodeId, {}, {},
                                 tr("The backend rejected the method argument request."));
    }
}

///
/// \brief Calls a method on its owning object, emitting methodCallFinished() with the outputs.
/// \param objectNodeId Object node that owns the method.
/// \param methodNodeId Method node to call.
/// \param args Input argument values in call order.
/// \param argTypes QOpcUa::Types numeric values matching \a args positionally.
///
void QtOpcUaBackend::callMethod(const QString &objectNodeId, const QString &methodNodeId,
                                const QVariantList &args, const QList<int> &argTypes)
{
    const int timeoutMs = requestTimeout();
    if (!_d->connection.client() || _d->connection.state() != OpcUaConnectionState::Connected) {
        emit methodCallFinished(methodNodeId, QVariant(), false,
                                tr("The OPC UA client is not connected."));
        return;
    }
    QOpcUaNode *node = _d->connection.client()->node(objectNodeId);
    if (!node) {
        emit methodCallFinished(methodNodeId, QVariant(), false,
                                tr("Could not create node %1.").arg(objectNodeId));
        return;
    }

    QList<QOpcUa::TypedVariant> typedArgs;
    typedArgs.reserve(args.size());
    for (int i = 0; i < args.size(); ++i) {
        const QOpcUa::Types type = i < argTypes.size()
            ? static_cast<QOpcUa::Types>(argTypes.at(i))
            : QOpcUa::Types::Undefined;
        typedArgs.append(QOpcUa::TypedVariant(args.at(i), type));
    }

    _d->runNodeRequest(node, QtOpcUaRequestCoordinator::Operation::Call, timeoutMs,
        &QOpcUaNode::methodCallFinished,
        [this, methodNodeId](const QString &, const QVariant &result, QOpcUa::UaStatusCode status) {
            const bool success = QOpcUa::isSuccessStatus(status);
            emit methodCallFinished(methodNodeId, result, success,
                                    success ? QString() : statusName(status));
        },
        [node, methodNodeId, typedArgs]() { return node->callMethod(methodNodeId, typedArgs); },
        [this, methodNodeId]() {
            emit methodCallFinished(methodNodeId, QVariant(), false, tr("Method call timed out."));
        },
        [this, methodNodeId]() {
            emit methodCallFinished(methodNodeId, QVariant(), false,
                                    tr("The backend rejected the method call request."));
        });
}

///
/// \brief Enables Value monitoring for a node.
/// \param nodeId Node to monitor.
/// \param publishingInterval Publishing interval in milliseconds.
///
void QtOpcUaBackend::subscribe(const QString &nodeId, double publishingInterval)
{
    if (!_d->connection.client() || _d->connection.state() != OpcUaConnectionState::Connected) {
        emit monitoringFinished(nodeId, true, false, tr("The OPC UA client is not connected."));
        return;
    }
    _d->monitoring.subscribeValue(nodeId, publishingInterval);
}

///
/// \brief Disables Value monitoring for a node.
/// \param nodeId Monitored node.
///
void QtOpcUaBackend::unsubscribe(const QString &nodeId)
{
    _d->monitoring.unsubscribeValue(nodeId);
}

///
/// \brief Reads historical events for a node, emitting historyEventsReady().
/// \param nodeId Node whose event history is read.
/// \param start Inclusive range start.
/// \param end Inclusive range end.
/// \param numValuesPerNode Maximum events to return, or 0 for no limit.
///
void QtOpcUaBackend::readHistoryEvents(const QString &nodeId, const QDateTime &start,
                                       const QDateTime &end, quint32 numValuesPerNode)
{
    const int timeoutMs = requestTimeout();
    if (!_d->connection.client() || _d->connection.state() != OpcUaConnectionState::Connected) {
        emit historyEventsReady(nodeId, {}, tr("The OPC UA client is not connected."));
        return;
    }

    QOpcUaNode *node = _d->connection.client()->node(nodeId);
    if (!node) {
        emit historyEventsReady(nodeId, {}, tr("Could not create node %1.").arg(nodeId));
        return;
    }

    QOpcUaMonitoringParameters::EventFilter filter = QtOpcUaResultMapper::baseEventFilter();
    qCInfo(lcClient).noquote()
        << QStringLiteral("HistoryReadEvents request: node=%1 startUtc=%2 endUtc=%3 numValues=%4")
               .arg(nodeId, start.toUTC().toString(Qt::ISODateWithMs),
                    end.toUTC().toString(Qt::ISODateWithMs))
               .arg(numValuesPerNode);
    QOpcUaHistoryReadResponse *response =
        node->readHistoryEvents(start, end, filter, numValuesPerNode);
    if (!response) {
        node->deleteLater();
        emit historyEventsReady(nodeId, {}, tr("The backend rejected the event history request."));
        return;
    }

    response->setParent(this);
    node->setParent(response);
    const auto token = _d->requests.begin(QtOpcUaRequestCoordinator::Operation::HistoryEventsRead);
    connect(response, &QOpcUaHistoryReadResponse::readHistoryEventsFinished, this,
            [this, response, nodeId, token](const QList<QOpcUaHistoryEvent> &results,
                                            QOpcUa::UaStatusCode serviceResult) {
        if (!_d->requests.isCurrent(token)) {
            response->deleteLater();
            return;
        }
        qCInfo(lcClient).noquote()
            << QStringLiteral("HistoryReadEvents reply: node=%1 serviceResult=%2 results=%3 "
                              "nodeStatus=%4 count=%5 state=%6")
                   .arg(nodeId, statusName(serviceResult))
                   .arg(results.size())
                   .arg(results.isEmpty() ? QStringLiteral("-")
                                          : statusName(results.first().statusCode()))
                   .arg(results.isEmpty() ? 0 : results.first().count())
                   .arg(static_cast<int>(response->state()));
        if (QOpcUa::isSuccessStatus(serviceResult) && response->hasMoreData()
            && response->readMoreData())
            return;
        if (!_d->requests.settle(token)) {
            response->deleteLater();
            return;
        }
        QString error;
        QVector<OpcUaEvent> events;
        if (!QOpcUa::isSuccessStatus(serviceResult)) {
            error = tr("Event history read failed: %1").arg(statusName(serviceResult));
        } else if (!results.isEmpty()) {
            const QOpcUaHistoryEvent &nodeResult = results.first();
            if (nodeResult.count() == 0 && !QOpcUa::isSuccessStatus(nodeResult.statusCode()))
                error = tr("Event history read failed: %1").arg(statusName(nodeResult.statusCode()));
            else
                events = QtOpcUaResultMapper::historyEvents(nodeResult);
        }
        emit historyEventsReady(nodeId, events, error);
        response->deleteLater();
    });
    QTimer::singleShot(QtOpcUaRequestCoordinator::boundedTimeout(timeoutMs), this,
                       [this, response, nodeId, token]() {
        if (!_d->requests.settle(token))
            return;
        response->deleteLater();
        emit historyEventsReady(nodeId, {}, tr("Event history read timed out."));
    });
}

///
/// \brief Enables event monitoring for a node with an EventNotifier.
/// \param nodeId Node to monitor for events.
/// \param publishingInterval Publishing interval in milliseconds.
///
void QtOpcUaBackend::subscribeEvents(const QString &nodeId, double publishingInterval)
{
    if (!_d->connection.client() || _d->connection.state() != OpcUaConnectionState::Connected) {
        emit eventMonitoringFinished(nodeId, true, false, tr("The OPC UA client is not connected."));
        return;
    }
    _d->monitoring.subscribeEvents(nodeId, publishingInterval);
}

///
/// \brief Disables event monitoring for a node.
/// \param nodeId Node being monitored for events.
///
void QtOpcUaBackend::unsubscribeEvents(const QString &nodeId)
{
    _d->monitoring.unsubscribeEvents(nodeId);
}
