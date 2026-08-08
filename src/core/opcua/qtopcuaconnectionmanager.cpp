// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include "qtopcuaconnectionmanager.h"

#include <algorithm>

#include <QCoreApplication>
#include <QLoggingCategory>
#include <QOpcUaApplicationIdentity>
#include <QOpcUaAuthenticationInformation>
#include <QOpcUaConnectionSettings>
#include <QOpcUaErrorState>
#include <QOpcUaPkiConfiguration>
#include <QOpcUaUserTokenPolicy>

#include "formatters/attributeformatter.h"
#include "certificatetrustdecider.h"
#include "loggingcategories.h"

using namespace OpcUaFormat;

namespace {

///
/// \brief Carries the stable QtOpcUaBackend translation context for this file.
///
/// The literals have to sit at the tr() call itself, otherwise lupdate cannot collect them.
///
class BackendText
{
    Q_DECLARE_TR_FUNCTIONS(QtOpcUaBackend)
};

///
/// \brief Builds the identity this client reports to servers when no certificate is configured.
/// \return Application identity naming this installation.
///
QOpcUaApplicationIdentity applicationIdentity()
{
    QOpcUaApplicationIdentity identity;
    identity.setApplicationName(PkiManager::applicationName());
    identity.setApplicationUri(PkiManager::applicationUri());
    identity.setProductUri(PkiManager::productUri());
    identity.setApplicationType(QOpcUaApplicationDescription::Client);
    return identity;
}

///
/// \brief Returns a user-facing description of a Qt OPC UA client error.
/// \param error Client error reported by QOpcUaClient.
/// \return Translated error description.
///
QString clientErrorName(QOpcUaClient::ClientError error)
{
    switch (error) {
    case QOpcUaClient::NoError:         return BackendText::tr("No error.");
    case QOpcUaClient::InvalidUrl:      return BackendText::tr("Invalid server URL.");
    case QOpcUaClient::AccessDenied:    return BackendText::tr("Access denied: authentication failed.");
    case QOpcUaClient::ConnectionError: return BackendText::tr("Connection error.");
    case QOpcUaClient::UnknownError:    return BackendText::tr("Unknown client error.");
    case QOpcUaClient::UnsupportedAuthenticationInformation:
        return BackendText::tr("Unsupported authentication information.");
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    case QOpcUaClient::InvalidAuthenticationInformation:
        return BackendText::tr("Invalid authentication information.");
    case QOpcUaClient::InvalidEndpointDescription:
        return BackendText::tr("Invalid endpoint description.");
    case QOpcUaClient::NoMatchingUserIdentityTokenFound:
        return BackendText::tr("No matching user identity token found.");
    case QOpcUaClient::UnsupportedSecurityPolicy:
        return BackendText::tr("Unsupported security policy.");
    case QOpcUaClient::InvalidPki:
        return BackendText::tr("Invalid PKI configuration.");
    case QOpcUaClient::CertificateUntrusted:
        return BackendText::tr("Certificate is not trusted.");
#endif
    }
    return BackendText::tr("Unknown client error (%1).").arg(static_cast<int>(error));
}

///
/// \brief Returns a user-facing name for a connection step.
/// \param step Connection step reported by the backend.
/// \return Translated step name.
///
QString connectionStepName(QOpcUaErrorState::ConnectionStep step)
{
    switch (step) {
    case QOpcUaErrorState::ConnectionStep::Unknown: return BackendText::tr("Unknown");
    case QOpcUaErrorState::ConnectionStep::CertificateValidation: return BackendText::tr("Certificate validation");
    case QOpcUaErrorState::ConnectionStep::OpenSecureChannel: return BackendText::tr("Open secure channel");
    case QOpcUaErrorState::ConnectionStep::CreateSession: return BackendText::tr("Create session");
    case QOpcUaErrorState::ConnectionStep::ActivateSession: return BackendText::tr("Activate session");
    }
    return BackendText::tr("Step %1").arg(static_cast<int>(step));
}

///
/// \brief Returns true when a client error means the server turned the credentials down.
/// \param error Client error reported by QOpcUaClient.
/// \return True when the error means the credentials were refused.
///
bool rejectsCredentials(QOpcUaClient::ClientError error)
{
    switch (error) {
    case QOpcUaClient::AccessDenied:
    case QOpcUaClient::UnsupportedAuthenticationInformation:
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    case QOpcUaClient::InvalidAuthenticationInformation:
    case QOpcUaClient::NoMatchingUserIdentityTokenFound:
#endif
        return true;
    default:
        return false;
    }
}

///
/// \brief Returns true when a connection step failed because the identity was refused.
/// \param status Status code of the failed connection step.
/// \return True when the status means the credentials were refused.
///
bool rejectsCredentials(QOpcUa::UaStatusCode status)
{
    switch (status) {
    case QOpcUa::UaStatusCode::BadUserAccessDenied:
    case QOpcUa::UaStatusCode::BadIdentityTokenInvalid:
    case QOpcUa::UaStatusCode::BadIdentityTokenRejected:
        return true;
    default:
        return false;
    }
}

///
/// \brief Returns true when an endpoint advertises the requested user-token type.
/// \param endpoint Endpoint description to inspect.
/// \param authentication Authentication method the connection profile asks for.
/// \return True when the endpoint offers a matching user identity token.
///
bool supportsAuthentication(const QOpcUaEndpointDescription &endpoint,
                            ConnectionProfile::Authentication authentication)
{
    QOpcUaUserTokenPolicy::TokenType tokenType = QOpcUaUserTokenPolicy::Anonymous;
    switch (authentication) {
    case ConnectionProfile::Authentication::Username:
        tokenType = QOpcUaUserTokenPolicy::Username;
        break;
    case ConnectionProfile::Authentication::Certificate:
        tokenType = QOpcUaUserTokenPolicy::Certificate;
        break;
    case ConnectionProfile::Authentication::Anonymous:
        tokenType = QOpcUaUserTokenPolicy::Anonymous;
        break;
    }
    for (const QOpcUaUserTokenPolicy &token : endpoint.userIdentityTokens())
        if (token.tokenType() == tokenType)
            return true;
    return false;
}

} // namespace

