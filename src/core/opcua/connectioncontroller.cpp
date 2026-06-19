// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include "connectioncontroller.h"

#include <QDateTime>

#include "connectionprofilestore.h"
#include "opcuaclientservice.h"
#include "recentconnectionstore.h"

///
/// \brief Constructs the controller owning freshly created client, secret, profile,
///        and recent-connection stores.
/// \param parent Owning QObject.
///
ConnectionController::ConnectionController(QObject *parent)
    : ConnectionController(new OpcUaClientService,
                           new SecretStore,
                           new ConnectionProfileStore,
                           new RecentConnectionStore,
                           parent)
{
    _clientService->setParent(this);
    _secretStore->setParent(this);
    _ownsDependencies = true;
}

///
/// \brief Constructs the controller with injected dependencies, used for testing.
/// \param clientService OPC UA client service.
/// \param secretStore Secret store for profile passwords.
/// \param profileStore Persistent profile store.
/// \param recentStore Persistent recent-connection store.
/// \param parent Owning QObject.
///
ConnectionController::ConnectionController(OpcUaClientService *clientService,
                                           SecretStore *secretStore,
                                           ConnectionProfileStore *profileStore,
                                           RecentConnectionStore *recentStore,
                                           QObject *parent)
    : QObject(parent)
    , _clientService(clientService)
    , _secretStore(secretStore)
    , _profileStore(profileStore)
    , _recentStore(recentStore)
    , _ownsDependencies(false)
{
    Q_ASSERT(_clientService);
    Q_ASSERT(_secretStore);
    Q_ASSERT(_profileStore);
    Q_ASSERT(_recentStore);
    connect(_secretStore, &SecretStore::readFinished,
            this, &ConnectionController::handleSecretRead);
    connect(_clientService, &OpcUaClientService::endpointsDiscovered,
            this, &ConnectionController::handleEndpoints);
}

///
/// \brief Destroys the controller, deleting the owned stores only when self-created.
///
ConnectionController::~ConnectionController()
{
    if (_ownsDependencies) {
        delete _profileStore;
        delete _recentStore;
    }
}

///
/// \brief Gives access to the underlying client service.
/// \return The OPC UA client service.
///
OpcUaClientService *ConnectionController::clientService() const
{
    return _clientService;
}

///
/// \brief Returns the saved connection profiles.
/// \return All persisted profiles.
///
QList<ConnectionProfile> ConnectionController::profiles() const
{
    return _profileStore->profiles();
}

///
/// \brief Returns the most recent connections, most-recent first.
/// \return Recent connection profiles.
///
QList<ConnectionProfile> ConnectionController::recentConnections() const
{
    return _recentStore->connections();
}

///
/// \brief Returns the profile of the current or most recent connection attempt.
/// \return The active profile.
///
const ConnectionProfile &ConnectionController::activeProfile() const
{
    return _activeProfile;
}

///
/// \brief Sets the delegate that decides whether to trust a server certificate.
/// \param decider Trust decider, forwarded to the client service.
///
void ConnectionController::setCertificateTrustDecider(CertificateTrustDecider *decider)
{
    _clientService->setCertificateTrustDecider(decider);
}

///
/// \brief Connects immediately using credentials supplied by the user (no stored-secret lookup).
/// \param profile Profile to connect with.
/// \param password User password, if any.
/// \param privateKeyPassword Private-key password, if any.
///
void ConnectionController::connectNewProfile(const ConnectionProfile &profile,
                                             const QString &password,
                                             const QString &privateKeyPassword)
{
    _waitingForDiscovery = false;
    _activeProfile = profile;
    _recentStore->record(profile);
    emit recentsChanged();
    touchFavorite(profile);
    _clientService->connectToEndpoint(profile, password, privateKeyPassword);
}

///
/// \brief Connects a saved profile, first loading any required secrets from the keychain.
/// \param profile Saved profile to connect with.
///
void ConnectionController::connectSavedProfile(const ConnectionProfile &profile)
{
    _pendingProfile = profile;
    _pendingPassword.clear();
    _pendingPrivateKeyPassword.clear();
    _pendingSecretReads = 0;
    _waitingForDiscovery = false;

    _recentStore->record(profile);
    emit recentsChanged();
    touchFavorite(profile);

    const bool needsPassword =
        profile.authentication == ConnectionProfile::Authentication::Username;
    const bool needsPrivateKeyPassword = !profile.privateKeyFile.isEmpty();
    _pendingSecretReads = static_cast<int>(needsPassword)
        + static_cast<int>(needsPrivateKeyPassword);

    if (needsPassword) {
        _secretStore->read(profile.id, SecretStore::Secret::Password);
    }
    if (needsPrivateKeyPassword) {
        _secretStore->read(profile.id, SecretStore::Secret::PrivateKeyPassword);
    }
    if (!needsPassword && !needsPrivateKeyPassword)
        discoverPendingProfile();
}

