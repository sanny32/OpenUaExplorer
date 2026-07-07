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

class CertificateTrustDecider;
class OpcUaBackend;

///
/// \brief Owns one QOpcUaClient and exposes transport-neutral asynchronous operations.
///
class OpcUaClientService : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs the service over a freshly created Qt OPC UA backend.
    /// \param parent Owning QObject.
    ///
    explicit OpcUaClientService(QObject *parent = nullptr);

    ///
    /// \brief Constructs the service over an injected backend and forwards its signals.
    /// \param backend Backend implementation to wrap.
    /// \param parent Owning QObject.
    ///
    explicit OpcUaClientService(OpcUaBackend *backend, QObject *parent = nullptr);

    ///
    /// \brief Destroys the service, deleting the backend only when it was self-created and unparented.
    ///
    ~OpcUaClientService() override;

    ///
    /// \brief Reports whether an OPC UA backend is available.
    /// \return True when OPC UA support is usable.
    ///
    bool isAvailable() const;

    ///
    /// \brief Lists the OPC UA backend plugins available at runtime.
    /// \return Backend names.
    ///
    QStringList availableBackends() const;

    ///
    /// \brief Returns the current connection state.
    /// \return The backend's connection state.
    ///
    OpcUaConnectionState state() const;

    ///
    /// \brief Returns the last error reported by the backend.
    /// \return Last error message, or empty.
    ///
    QString lastError() const;

    ///
    /// \brief Returns the DER-encoded certificate of the endpoint in use, or empty.
    /// \return DER-encoded server certificate, or an empty array.
    ///
    QByteArray activeServerCertificate() const;

    ///
    /// \brief Sets the delegate consulted for server-certificate trust decisions.
    /// \param decider Trust decider forwarded to the backend.
    ///
    void setCertificateTrustDecider(CertificateTrustDecider *decider);

    ///
    /// \brief Discovers endpoints with the default timeout.
    /// \param url Discovery URL.
    /// \param backend Backend name to use.
    ///
    void discoverEndpoints(const QString &url,
                           const QString &backend = QStringLiteral("open62541"));

    ///
    /// \brief Discovers the endpoints offered by a server, bounded by a timeout.
    /// \param url Discovery URL.
    /// \param backend Backend name to use.
    /// \param timeoutMs Discovery timeout in milliseconds.
    ///
    void discoverEndpointsWithTimeout(const QString &url, const QString &backend,
                                      int timeoutMs);

    ///
    /// \brief Connects to the endpoint described by a profile, caching its request timeout.
    /// \param profile Connection profile.
    /// \param password User password, if any.
    /// \param privateKeyPassword Private-key password, if any.
    ///
    void connectToEndpoint(const ConnectionProfile &profile,
                           const QString &password = QString(),
                           const QString &privateKeyPassword = QString());

    ///
    /// \brief Disconnects from the current endpoint.
    ///
    void disconnectFromEndpoint();

    ///
    /// \brief Browses the children of a node using the cached request timeout.
    /// \param nodeId Node to browse.
    ///
    void browse(const QString &nodeId);

    ///
    /// \brief Browses the forward references of a node using the cached request timeout.
    /// \param nodeId Node to browse.
    ///
    void browseReferences(const QString &nodeId);

    ///
    /// \brief Reads a node's attributes using the cached request timeout.
    /// \param nodeId Node to read.
    ///
    void readNode(const QString &nodeId);

    ///
    /// \brief Reads the values of several nodes using the cached request timeout.
    /// \param nodeIds Nodes to read.
    ///
    void readValues(const QStringList &nodeIds);

    ///
    /// \brief Reads the raw history of a node's Value using the cached request timeout.
    /// \param nodeId Node whose history is read.
    /// \param start Inclusive range start.
    /// \param end Inclusive range end.
    /// \param numValuesPerNode Maximum samples to return, or 0 for no limit.
    ///
    void readHistoryRaw(const QString &nodeId, const QDateTime &start, const QDateTime &end,
                        quint32 numValuesPerNode);

    ///
    /// \brief Reads historical events for a node using the cached request timeout.
    /// \param nodeId Node whose event history is read.
    /// \param start Inclusive range start.
    /// \param end Inclusive range end.
    /// \param numValuesPerNode Maximum events to return, or 0 for no limit.
    ///
    void readHistoryEvents(const QString &nodeId, const QDateTime &start, const QDateTime &end,
                           quint32 numValuesPerNode);

    ///
    /// \brief Writes a value to a node using the cached request timeout.
    /// \param nodeId Target node.
    /// \param value Value to write.
    /// \param valueType OPC UA type of the value.
    ///
    void writeValue(const QString &nodeId, const QVariant &value, int valueType);

    ///
    /// \brief Reads a method's argument metadata using the cached request timeout.
    /// \param methodNodeId Method node whose InputArguments/OutputArguments are read.
    ///
    void readMethodInfo(const QString &methodNodeId);

    ///
    /// \brief Calls a method on its owning object using the cached request timeout.
    /// \param objectNodeId Object node that owns the method.
    /// \param methodNodeId Method node to call.
    /// \param args Input argument values in call order.
    /// \param argTypes QOpcUa::Types numeric values matching \a args positionally.
    ///
    void callMethod(const QString &objectNodeId, const QString &methodNodeId,
                    const QVariantList &args, const QList<int> &argTypes);

    ///
    /// \brief Enables Value monitoring, or re-applies the interval to an already monitored node.
    /// \param nodeId Node to monitor.
    /// \param publishingInterval Publishing interval in milliseconds.
    ///
    void subscribe(const QString &nodeId, double publishingInterval = 1000.0);

    ///
    /// \brief Disables Value monitoring for a node.
    /// \param nodeId Monitored node.
    ///
    void unsubscribe(const QString &nodeId);

    ///
    /// \brief Enables event monitoring for a node with an EventNotifier.
    /// \param nodeId Node to monitor for events.
    /// \param publishingInterval Publishing interval in milliseconds.
    ///
    void subscribeEvents(const QString &nodeId, double publishingInterval = 1000.0);

    ///
    /// \brief Disables event monitoring for a node.
    /// \param nodeId Node being monitored for events.
    ///
    void unsubscribeEvents(const QString &nodeId);

    ///
    /// \brief Resolves this client's session name from the server diagnostics.
    ///
    void readServerSessionName();

    ///
    /// \brief Reads the server NamespaceArray using the cached request timeout.
    ///
    void requestNamespaces();

    ///
    /// \brief Crawls the address space to count nodes per namespace, using the cached request timeout.
    ///
    void requestNamespaceStatistics();

    ///
    /// \brief Cancels an in-progress namespace statistics crawl, if any.
    ///
    void cancelNamespaceStatistics();

