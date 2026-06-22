// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include "qtopcuaconnectionmanager.h"

#include <algorithm>

#include <QCoreApplication>
#include <QLoggingCategory>
#include <QOpcUaApplicationIdentity>
#include <QOpcUaAuthenticationInformation>
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
#include <QOpcUaConnectionSettings>
#endif
#include <QOpcUaErrorState>
#include <QOpcUaPkiConfiguration>

#include "attributeformatter.h"
#include "certificatetrustdecider.h"
#include "loggingcategories.h"

using namespace OpcUaFormat;

namespace {

/// \brief Translates user-facing text in the stable QtOpcUaBackend context.
QString backendTr(const char *text)
{
    return QCoreApplication::translate("QtOpcUaBackend", text);
}

/// \brief Returns a user-facing description of a Qt OPC UA client error.
QString clientErrorName(QOpcUaClient::ClientError error)
{
    switch (error) {
    case QOpcUaClient::NoError:         return backendTr("No error.");
    case QOpcUaClient::InvalidUrl:      return backendTr("Invalid server URL.");
    case QOpcUaClient::AccessDenied:    return backendTr("Access denied: authentication failed.");
    case QOpcUaClient::ConnectionError: return backendTr("Connection error.");
    case QOpcUaClient::UnknownError:    return backendTr("Unknown client error.");
    }
    return backendTr("Unknown client error (%1).").arg(static_cast<int>(error));
}

/// \brief Returns a user-facing name for a connection step.
QString connectionStepName(QOpcUaErrorState::ConnectionStep step)
{
    switch (step) {
    case QOpcUaErrorState::ConnectionStep::Unknown: return backendTr("Unknown");
    case QOpcUaErrorState::ConnectionStep::CertificateValidation: return backendTr("Certificate validation");
    case QOpcUaErrorState::ConnectionStep::OpenSecureChannel: return backendTr("Open secure channel");
    case QOpcUaErrorState::ConnectionStep::CreateSession: return backendTr("Create session");
    case QOpcUaErrorState::ConnectionStep::ActivateSession: return backendTr("Activate session");
    }
    return backendTr("Step %1").arg(static_cast<int>(step));
}

} // namespace

/// \brief Constructs an idle connection manager.
QtOpcUaConnectionManager::QtOpcUaConnectionManager(QObject *parent)
    : QObject(parent)
{
    _watchdog.setSingleShot(true);
    connect(&_watchdog, &QTimer::timeout, this, &QtOpcUaConnectionManager::handleConnectTimeout);
}

/// \brief Destroys the managed client.
QtOpcUaConnectionManager::~QtOpcUaConnectionManager()
{
    delete _client;
}

/// \brief Reports whether a Qt OPC UA backend is installed.
bool QtOpcUaConnectionManager::isAvailable() const
{
    return !_provider.availableBackends().isEmpty();
}

/// \brief Returns the installed Qt OPC UA backend names.
QStringList QtOpcUaConnectionManager::availableBackends() const
{
    return _provider.availableBackends();
}

/// \brief Returns the current connection state.
OpcUaConnectionState QtOpcUaConnectionManager::state() const
{
    return _state;
}

/// \brief Returns the most recent connection error.
QString QtOpcUaConnectionManager::lastError() const
{
    return _error;
}

/// \brief Returns the managed Qt OPC UA client.
QOpcUaClient *QtOpcUaConnectionManager::client() const
{
    return _client;
}

/// \brief Returns the endpoints from the latest successful discovery.
const QVector<QOpcUaEndpointDescription> &QtOpcUaConnectionManager::endpointDescriptions() const
{
    return _endpoints;
}

/// \brief Sets the server-certificate trust delegate.
void QtOpcUaConnectionManager::setCertificateTrustDecider(CertificateTrustDecider *decider)
{
    _trustDecider = decider;
}

/// \brief Creates the requested backend for endpoint discovery.
bool QtOpcUaConnectionManager::prepareDiscovery(const QString &backend)
{
    return ensureClient(backend);
}

/// \brief Stores a discovery result and returns to disconnected state.
void QtOpcUaConnectionManager::finishDiscovery(
    const QVector<QOpcUaEndpointDescription> &endpoints)
{
    _endpoints = endpoints;
    setState(OpcUaConnectionState::Disconnected);
}