///
/// \brief Persists a profile and its secrets, emitting profilesChanged() on success.
/// \param profile Profile to store.
/// \param password User password to store, if non-empty.
/// \param privateKeyPassword Private-key password to store, if non-empty.
///
void ConnectionController::saveProfile(const ConnectionProfile &profile,
                                       const QString &password,
                                       const QString &privateKeyPassword)
{
    const QList<ConnectionProfile> existing = _profileStore->profiles();
    for (const ConnectionProfile &other : existing) {
        if (other.endpointUrl == profile.endpointUrl && other.id != profile.id)
            forgetProfile(other.id);
    }

    if (!_profileStore->save(profile)) {
        emit errorOccurred(tr("Could not save the connection profile."));
        return;
    }
    if (!password.isEmpty())
        _secretStore->write(profile.id, SecretStore::Secret::Password, password);
    if (!privateKeyPassword.isEmpty()) {
        _secretStore->write(profile.id, SecretStore::Secret::PrivateKeyPassword,
                            privateKeyPassword);
    }
    emit profilesChanged();
}

///
/// \brief Removes any saved profile matching an endpoint URL, along with its secrets.
/// \param endpointUrl Endpoint URL whose favourites should be removed.
///
void ConnectionController::removeFavorite(const QString &endpointUrl)
{
    bool removed = false;
    const QList<ConnectionProfile> existing = _profileStore->profiles();
    for (const ConnectionProfile &profile : existing) {
        if (profile.endpointUrl == endpointUrl) {
            forgetProfile(profile.id);
            removed = true;
        }
    }
    if (removed)
        emit profilesChanged();
}

///
/// \brief Deletes a stored profile and its secrets without emitting change notifications.
/// \param id Profile identifier.
///
void ConnectionController::forgetProfile(const QString &id)
{
    _profileStore->remove(id);
    _secretStore->remove(id, SecretStore::Secret::Password);
    _secretStore->remove(id, SecretStore::Secret::PrivateKeyPassword);
}

///
/// \brief Stamps the matching saved favourite with the current time as its last use.
/// \param profile Profile being connected; matched against favourites by endpoint URL.
///
void ConnectionController::touchFavorite(const ConnectionProfile &profile)
{
    if (profile.endpointUrl.isEmpty())
        return;

    const QList<ConnectionProfile> existing = _profileStore->profiles();
    for (ConnectionProfile favorite : existing) {
        if (favorite.endpointUrl != profile.endpointUrl)
            continue;
        favorite.lastUsed = QDateTime::currentDateTime();
        if (_profileStore->save(favorite))
            emit profilesChanged();
        break;
    }
}

///
/// \brief Collects a loaded secret and starts discovery once all pending reads complete.
/// \param profileId Profile the secret belongs to.
/// \param secret Which secret was read.
/// \param value Secret value.
/// \param error Read error, if any.
///
void ConnectionController::handleSecretRead(const QString &profileId,
                                            SecretStore::Secret secret,
                                            const QString &value,
                                            const QString &error)
{
    if (profileId != _pendingProfile.id || _pendingSecretReads == 0)
        return;
    if (!error.isEmpty())
        emit errorOccurred(error);
    if (secret == SecretStore::Secret::Password)
        _pendingPassword = value;
    else
        _pendingPrivateKeyPassword = value;
    if (--_pendingSecretReads == 0)
        discoverPendingProfile();
}

///
/// \brief After discovery succeeds for a pending profile, connects to its endpoint.
/// \param error Discovery error, if any.
///
void ConnectionController::handleEndpoints(const QList<EndpointInfo> &,
                                           const QString &error)
{
    if (!_waitingForDiscovery)
        return;
    _waitingForDiscovery = false;
    if (!error.isEmpty()) {
        emit errorOccurred(error);
        return;
    }
    _activeProfile = _pendingProfile;
    _clientService->connectToEndpoint(_pendingProfile, _pendingPassword,
                                      _pendingPrivateKeyPassword);
}

///
/// \brief Starts endpoint discovery for the pending profile and arms the discovery handler.
///
void ConnectionController::discoverPendingProfile()
{
    _waitingForDiscovery = true;
    _clientService->discoverEndpointsWithTimeout(
        _pendingProfile.endpointUrl, _pendingProfile.backend,
        _pendingProfile.endpointTimeoutMs);
}
