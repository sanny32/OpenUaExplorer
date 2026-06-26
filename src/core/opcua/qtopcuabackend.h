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
    /// \brief Requests the server's endpoint list, emitting endpointsDiscovered() with the result.
    /// \param url Discovery URL (must be opc.tcp).
    /// \param backend Preferred backend.
    /// \param timeoutMs Discovery timeout in milliseconds.
    ///
    void discoverEndpoints(const QString &url, const QString &backend,
                           int timeoutMs) override;

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
    /// \param timeoutMs Request timeout in milliseconds.
    ///
    void browse(const QString &nodeId, int timeoutMs) override;

    ///
    /// \brief Browses a node's forward references, emitting referencesBrowseFinished().
    /// \param nodeId Node to browse.
    /// \param timeoutMs Request timeout in milliseconds.
    ///
    void browseReferences(const QString &nodeId, int timeoutMs) override;

    ///
    /// \brief Reads a node's full attribute set, emitting nodeDetailsReady() with the formatted result.
    /// \param nodeId Node to read.
    /// \param timeoutMs Request timeout in milliseconds.
    ///
    void readNode(const QString &nodeId, int timeoutMs) override;

    ///
    /// \brief Batch-reads the Value attribute of several nodes, emitting dataValuesReady().
    /// \param nodeIds Nodes whose Value attributes should be read.
    /// \param timeoutMs Request timeout in milliseconds.
    ///
    void readValues(const QStringList &nodeIds, int timeoutMs) override;

    ///
    /// \brief Reads the raw history of a node's Value, emitting historyDataReady() with the samples.
    /// \param nodeId Node whose history is read.
    /// \param start Inclusive range start.
    /// \param end Inclusive range end.
    /// \param numValuesPerNode Maximum samples to return, or 0 for no limit.
    /// \param timeoutMs Request timeout in milliseconds.
    ///
    void readHistoryRaw(const QString &nodeId, const QDateTime &start, const QDateTime &end,
                        quint32 numValuesPerNode, int timeoutMs) override;

    ///
    /// \brief Writes a node's Value attribute, emitting writeFinished() with the outcome.
    /// \param nodeId Node to write.
    /// \param value Typed value.
    /// \param valueType QOpcUa::Types numeric value or Undefined.
    /// \param timeoutMs Request timeout in milliseconds.
    ///
    void writeValue(const QString &nodeId, const QVariant &value,
                    int valueType, int timeoutMs) override;

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
    /// \param timeoutMs Request timeout in milliseconds.
    ///
    void readServerSessionName(int timeoutMs) override;

private:
    class Private;
    Private *_d;
};