signals:
    ///
    /// \brief Emitted when the connection state changes.
    /// \param state New connection state.
    ///
    void stateChanged(OpcUaConnectionState state);

    ///
    /// \brief Emitted when an operation reports an error.
    /// \param message Error description.
    ///
    void errorOccurred(QString message);

    ///
    /// \brief Emitted when endpoint discovery finishes.
    /// \param endpoints Discovered endpoints.
    /// \param error Error description, empty on success.
    ///
    void endpointsDiscovered(QList<EndpointInfo> endpoints, QString error);

    ///
    /// \brief Emitted when a browse finishes.
    /// \param parentNodeId Browsed node.
    /// \param children Discovered child nodes.
    /// \param error Error description, empty on success.
    ///
    void browseFinished(QString parentNodeId, QVector<OpcUaNodeInfo> children, QString error);

    ///
    /// \brief Emitted when a reference browse finishes.
    /// \param sourceNodeId Browsed node.
    /// \param references Forward reference targets.
    /// \param error Error description, empty on success.
    ///
    void referencesBrowseFinished(QString sourceNodeId, QVector<OpcUaNodeInfo> references,
                                  QString error);

    ///
    /// \brief Emitted when a node read finishes.
    /// \param details Read node details.
    /// \param error Error description, empty on success.
    ///
    void nodeDetailsReady(OpcUaNodeDetails details, QString error);

    ///
    /// \brief Emitted when a batch value read finishes.
    /// \param values Read values.
    /// \param error Error description, empty on success.
    ///
    void dataValuesReady(QVector<OpcUaDataValue> values, QString error);

    ///
    /// \brief Emitted when a raw history read finishes.
    /// \param nodeId Node whose history was read.
    /// \param values History samples in time order.
    /// \param error Error description, empty on success.
    ///
    void historyDataReady(QString nodeId, QVector<OpcUaHistoryValue> values, QString error);

    ///
    /// \brief Emitted when an event history read finishes.
    /// \param nodeId Node whose event history was read.
    /// \param events Historical events in server order.
    /// \param error Error description, empty on success.
    ///
    void historyEventsReady(QString nodeId, QVector<OpcUaEvent> events, QString error);

    ///
    /// \brief Emitted when a write finishes.
    /// \param nodeId Written node.
    /// \param success Whether the write succeeded.
    /// \param error Error description, empty on success.
    ///
    void writeFinished(QString nodeId, bool success, QString error);

    ///
    /// \brief Emitted when a method's argument metadata has been read.
    /// \param methodNodeId Method whose metadata was read.
    /// \param inputs Input argument descriptions in call order.
    /// \param outputs Output argument descriptions in result order.
    /// \param error Error description, empty on success.
    ///
    void methodInfoReady(QString methodNodeId, QVector<OpcUaMethodArgument> inputs,
                         QVector<OpcUaMethodArgument> outputs, QString error);

    ///
    /// \brief Emitted when a method call finishes.
    /// \param methodNodeId Called method.
    /// \param result Raw output value: a single value, or a list for several output arguments.
    /// \param success Whether the call succeeded.
    /// \param error Error description, empty on success.
    ///
    void methodCallFinished(QString methodNodeId, QVariant result, bool success, QString error);

    ///
    /// \brief Emitted after a monitoring request finishes.
    /// \param nodeId Affected node.
    /// \param subscribed True for subscribe and false for unsubscribe.
    /// \param success Whether the request succeeded.
    /// \param error Error description, empty on success.
    ///
    void monitoringFinished(QString nodeId, bool subscribed, bool success, QString error);

    ///
    /// \brief Emitted when events are received for a monitored node.
    /// \param nodeId Monitored node that produced the events.
    /// \param events Received events.
    /// \param error Error description, empty on success.
    ///
    void eventsReady(QString nodeId, QVector<OpcUaEvent> events, QString error);

    ///
    /// \brief Emitted after an event-monitoring request finishes.
    /// \param nodeId Affected node.
    /// \param subscribed True for subscribe and false for unsubscribe.
    /// \param success Whether the request succeeded.
    /// \param error Error description, empty on success.
    ///
    void eventMonitoringFinished(QString nodeId, bool subscribed, bool success, QString error);

    ///
    /// \brief Emitted when the server-assigned session name has been resolved.
    /// \param sessionName Resolved session name, empty when unavailable.
    ///
    void serverSessionNameResolved(QString sessionName);

    ///
    /// \brief Emitted when the server NamespaceArray has been read.
    /// \param namespaces Namespace URIs indexed by namespace index.
    /// \param error Error description, empty on success.
    ///
    void namespacesReady(QStringList namespaces, QString error);

    ///
    /// \brief Emitted periodically while a namespace statistics crawl runs.
    /// \param visitedNodes Number of unique nodes visited so far.
    ///
    void namespaceStatisticsProgress(int visitedNodes);

    ///
    /// \brief Emitted when a namespace statistics crawl finishes or is cancelled.
    /// \param nodeCounts Node counts keyed by namespace index.
    /// \param error Error description, empty on success.
    ///
    void namespaceStatisticsReady(OpcUaNamespaceNodeCounts nodeCounts, QString error);
private:
    OpcUaBackend *_backend;
    bool _ownsBackend;
    int _requestTimeoutMs = 15000;
};