/// \brief Configures the client and connects to the profile's discovered endpoint.
bool QtOpcUaConnectionManager::connectToEndpoint(const ConnectionProfile &profile,
                                                 const QString &password,
                                                 const QString &privateKeyPassword)
{
    if (!ensureClient(profile.backend))
        return false;
    if (!privateKeyPassword.isEmpty()) {
        setError(backendTr("Encrypted private keys are not supported by Qt OPC UA."));
        return false;
    }
    const int index = endpointIndex(profile);
    if (index < 0) {
        setError(backendTr("The selected endpoint is no longer available. Run discovery again."));
        return false;
    }
    configureClient(profile, password);
    _activeCertificate = _endpoints.at(index).serverCertificate();
    _watchdog.start(qMax(1000, profile.connectTimeoutMs));
    _client->connectToEndpoint(_endpoints.at(index));
    return true;
}

/// \brief Invalidates requests and disconnects the client.
void QtOpcUaConnectionManager::disconnectFromEndpoint()
{
    emit clientInvalidated();
    if (_client)
        _client->disconnectFromEndpoint();
}

/// \brief Updates the externally visible connection state.
void QtOpcUaConnectionManager::setState(OpcUaConnectionState state)
{
    if (_state == state)
        return;
    _state = state;
    emit stateChanged(state);
}

/// \brief Records and reports a connection error.
void QtOpcUaConnectionManager::setError(const QString &message)
{
    _error = message;
    qCWarning(lcClient) << message;
    emit errorOccurred(message);
}

/// \brief Maps a Qt client state transition to the transport-neutral state.
void QtOpcUaConnectionManager::handleClientState(QOpcUaClient::ClientState state)
{
    switch (state) {
    case QOpcUaClient::Disconnected:
        _watchdog.stop();
        emit clientInvalidated();
        setState(OpcUaConnectionState::Disconnected);
        break;
    case QOpcUaClient::Connecting: setState(OpcUaConnectionState::Connecting); break;
    case QOpcUaClient::Connected:
        _watchdog.stop();
        setState(OpcUaConnectionState::Connected);
        break;
    case QOpcUaClient::Closing: setState(OpcUaConnectionState::Closing); break;
    }
}

/// \brief Reports a Qt client error when it is not NoError.
void QtOpcUaConnectionManager::handleClientError(QOpcUaClient::ClientError error)
{
    if (error != QOpcUaClient::NoError)
        setError(backendTr("OPC UA client error: %1").arg(clientErrorName(error)));
}

/// \brief Applies certificate trust decisions or reports a connection-step failure.
void QtOpcUaConnectionManager::handleConnectError(QOpcUaErrorState *state)
{
    QString message = backendTr("Connection step '%1' failed: %2")
        .arg(connectionStepName(state->connectionStep()), statusName(state->errorCode()));
    if (state->errorCode() == QOpcUa::UaStatusCode::BadCertificateInvalid) {
        message += backendTr("\nThe server rejected the client certificate. Add this certificate "
                      "to the server trust list and retry: %1")
                       .arg(_activeClientCertificateFile);
    }
    if (state->connectionStep() != QOpcUaErrorState::ConnectionStep::CertificateValidation) {
        setError(message);
        return;
    }
    const CertificateTrustDecision decision = _trustDecider
        ? _trustDecider->decide(_activeCertificate, message)
        : CertificateTrustDecision::Reject;
    if (decision == CertificateTrustDecision::TrustPermanently) {
        QString error;
        if (!_pki.trustServerCertificate(_activeCertificate, &error)) {
            setError(error);
            state->setIgnoreError(false);
            return;
        }
    }
    state->setIgnoreError(decision != CertificateTrustDecision::Reject);
}

/// \brief Aborts a connection attempt after its watchdog expires.
void QtOpcUaConnectionManager::handleConnectTimeout()
{
    setError(backendTr("The OPC UA connection timed out."));
    if (_client)
        _client->disconnectFromEndpoint();
}