///
/// \brief Constructs an idle connection manager.
/// \param parent Owning QObject.
///
QtOpcUaConnectionManager::QtOpcUaConnectionManager(QObject *parent)
    : QObject(parent)
{
    _watchdog.setSingleShot(true);
    connect(&_watchdog, &QTimer::timeout, this, &QtOpcUaConnectionManager::handleConnectTimeout);
}

///
/// \brief Destroys the managed client.
///
QtOpcUaConnectionManager::~QtOpcUaConnectionManager()
{
    delete _client;
}

///
/// \brief Reports whether a Qt OPC UA backend is installed.
/// \return True when at least one backend is available.
///
bool QtOpcUaConnectionManager::isAvailable() const
{
    return !_provider.availableBackends().isEmpty();
}

///
/// \brief Returns the installed Qt OPC UA backend names.
/// \return Backend names reported by the Qt OPC UA provider.
///
QStringList QtOpcUaConnectionManager::availableBackends() const
{
    return _provider.availableBackends();
}

///
/// \brief Returns the current connection state.
/// \return Transport-neutral connection state.
///
OpcUaConnectionState QtOpcUaConnectionManager::state() const
{
    return _state;
}

///
/// \brief Returns the most recent connection error.
/// \return Error message, or an empty string when none was recorded.
///
QString QtOpcUaConnectionManager::lastError() const
{
    return _error;
}

///
/// \brief Returns the managed Qt OPC UA client.
/// \return Client instance, or nullptr while no backend has been created.
///
QOpcUaClient *QtOpcUaConnectionManager::client() const
{
    return _client;
}

///
/// \brief Returns the endpoints from the latest successful discovery.
/// \return Cached endpoint descriptions.
///
const QVector<QOpcUaEndpointDescription> &QtOpcUaConnectionManager::endpointDescriptions() const
{
    return _endpoints;
}

///
/// \brief Returns the DER-encoded certificate of the endpoint in use.
/// \return Server certificate, or an empty array outside a connection attempt.
///
QByteArray QtOpcUaConnectionManager::activeServerCertificate() const
{
    return _activeCertificate;
}

///
/// \brief Sets the server-certificate trust delegate.
/// \param decider Delegate deciding on an untrusted server certificate; nullptr rejects every one.
///
void QtOpcUaConnectionManager::setCertificateTrustDecider(CertificateTrustDecider *decider)
{
    _trustDecider = decider;
}

