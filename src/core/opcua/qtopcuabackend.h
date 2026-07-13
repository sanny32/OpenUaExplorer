// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "opcuabackend.h"

///
/// \brief OPC UA backend implemented on top of the Qt OPC UA module.
///
class QtOpcUaBackend : public OpcUaBackend
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs the backend and registers its transferable metatypes.
    /// \param parent Parent object.
    ///
    explicit QtOpcUaBackend(QObject *parent = nullptr);

    ///
    /// \brief Destroys the backend, tearing down the client and private state.
    ///
    ~QtOpcUaBackend() override;

    ///
    /// \brief Reports whether at least one Qt OPC UA backend plugin is installed.
    /// \return True when Qt OpcUa and at least one backend are available.
    ///
    bool isAvailable() const override;

    ///
    /// \brief Lists the installed Qt OPC UA backend plugins.
    /// \return Installed Qt OPC UA backend names.
    ///
    QStringList availableBackends() const override;

    ///
    /// \brief Returns the current connection state.
    /// \return Current connection state.
    ///
    OpcUaConnectionState state() const override;

    ///
    /// \brief Returns the most recent error reported by the backend.
    /// \return Most recent service error.
    ///
    QString lastError() const override;

    ///
    /// \brief Sets the delegate consulted during server-certificate validation.
    /// \param decider Trust decider, or nullptr to reject untrusted certificates.
    ///
    void setCertificateTrustDecider(CertificateTrustDecider *decider) override;

    ///
    /// \brief Returns the DER-encoded certificate of the endpoint in use, or empty.
    /// \return DER-encoded server certificate, or an empty array.
    ///
    QByteArray activeServerCertificate() const override;

    ///
    /// \brief Requests the server's endpoint list, emitting endpointsDiscovered() with the result.
    /// \param url Discovery URL (must be opc.tcp).
    /// \param backend Preferred backend.
    /// \param timeoutMs Discovery timeout in milliseconds.
    ///
    void discoverEndpoints(const QString &url, const QString &backend,
                           int timeoutMs) override;

    ///
    /// \brief Lists the servers registered with a discovery server, emitting serversDiscovered().
    /// \param url Discovery server URL (must be opc.tcp).
    /// \param backend Preferred backend.
    /// \param timeoutMs Request timeout in milliseconds.
    ///
    void findServers(const QString &url, const QString &backend, int timeoutMs) override;

    ///
    /// \brief Connects to the discovered endpoint that matches the profile's URL, policy, and mode.
    /// \param profile Connection settings.
    /// \param password Username password.
    /// \param privateKeyPassword Private key password (unsupported; non-empty values are rejected).
    ///
    void connectToEndpoint(const ConnectionProfile &profile,
                           const QString &password,
                           const QString &privateKeyPassword) override;

    ///
    /// \brief Cancels pending requests and disconnects from the endpoint.
    ///
    void disconnectFromEndpoint() override;

    ///
    /// \brief Browses a node's children, emitting browseFinished() with the references or an error.
    /// \param nodeId Node to browse.
    ///
    void browse(const QString &nodeId) override;

    ///
    /// \brief Browses a node's forward references, emitting referencesBrowseFinished().
    /// \param nodeId Node to browse.
    ///
    void browseReferences(const QString &nodeId) override;

    ///
    /// \brief Reads a node's full attribute set, emitting nodeDetailsReady() with the formatted result.
    /// \param nodeId Node to read.
    ///
    void readNode(const QString &nodeId) override;

    ///
    /// \brief Batch-reads the Value attribute of several nodes, emitting dataValuesReady().
    /// \param nodeIds Nodes whose Value attributes should be read.
    ///
    void readValues(const QStringList &nodeIds) override;

    ///
    /// \brief Reads the raw history of a node's Value, emitting historyDataReady() with the samples.
    /// \param nodeId Node whose history is read.
    /// \param start Inclusive range start.
    /// \param end Inclusive range end.
    /// \param numValuesPerNode Maximum samples to return, or 0 for no limit.
    ///
    void readHistoryRaw(const QString &nodeId, const QDateTime &start, const QDateTime &end,
                        quint32 numValuesPerNode) override;

    ///
    /// \brief Reads historical events for a node, emitting historyEventsReady().
    /// \param nodeId Node whose event history is read.
    /// \param start Inclusive range start.
    /// \param end Inclusive range end.
    /// \param numValuesPerNode Maximum events to return, or 0 for no limit.
    ///
    void readHistoryEvents(const QString &nodeId, const QDateTime &start, const QDateTime &end,
                           quint32 numValuesPerNode) override;

    ///
    /// \brief Writes a node's Value attribute, emitting writeFinished() with the outcome.
    /// \param nodeId Node to write.
    /// \param value Typed value.
    /// \param valueType QOpcUa::Types numeric value or Undefined.
    ///
    void writeValue(const QString &nodeId, const QVariant &value, int valueType) override;

    ///
    /// \brief Reads a method's InputArguments/OutputArguments, emitting methodInfoReady().
    /// \param methodNodeId Method node whose argument metadata is read.
    ///
    void readMethodInfo(const QString &methodNodeId) override;

    ///
    /// \brief Calls a method on its owning object, emitting methodCallFinished() with the outputs.
    /// \param objectNodeId Object node that owns the method.
    /// \param methodNodeId Method node to call.
    /// \param args Input argument values in call order.
    /// \param argTypes QOpcUa::Types numeric values matching \a args positionally.
    ///
    void callMethod(const QString &objectNodeId, const QString &methodNodeId,
                    const QVariantList &args, const QList<int> &argTypes) override;

    ///
    /// \brief Enables Value monitoring for a node.
    /// \param nodeId Node to monitor.
    /// \param publishingInterval Publishing interval in milliseconds.
    ///
    void subscribe(const QString &nodeId, double publishingInterval) override;

    ///
    /// \brief Disables Value monitoring for a node.
    /// \param nodeId Monitored node.
    ///
    void unsubscribe(const QString &nodeId) override;

    ///
    /// \brief Enables event monitoring for a node with an EventNotifier.
    /// \param nodeId Node to monitor for events.
    /// \param publishingInterval Publishing interval in milliseconds.
    ///
    void subscribeEvents(const QString &nodeId, double publishingInterval) override;

    ///
    /// \brief Disables event monitoring for a node.
    /// \param nodeId Node being monitored for events.
    ///
    void unsubscribeEvents(const QString &nodeId) override;

    ///
    /// \brief Reads the SessionDiagnosticsArray and resolves this client's session name.
    ///
    void readServerSessionName() override;

    ///
    /// \brief Reads the server NamespaceArray, emitting namespacesReady() with the URIs.
    ///
    void requestNamespaces() override;

    ///
    /// \brief Crawls the address space, emitting namespaceStatisticsReady() with per-namespace counts.
    ///
    void requestNamespaceStatistics() override;

    ///
    /// \brief Cancels an in-progress namespace statistics crawl, if any.
    ///
    void cancelNamespaceStatistics() override;

    ///
    /// \brief Searches a subtree for a display name, emitting nodeSearchFinished() with the match.
    /// \param startNodeId Node whose subtree is searched.
    /// \param pattern Case-insensitive substring matched against display names.
    ///
    void searchNode(const QString &startNodeId, const QString &pattern) override;

    ///
    /// \brief Cancels an in-progress node search, if any.
    ///
    void cancelNodeSearch() override;