/// \brief Creates or reuses a client for the requested backend.
bool QtOpcUaConnectionManager::ensureClient(const QString &backend)
{
    if (_client && _activeBackend == backend)
        return true;
    if (_client) {
        emit clientInvalidated();
        delete _client;
        _client = nullptr;
        clearConnectionData();
        _endpoints.clear();
        _activeBackend.clear();
    }
    const QStringList backends = _provider.availableBackends();
    QString selected = backend;
    if (!backends.contains(selected) && selected == QLatin1String("open62541")) {
        const auto match = std::find_if(backends.cbegin(), backends.cend(), [](const QString &name) {
            return name.contains(QLatin1String("open62541"), Qt::CaseInsensitive);
        });
        if (match != backends.cend())
            selected = *match;
    }
    if (!backends.contains(selected)) {
        setError(backendTr("The requested OPC UA backend '%1' is unavailable. Installed backends: %2")
                     .arg(backend, backends.join(QStringLiteral(", "))));
        setState(OpcUaConnectionState::Unavailable);
        return false;
    }
    _client = _provider.createClient(selected);
    if (!_client) {
        setError(backendTr("Could not create the OPC UA backend '%1'.").arg(selected));
        setState(OpcUaConnectionState::Unavailable);
        return false;
    }
    _activeBackend = selected;
    connect(_client, &QOpcUaClient::stateChanged, this, &QtOpcUaConnectionManager::handleClientState);
    connect(_client, &QOpcUaClient::errorChanged, this, &QtOpcUaConnectionManager::handleClientError);
    connect(_client, &QOpcUaClient::connectError, this, &QtOpcUaConnectionManager::handleConnectError);
    return true;
}

/// \brief Replaces authentication, PKI and timeout settings for a connection profile.
void QtOpcUaConnectionManager::configureClient(const ConnectionProfile &profile,
                                                const QString &password)
{
    clearConnectionData();
    QOpcUaAuthenticationInformation authentication;
    switch (profile.authentication) {
    case ConnectionProfile::Authentication::Username:
        authentication.setUsernameAuthentication(profile.username, password);
        break;
    case ConnectionProfile::Authentication::Certificate:
        authentication.setCertificateAuthentication();
        break;
    case ConnectionProfile::Authentication::Anonymous:
        authentication.setAnonymousAuthentication();
        break;
    }
    _client->setAuthenticationInformation(authentication);
    _client->setPkiConfiguration(QOpcUaPkiConfiguration());
    _client->setApplicationIdentity(QOpcUaApplicationIdentity());
    if (!profile.clientCertificateFile.isEmpty()) {
        QString error;
        _pki.ensureDirectories(&error);
        const PkiManager::Paths paths = _pki.paths();
        QOpcUaPkiConfiguration configuration;
        configuration.setClientCertificateFile(profile.clientCertificateFile);
        configuration.setPrivateKeyFile(profile.privateKeyFile);
        configuration.setTrustListDirectory(paths.trustedCertificates);
        configuration.setRevocationListDirectory(paths.trustedCrl);
        configuration.setIssuerListDirectory(paths.issuerCertificates);
        configuration.setIssuerRevocationListDirectory(paths.issuerCrl);
        _client->setPkiConfiguration(configuration);
        if (configuration.isKeyAndCertificateFileSet())
            _client->setApplicationIdentity(configuration.applicationIdentity());
        _activeClientCertificateFile = profile.clientCertificateFile;
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    QOpcUaConnectionSettings settings;
    settings.setSessionTimeout(std::chrono::milliseconds(profile.sessionTimeoutMs));
    settings.setConnectTimeout(std::chrono::milliseconds(profile.connectTimeoutMs));
    settings.setSecureChannelLifeTime(std::chrono::milliseconds(profile.secureChannelLifetimeMs));
    settings.setRequestTimeout(std::chrono::milliseconds(profile.requestTimeoutMs));
    _client->setConnectionSettings(settings);
#endif
}

/// \brief Finds the discovered endpoint selected by a connection profile.
int QtOpcUaConnectionManager::endpointIndex(const ConnectionProfile &profile) const
{
    for (int i = 0; i < _endpoints.size(); ++i) {
        const QOpcUaEndpointDescription &candidate = _endpoints.at(i);
        if (candidate.endpointUrl() == profile.endpointUrl
            && candidate.securityPolicy() == profile.securityPolicy
            && static_cast<int>(candidate.securityMode()) == profile.securityMode) {
            return i;
        }
    }
    return -1;
}

/// \brief Clears certificate state that must not cross connection profiles.
void QtOpcUaConnectionManager::clearConnectionData()
{
    _activeCertificate.clear();
    _activeClientCertificateFile.clear();
}
