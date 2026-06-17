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
    /// \brief Writes a value to a node using the cached request timeout.
    /// \param nodeId Target node.
    /// \param value Value to write.
    /// \param valueType OPC UA type of the value.
    ///
    void writeValue(const QString &nodeId, const QVariant &value, int valueType);

    ///
    /// \brief Resolves this client's session name from the server diagnostics.
    ///
    void readServerSessionName();

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
    /// \brief Emitted when a write finishes.
    /// \param nodeId Written node.
    /// \param success Whether the write succeeded.
    /// \param error Error description, empty on success.
    ///
    void writeFinished(QString nodeId, bool success, QString error);

    ///
    /// \brief Emitted when the server-assigned session name has been resolved.
    /// \param sessionName Resolved session name, empty when unavailable.
    ///
    void serverSessionNameResolved(QString sessionName);
private:
    OpcUaBackend *_backend;
    bool _ownsBackend;
    int _requestTimeoutMs = 15000;
};
