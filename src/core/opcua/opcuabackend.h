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
    /// \brief Resolves this client's session name from the server diagnostics.
    ///
    /// Backends that cannot read the server diagnostics may leave the default
    /// no-op, which resolves to an empty name.
    /// \param timeoutMs Request timeout in milliseconds.
    ///
    virtual void readServerSessionName(int timeoutMs) { Q_UNUSED(timeoutMs); }

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
};