///
/// \brief Creates the requested backend for endpoint discovery.
/// \param backend Preferred backend name.
/// \return True when a client for the backend is ready.
///
bool QtOpcUaConnectionManager::prepareDiscovery(const QString &backend)
{
    return ensureClient(backend);
}

///
/// \brief Stores a discovery result and returns to disconnected state.
/// \param endpoints Endpoints reported by the discovery request.
///
void QtOpcUaConnectionManager::finishDiscovery(
    const QVector<QOpcUaEndpointDescription> &endpoints)
{
    _endpoints = endpoints;
    setState(OpcUaConnectionState::Disconnected);
}

///
/// \brief Returns to disconnected state, keeping the cached endpoints intact.
///
/// FindServers resolves servers rather than endpoints, so the endpoint list a pending
/// connectToEndpoint() still selects from must survive the request.
///
void QtOpcUaConnectionManager::finishServerLookup()
{
    setState(OpcUaConnectionState::Disconnected);
}

///
/// \brief Configures the client and connects to the profile's discovered endpoint.
/// \param profile Connection profile selecting the endpoint and carrying its settings.
/// \param password User password used by username authentication.
/// \param privateKeyPassword Client private key password; a non-empty one is rejected.
/// \return True when the connection attempt was started.
///
bool QtOpcUaConnectionManager::connectToEndpoint(const ConnectionProfile &profile,
                                                 const QString &password,
                                                 const QString &privateKeyPassword)
{
    if (!ensureClient(profile.backend))
        return false;
    if (!privateKeyPassword.isEmpty()) {
        setError(BackendText::tr("Encrypted private keys are not supported by Qt OPC UA."));
        return false;
    }
    const int index = endpointIndex(profile);
    if (index < 0) {
        setError(BackendText::tr("The selected endpoint is no longer available. Run discovery again."));
        return false;
    }
    configureClient(profile, password);
    _activeCertificate = _endpoints.at(index).serverCertificate();
    _watchdog.start(qMax(1000, profile.connectTimeoutMs));
    _client->connectToEndpoint(_endpoints.at(index));
    return true;
}

///
/// \brief Invalidates requests and disconnects the client.
///
void QtOpcUaConnectionManager::disconnectFromEndpoint()
{
    emit clientInvalidated();
    if (_client)
        _client->disconnectFromEndpoint();
}

///
/// \brief Updates the externally visible connection state.
/// \param state New connection state; an unchanged one emits no signal.
///
void QtOpcUaConnectionManager::setState(OpcUaConnectionState state)
{
    if (_state == state)
        return;
    _state = state;
    emit stateChanged(state);
}

///
/// \brief Records and reports a connection error.
/// \param message Error message written to the log and delivered to the user.
///
void QtOpcUaConnectionManager::setError(const QString &message)
{
    _error = message;
    qCWarning(lcClient) << message;
    emit errorOccurred(message);
}

///
/// \brief Maps a Qt client state transition to the transport-neutral state.
/// \param state Client state reported by QOpcUaClient.
///
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

///
/// \brief Reports a Qt client error when it is not NoError.
/// \param error Client error reported by QOpcUaClient.
///
void QtOpcUaConnectionManager::handleClientError(QOpcUaClient::ClientError error)
{
    if (error == QOpcUaClient::NoError)
        return;
    const QString name = clientErrorName(error);
    // The backend maps every status code except AccessDenied to UnknownError, but it always
    // reports the real one through connectError first. Keep that message instead of this one.
    if (_connectErrorReported)
        _connectErrorReported = false;
    else
        setError(BackendText::tr("OPC UA client error: %1").arg(name));
    if (rejectsCredentials(error))
        emit authenticationRejected(name);
}

