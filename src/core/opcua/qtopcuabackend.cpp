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
#include <QHash>
#include <QPointer>
#include <QUrl>

#include <QOpcUaBrowseRequest>
#include <QOpcUaClient>
#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
#include <QOpcUaHistoryData>
#include <QOpcUaHistoryReadRawRequest>
#include <QOpcUaHistoryReadResponse>
#endif
#include <QOpcUaNode>
#include <QOpcUaMonitoringParameters>
#include <QOpcUaReadItem>
#include <QOpcUaReadResult>
#include <QOpcUaReferenceDescription>

#include "attributeformatter.h"
#include "loggingcategories.h"
#if QT_VERSION < QT_VERSION_CHECK(6, 3, 0) && defined(OUAEXP_HAS_OPEN62541_HISTORY)
#include "uahistorybackend.h"
#endif
#include "pkimanager.h"
#include "qtopcuabackend.h"
#include "qtopcuaconnectionmanager.h"
#include "qtopcuarequestcoordinator.h"
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
    {
        QObject::connect(&connection, &QtOpcUaConnectionManager::stateChanged,
                         q, &QtOpcUaBackend::stateChanged);
        QObject::connect(&connection, &QtOpcUaConnectionManager::errorOccurred,
                         q, &QtOpcUaBackend::errorOccurred);
        QObject::connect(&connection, &QtOpcUaConnectionManager::clientInvalidated,
                          q, [this]() {
            cancelRequests();
            clearMonitoredNodes();
            clearActiveProfile();
        });
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
    /// \brief Deletes all nodes retained for active monitoring.
    ///
    void clearMonitoredNodes()
    {
        qDeleteAll(monitoredNodes);
        monitoredNodes.clear();
    }

    ///
    /// \brief Stores connection data required by the direct Qt5 history reader.
    /// \param profile Active connection profile.
    /// \param password Username password.
    /// \param privateKeyPassword Private key password.
    ///
    void rememberActiveProfile(const ConnectionProfile &profile, const QString &password,
                               const QString &privateKeyPassword)
    {
        activeProfile = profile;
        activePassword = password;
        activePrivateKeyPassword = privateKeyPassword;
        hasActiveProfile = true;
    }

    ///
    /// \brief Clears connection data retained for the direct Qt5 history reader.
    ///
    void clearActiveProfile()
    {
        activeProfile = ConnectionProfile();
        activePassword.clear();
        activePrivateKeyPassword.clear();
        hasActiveProfile = false;
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
    QtOpcUaRequestCoordinator requests;
    std::array<QPointer<QOpcUaNode>,
               static_cast<std::size_t>(QtOpcUaRequestCoordinator::Operation::Count)> activeNodes{};
    std::array<QMetaObject::Connection,
               static_cast<std::size_t>(QtOpcUaRequestCoordinator::Operation::Count)> activeConnections{};
    QHash<QString, QOpcUaNode *> monitoredNodes;
    ConnectionProfile activeProfile;
    QString activePassword;
    QString activePrivateKeyPassword;
    bool hasActiveProfile = false;
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
    qRegisterMetaType<OpcUaNodeInfo>();
    qRegisterMetaType<OpcUaNodeDetails>();
    qRegisterMetaType<OpcUaDataValue>();
    qRegisterMetaType<OpcUaHistoryValue>();
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
    _d->connection.setState(OpcUaConnectionState::Discovering);
    constexpr auto operation = QtOpcUaRequestCoordinator::Operation::Discovery;
    const auto token = _d->beginConnectionRequest(operation);
    auto connection = std::make_shared<QMetaObject::Connection>();
    *connection = connect(
        _d->connection.client(), &QOpcUaClient::endpointsRequestFinished, this,
        [this, connection, token, operation](
            const QVector<QOpcUaEndpointDescription> &result,
            QOpcUa::UaStatusCode status, const QUrl &) {
        disconnect(*connection);
        _d->clearConnection(operation);
        if (!_d->requests.settle(token))
            return;
        _d->connection.finishDiscovery(result);
        const QList<EndpointInfo> endpoints = QOpcUa::isSuccessStatus(status)
            ? QtOpcUaTypeMapper::endpointInfos(result) : QList<EndpointInfo>();
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
/// \brief Connects to the discovered endpoint that matches the profile's URL, policy, and mode.
/// \param profile Connection settings.
/// \param password Username password.
/// \param privateKeyPassword Private key password (unsupported; non-empty values are rejected).
///
void QtOpcUaBackend::connectToEndpoint(const ConnectionProfile &profile,
                                        const QString &password,
                                        const QString &privateKeyPassword)
{
    if (_d->connection.connectToEndpoint(profile, password, privateKeyPassword))
        _d->rememberActiveProfile(profile, password, privateKeyPassword);
    else
        _d->clearActiveProfile();
}

///
/// \brief Cancels pending requests and disconnects from the endpoint.
///
void QtOpcUaBackend::disconnectFromEndpoint()
{
    _d->clearActiveProfile();
    _d->connection.disconnectFromEndpoint();
}

///
/// \brief Browses a node's children, emitting browseFinished() with the references or an error.
/// \param nodeId Node to browse.
/// \param timeoutMs Request timeout in milliseconds.
///
void QtOpcUaBackend::browse(const QString &nodeId, int timeoutMs)
{
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
        [this, nodeId](const QVector<QOpcUaReferenceDescription> &references,
                       QOpcUa::UaStatusCode status) {
            if (QOpcUa::isSuccessStatus(status))
                emit browseFinished(nodeId, QtOpcUaTypeMapper::nodeInfos(references), QString());
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
/// \brief Browses a node's forward references, emitting referencesBrowseFinished().
/// \param nodeId Node to browse.
/// \param timeoutMs Request timeout in milliseconds.
///
void QtOpcUaBackend::browseReferences(const QString &nodeId, int timeoutMs)
{
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
/// \param timeoutMs Request timeout in milliseconds.
///
void QtOpcUaBackend::readNode(const QString &nodeId, int timeoutMs)
{
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
/// \param timeoutMs Request timeout in milliseconds.
///
void QtOpcUaBackend::readValues(const QStringList &nodeIds, int timeoutMs)
{
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
        disconnect(*connection);
        _d->clearConnection(operation);
        if (!_d->requests.settle(token))
            return;
        QVector<OpcUaDataValue> values;
        values.reserve(results.size());
        for (const QOpcUaReadResult &result : results) {
            OpcUaDataValue value;
            value.nodeId = result.nodeId();
            value.value = result.value();
            value.status = statusName(result.statusCode());
            value.sourceTimestamp = result.sourceTimestamp();
            value.serverTimestamp = result.serverTimestamp();
            values.append(value);
        }
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

#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
namespace {
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
        value.status = statusName(result.statusCode());
        value.sourceTimestamp = result.sourceTimestamp();
        value.serverTimestamp = result.serverTimestamp();
        values.append(value);
    }
    return values;
}
}

///
/// \brief Reads the raw history of a node's Value, emitting historyDataReady() with the samples.
/// \param nodeId Node whose history is read.
/// \param start Inclusive range start.
/// \param end Inclusive range end.
/// \param numValuesPerNode Maximum samples to return, or 0 for no limit.
/// \param timeoutMs Request timeout in milliseconds.
///
void QtOpcUaBackend::readHistoryRaw(const QString &nodeId, const QDateTime &start,
                                    const QDateTime &end, quint32 numValuesPerNode, int timeoutMs)
{
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
    const auto token = _d->requests.begin(QtOpcUaRequestCoordinator::Operation::HistoryRead);
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
                values = historyValues(nodeResult);
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
#elif defined(OUAEXP_HAS_OPEN62541_HISTORY)
///
/// \brief Reads the raw history of a node's Value through bundled open62541.
/// \param nodeId Node whose history is read.
/// \param start Inclusive range start.
/// \param end Inclusive range end.
/// \param numValuesPerNode Maximum samples to return, or 0 for no limit.
/// \param timeoutMs Request timeout in milliseconds.
///
void QtOpcUaBackend::readHistoryRaw(const QString &nodeId, const QDateTime &start,
                                    const QDateTime &end, quint32 numValuesPerNode, int timeoutMs)
{
    if (!_d->connection.client() || _d->connection.state() != OpcUaConnectionState::Connected
        || !_d->hasActiveProfile) {
        emit historyDataReady(nodeId, {}, tr("The OPC UA client is not connected."));
        return;
    }
    qCInfo(lcClient).noquote()
        << QStringLiteral("HistoryRead request: node=%1 startUtc=%2 endUtc=%3 numValues=%4")
               .arg(nodeId, start.toUTC().toString(Qt::ISODateWithMs),
                    end.toUTC().toString(Qt::ISODateWithMs))
               .arg(numValuesPerNode);
    UaHistoryBackend historyBackend;
    const UaHistoryReadResult result = historyBackend.readRaw(
        _d->activeProfile, _d->activePassword, _d->activePrivateKeyPassword,
        nodeId, start, end, numValuesPerNode, timeoutMs);
    qCInfo(lcClient).noquote()
        << QStringLiteral("HistoryRead reply: node=%1 count=%2 error=%3")
               .arg(nodeId)
               .arg(result.values.size())
               .arg(result.error.isEmpty() ? QStringLiteral("-") : result.error);
    emit historyDataReady(nodeId, result.values, result.error);
}
#else
///
/// \brief Reports that history reads require Qt 6.3 or newer.
/// \param nodeId Node whose history was requested.
/// \param start Inclusive range start (unused).
/// \param end Inclusive range end (unused).
/// \param numValuesPerNode Maximum samples to return (unused).
/// \param timeoutMs Request timeout in milliseconds (unused).
///
void QtOpcUaBackend::readHistoryRaw(const QString &nodeId, const QDateTime &start,
                                    const QDateTime &end, quint32 numValuesPerNode, int timeoutMs)
{
    Q_UNUSED(start)
    Q_UNUSED(end)
    Q_UNUSED(numValuesPerNode)
    Q_UNUSED(timeoutMs)
    emit historyDataReady(nodeId, {}, tr("History read requires Qt 6.3 or newer."));
}
#endif

///
/// \brief Reads the SessionDiagnosticsArray and resolves this client's session name.
/// \param timeoutMs Request timeout in milliseconds.
///
void QtOpcUaBackend::readServerSessionName(int timeoutMs)
{
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
/// \brief Writes a node's Value attribute, emitting writeFinished() with the outcome.
/// \param nodeId Node to write.
/// \param value Typed value.
/// \param valueType QOpcUa::Types numeric value or Undefined.
/// \param timeoutMs Request timeout in milliseconds.
///
void QtOpcUaBackend::writeValue(const QString &nodeId, const QVariant &value,
                                int valueType, int timeoutMs)
{
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
    if (_d->monitoredNodes.contains(nodeId)) {
        QOpcUaNode *monitored = _d->monitoredNodes.take(nodeId);
        connect(monitored, &QOpcUaNode::disableMonitoringFinished, this,
                [this, monitored, nodeId, publishingInterval](QOpcUa::NodeAttribute attribute,
                                                              QOpcUa::UaStatusCode status) {
            if (attribute != QOpcUa::NodeAttribute::Value)
                return;
            monitored->deleteLater();
            if (QOpcUa::isSuccessStatus(status)) {
                subscribe(nodeId, publishingInterval);
            } else {
                emit monitoringFinished(nodeId, true, false, statusName(status));
            }
        });
        if (!monitored->disableMonitoring(QOpcUa::NodeAttribute::Value)) {
            _d->monitoredNodes.insert(nodeId, monitored);
            emit monitoringFinished(nodeId, true, false,
                                    tr("The backend rejected the monitoring update request."));
        }
        return;
    }
    QOpcUaNode *node = _d->connection.client()->node(nodeId);
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
        if (success) {
            _d->monitoredNodes.insert(nodeId, node);
        } else {
            node->deleteLater();
        }
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
void QtOpcUaBackend::unsubscribe(const QString &nodeId)
{
    QOpcUaNode *node = _d->monitoredNodes.take(nodeId);
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
            _d->monitoredNodes.insert(nodeId, node);
        else
            node->deleteLater();
        emit monitoringFinished(nodeId, false, success,
                                success ? QString() : statusName(status));
    });
    if (!node->disableMonitoring(QOpcUa::NodeAttribute::Value)) {
        _d->monitoredNodes.insert(nodeId, node);
        emit monitoringFinished(nodeId, false, false,
                                tr("The backend rejected the unmonitoring request."));
    }
}
