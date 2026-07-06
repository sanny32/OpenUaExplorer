// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>
#include <QStringList>

#include "connectionprofile.h"
#include "opcuatypes.h"

class CertificateTrustDecider;

///
/// \brief Abstract transport interface for asynchronous OPC UA operations.
///
class OpcUaBackend : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs the backend base.
    /// \param parent Owning QObject.
    ///
    explicit OpcUaBackend(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    ///
    /// \brief Default destructor.
    ///
    ~OpcUaBackend() override = default;

    ///
    /// \brief Reports whether the backend can be used.
    /// \return True when OPC UA support is usable.
    ///
    virtual bool isAvailable() const = 0;

    ///
    /// \brief Lists the backend plugins available at runtime.
    /// \return Backend names.
    ///
    virtual QStringList availableBackends() const = 0;

    ///
    /// \brief Returns the current connection state.
    /// \return Connection state.
    ///
    virtual OpcUaConnectionState state() const = 0;

    ///
    /// \brief Returns the last error reported.
    /// \return Last error message, or empty.
    ///
    virtual QString lastError() const = 0;

    ///
    /// \brief Sets the delegate consulted for server-certificate trust decisions.
    /// \param decider Trust decider.
    ///
    virtual void setCertificateTrustDecider(CertificateTrustDecider *decider) = 0;

    ///
    /// \brief Returns the DER-encoded certificate of the endpoint in use, or empty.
    ///
    /// Backends that do not track the active endpoint may leave the default,
    /// which reports no certificate.
    /// \return DER-encoded server certificate, or an empty array.
    ///
    virtual QByteArray activeServerCertificate() const { return {}; }

    ///
    /// \brief Discovers the endpoints offered by a server, bounded by a timeout.
    /// \param url Discovery URL.
    /// \param backend Backend name to use.
    /// \param timeoutMs Discovery timeout in milliseconds.
    ///
    virtual void discoverEndpoints(const QString &url, const QString &backend,
                                   int timeoutMs) = 0;

    ///
    /// \brief Connects to the endpoint described by a profile.
    /// \param profile Connection profile.
    /// \param password User password, if any.
    /// \param privateKeyPassword Private-key password, if any.
    ///
    virtual void connectToEndpoint(const ConnectionProfile &profile,
                                   const QString &password,
                                   const QString &privateKeyPassword) = 0;

    ///
    /// \brief Disconnects from the current endpoint.
    ///
    virtual void disconnectFromEndpoint() = 0;

    ///
    /// \brief Browses a node's children.
    /// \param nodeId Node to browse.
    /// \param timeoutMs Request timeout in milliseconds.
    ///
    virtual void browse(const QString &nodeId, int timeoutMs) = 0;

    ///
    /// \brief Browses a node's forward references.
    /// \param nodeId Node to browse.
    /// \param timeoutMs Request timeout in milliseconds.
    ///
    virtual void browseReferences(const QString &nodeId, int timeoutMs) = 0;

    ///
    /// \brief Reads a node's attributes.
    /// \param nodeId Node to read.
    /// \param timeoutMs Request timeout in milliseconds.
    ///
    virtual void readNode(const QString &nodeId, int timeoutMs) = 0;

    ///
    /// \brief Reads the Value attribute of several nodes.
    /// \param nodeIds Nodes to read.
    /// \param timeoutMs Request timeout in milliseconds.
    ///
    virtual void readValues(const QStringList &nodeIds, int timeoutMs) = 0;

    ///
    /// \brief Writes a value to a node.
    /// \param nodeId Target node.
    /// \param value Value to write.
    /// \param valueType OPC UA type of the value.
    /// \param timeoutMs Request timeout in milliseconds.
    ///
    virtual void writeValue(const QString &nodeId, const QVariant &value,
                            int valueType, int timeoutMs) = 0;

    ///
    /// \brief Reads the raw history of a node's Value over a time range.
    /// \param nodeId Node whose history is read.
    /// \param start Inclusive range start.
    /// \param end Inclusive range end.
    /// \param numValuesPerNode Maximum samples to return, or 0 for no limit.
    /// \param timeoutMs Request timeout in milliseconds.
    ///
    virtual void readHistoryRaw(const QString &nodeId, const QDateTime &start,
                                const QDateTime &end, quint32 numValuesPerNode, int timeoutMs)
    {
        Q_UNUSED(start)
        Q_UNUSED(end)
        Q_UNUSED(numValuesPerNode)
        Q_UNUSED(timeoutMs)
        emit historyDataReady(nodeId, {}, tr("History read is not supported."));
    }

    ///
    /// \brief Reads historical events for a node over a time range.
    /// \param nodeId Node whose event history is read.
    /// \param start Inclusive range start.
    /// \param end Inclusive range end.
    /// \param numValuesPerNode Maximum events to return, or 0 for no limit.
    /// \param timeoutMs Request timeout in milliseconds.
    ///
    virtual void readHistoryEvents(const QString &nodeId, const QDateTime &start,
                                   const QDateTime &end, quint32 numValuesPerNode, int timeoutMs)
    {
        Q_UNUSED(start)
        Q_UNUSED(end)
        Q_UNUSED(numValuesPerNode)
        Q_UNUSED(timeoutMs)
        emit historyEventsReady(nodeId, {}, tr("Event history read is not supported."));
    }

    ///
    /// \brief Enables Value monitoring for a node.
    /// \param nodeId Node to monitor.
    /// \param publishingInterval Publishing interval in milliseconds.
    ///
    virtual void subscribe(const QString &nodeId, double publishingInterval)
    {
        Q_UNUSED(publishingInterval)
        emit monitoringFinished(nodeId, true, false, tr("Monitoring is not supported."));
    }

    ///
    /// \brief Disables Value monitoring for a node.
    /// \param nodeId Monitored node.
    ///
    virtual void unsubscribe(const QString &nodeId)
    {
        emit monitoringFinished(nodeId, false, false, tr("Monitoring is not supported."));
    }

    ///
    /// \brief Enables event monitoring for a node with an EventNotifier.
    /// \param nodeId Node to monitor for events.
    /// \param publishingInterval Publishing interval in milliseconds.
    ///
    virtual void subscribeEvents(const QString &nodeId, double publishingInterval)
    {
        Q_UNUSED(publishingInterval)
        emit eventMonitoringFinished(nodeId, true, false, tr("Event monitoring is not supported."));
    }

    ///
    /// \brief Disables event monitoring for a node.
    /// \param nodeId Node being monitored for events.
    ///
    virtual void unsubscribeEvents(const QString &nodeId)
    {
        emit eventMonitoringFinished(nodeId, false, false, tr("Event monitoring is not supported."));
    }

    ///
    /// \brief Resolves this client's session name from the server diagnostics.
    ///
    /// Backends that cannot read the server diagnostics may leave the default
    /// no-op, which resolves to an empty name.
    /// \param timeoutMs Request timeout in milliseconds.
    ///
    virtual void readServerSessionName(int timeoutMs) { Q_UNUSED(timeoutMs); }

    ///
    /// \brief Reads the server NamespaceArray, emitting namespacesReady() with the URIs.
    ///
    /// Backends that cannot read the namespace table may leave this default,
    /// which reports that the operation is unsupported.
    /// \param timeoutMs Request timeout in milliseconds.
    ///
    virtual void requestNamespaces(int timeoutMs)
    {
        Q_UNUSED(timeoutMs)
        emit namespacesReady({}, tr("Reading the namespace table is not supported."));
    }

    ///
    /// \brief Crawls the address space, emitting namespaceStatisticsReady() with per-namespace counts.
    ///
    /// Backends that cannot crawl the address space may leave this default,
    /// which reports that the operation is unsupported.
    /// \param timeoutMs Per-request timeout in milliseconds.
    ///
    virtual void requestNamespaceStatistics(int timeoutMs)
    {
        Q_UNUSED(timeoutMs)
        emit namespaceStatisticsReady({}, tr("Counting namespace nodes is not supported."));
    }

    ///
    /// \brief Cancels an in-progress namespace statistics crawl, if any.
    ///
    virtual void cancelNamespaceStatistics() {}

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
};