///
/// \brief Applies certificate trust decisions or reports a connection-step failure.
/// \param state Error state of the failed step; its ignore flag carries the trust decision back.
///
void QtOpcUaConnectionManager::handleConnectError(QOpcUaErrorState *state)
{
    // open62541 runs the whole handshake inside one call, so it can only ever report the
    // Unknown step. Naming it adds nothing.
    QString message = state->connectionStep() == QOpcUaErrorState::ConnectionStep::Unknown
        ? BackendText::tr("Connection failed: %1").arg(statusName(state->errorCode()))
        : BackendText::tr("Connection step '%1' failed: %2")
              .arg(connectionStepName(state->connectionStep()), statusName(state->errorCode()));
    if (state->errorCode() == QOpcUa::UaStatusCode::BadCertificateInvalid) {
        message += BackendText::tr("\nThe server rejected the client certificate. Add this certificate "
                      "to the server trust list and retry: %1")
                       .arg(_activeClientCertificateFile);
    }
    if (state->connectionStep() != QOpcUaErrorState::ConnectionStep::CertificateValidation) {
        setError(message);
        _connectErrorReported = true;
        if (rejectsCredentials(state->errorCode()))
            emit authenticationRejected(statusName(state->errorCode()));
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

///
/// \brief Aborts a connection attempt after its watchdog expires.
///
void QtOpcUaConnectionManager::handleConnectTimeout()
{
    setError(BackendText::tr("The OPC UA connection timed out."));
    if (_client)
        _client->disconnectFromEndpoint();
}

///
/// \brief Creates or reuses a client for the requested backend.
/// \param backend Preferred backend name; "open62541" also matches a versioned plugin name.
/// \return True when a client for the backend is available.
///
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
        setError(BackendText::tr("The requested OPC UA backend '%1' is unavailable. Installed backends: %2")
                     .arg(backend, backends.join(QStringLiteral(", "))));
        setState(OpcUaConnectionState::Unavailable);
        return false;
    }
    _client = _provider.createClient(selected);
    if (!_client) {
        setError(BackendText::tr("Could not create the OPC UA backend '%1'.").arg(selected));
        setState(OpcUaConnectionState::Unavailable);
        return false;
    }
    _activeBackend = selected;
    connect(_client, &QOpcUaClient::stateChanged, this, &QtOpcUaConnectionManager::handleClientState);
    connect(_client, &QOpcUaClient::errorChanged, this, &QtOpcUaConnectionManager::handleClientError);
    connect(_client, &QOpcUaClient::connectError, this, &QtOpcUaConnectionManager::handleConnectError);
    return true;
}

///
/// \brief Replaces authentication, PKI and timeout settings for a connection profile.
/// \param profile Connection profile holding the credentials, certificates and timeouts.
/// \param password User password used by username authentication.
///
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
    _client->setApplicationIdentity(applicationIdentity());
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
        // The server matches the session's application URI against the one in the client
        // certificate, so an imported certificate dictates the identity we may present.
        const QString certificateUri =
            PkiManager::certificateApplicationUri(profile.clientCertificateFile);
        if (!certificateUri.isEmpty()) {
            QOpcUaApplicationIdentity identity = applicationIdentity();
            identity.setApplicationUri(certificateUri);
            _client->setApplicationIdentity(identity);
        }
        _activeClientCertificateFile = profile.clientCertificateFile;
    }
    QOpcUaConnectionSettings settings;
    settings.setSessionName(profile.sessionName);
    settings.setSessionTimeout(std::chrono::milliseconds(profile.sessionTimeoutMs));
    settings.setConnectTimeout(std::chrono::milliseconds(profile.connectTimeoutMs));
    settings.setSecureChannelLifeTime(std::chrono::milliseconds(profile.secureChannelLifetimeMs));
    settings.setRequestTimeout(std::chrono::milliseconds(profile.requestTimeoutMs));
    _client->setConnectionSettings(settings);
}

///
/// \brief Finds the discovered endpoint selected by a connection profile.
/// \param profile Connection profile naming the endpoint URL, security and authentication.
/// \return Index into the cached endpoints, or -1 when none of them matches.
///
int QtOpcUaConnectionManager::endpointIndex(const ConnectionProfile &profile) const
{
    for (int i = 0; i < _endpoints.size(); ++i) {
        const QOpcUaEndpointDescription &candidate = _endpoints.at(i);
        if (candidate.endpointUrl() == profile.endpointUrl
            && candidate.securityPolicy() == profile.securityPolicy
            && static_cast<int>(candidate.securityMode()) == profile.securityMode
            && supportsAuthentication(candidate, profile.authentication)) {
            return i;
        }
    }
    return -1;
}

///
/// \brief Clears certificate state that must not cross connection profiles.
///
void QtOpcUaConnectionManager::clearConnectionData()
{
    _activeCertificate.clear();
    _activeClientCertificateFile.clear();
    _connectErrorReported = false;
}
