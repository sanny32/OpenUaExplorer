// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>
#include <QTimer>

#include <QOpcUaClient>
#include <QOpcUaEndpointDescription>
#include <QOpcUaProvider>

#include "connectionprofile.h"
#include "opcuatypes.h"
#include "pkimanager.h"

class CertificateTrustDecider;

///
/// \brief Owns the Qt OPC UA client and its connection-specific state.
///
class QtOpcUaConnectionManager : public QObject
{
    Q_OBJECT

public:
    /// \brief Constructs an idle connection manager.
    explicit QtOpcUaConnectionManager(QObject *parent = nullptr);

    /// \brief Destroys the managed client.
    ~QtOpcUaConnectionManager() override;

    /// \brief Reports whether a Qt OPC UA backend is installed.
    bool isAvailable() const;

    /// \brief Returns the installed Qt OPC UA backend names.
    QStringList availableBackends() const;

    /// \brief Returns the current connection state.
    OpcUaConnectionState state() const;

    /// \brief Returns the most recent connection error.
    QString lastError() const;

    /// \brief Returns the managed Qt OPC UA client.
    QOpcUaClient *client() const;

    /// \brief Returns the endpoints from the latest successful discovery.
    const QVector<QOpcUaEndpointDescription> &endpointDescriptions() const;

    /// \brief Returns the DER-encoded certificate of the endpoint in use, or empty.
    QByteArray activeServerCertificate() const;

    /// \brief Sets the server-certificate trust delegate.
    void setCertificateTrustDecider(CertificateTrustDecider *decider);

    /// \brief Creates the requested backend for endpoint discovery.
    bool prepareDiscovery(const QString &backend);

    /// \brief Stores a discovery result and returns to disconnected state.
    void finishDiscovery(const QVector<QOpcUaEndpointDescription> &endpoints);

    /// \brief Configures the client and connects to the profile's discovered endpoint.
    bool connectToEndpoint(const ConnectionProfile &profile, const QString &password,
                           const QString &privateKeyPassword);

    /// \brief Invalidates requests and disconnects the client.
    void disconnectFromEndpoint();

    /// \brief Updates the externally visible connection state.
    void setState(OpcUaConnectionState state);
    
    /// \brief Records and reports a connection error.
    void setError(const QString &message);

signals:
    void stateChanged(OpcUaConnectionState state);
    void errorOccurred(QString message);
    void clientInvalidated();

private slots:
    void handleClientState(QOpcUaClient::ClientState state);
    void handleClientError(QOpcUaClient::ClientError error);
    void handleConnectError(QOpcUaErrorState *state);
    void handleConnectTimeout();

private:
    bool ensureClient(const QString &backend);
    void configureClient(const ConnectionProfile &profile, const QString &password);
    int endpointIndex(const ConnectionProfile &profile) const;
    void clearConnectionData();

    OpcUaConnectionState _state = OpcUaConnectionState::Disconnected;
    QString _error;
    QTimer _watchdog;
    PkiManager _pki;
    QOpcUaProvider _provider;
    QOpcUaClient *_client = nullptr;
    QString _activeBackend;
    QVector<QOpcUaEndpointDescription> _endpoints;
    QByteArray _activeCertificate;
    QString _activeClientCertificateFile;
    CertificateTrustDecider *_trustDecider = nullptr;
};