private:
    ///
    /// \brief Reads EventNotifier/Historizing of browsed children, then emits browseFinished().
    /// \param parentNodeId Browsed node whose children are being delivered.
    /// \param nodes Browsed children to enrich with event-source and history-read capability.
    /// \param timeoutMs Request timeout in milliseconds.
    ///
    /// Browse references omit EventNotifier (Object) and Historizing (Variable), so a follow-up
    /// batch read fills them in. Enrichment is best-effort: the children are still delivered if
    /// the read fails or times out.
    ///
    void enrichAndFinishBrowse(const QString &parentNodeId,
                               QVector<OpcUaNodeInfo> nodes, int timeoutMs);

    ///
    /// \brief Reads the Value of the located argument-property nodes and emits methodInfoReady().
    /// \param methodNodeId Method whose metadata is being resolved.
    /// \param inputArgumentsNodeId InputArguments property NodeId, or empty when absent.
    /// \param outputArgumentsNodeId OutputArguments property NodeId, or empty when absent.
    /// \param timeoutMs Request timeout in milliseconds.
    ///
    void readMethodArgumentValues(const QString &methodNodeId,
                                  const QString &inputArgumentsNodeId,
                                  const QString &outputArgumentsNodeId, int timeoutMs);

    class Private;
    Private *_d;
};
