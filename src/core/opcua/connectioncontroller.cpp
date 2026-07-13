// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include "connectioncontroller.h"

#include <algorithm>

#include <QDateTime>

#include "connectionprofilestore.h"
#include "opcuabackend.h"
#include "qtopcuabackend.h"
#include "recentconnectionstore.h"

///
/// \brief Constructs the controller owning freshly created client, secret, profile,
///        and recent-connection stores.
/// \param parent Owning QObject.
///
ConnectionController::ConnectionController(QObject *parent)
    : ConnectionController(new QtOpcUaBackend,
                           new SecretStore,
                           new ConnectionProfileStore,
                           new RecentConnectionStore,
                           parent)
{
    _backend->setParent(this);
    _secretStore->setParent(this);
    _ownsDependencies = true;
}

///
/// \brief Constructs the controller with injected dependencies, used for testing.
/// \param backend OPC UA backend.
/// \param secretStore Secret store for profile passwords.
/// \param profileStore Persistent profile store.
/// \param recentStore Persistent recent-connection store.
/// \param parent Owning QObject.
///
ConnectionController::ConnectionController(OpcUaBackend *backend,
                                           SecretStore *secretStore,
                                           ConnectionProfileStore *profileStore,
                                           RecentConnectionStore *recentStore,
                                           QObject *parent)
    : QObject(parent)
    , _backend(backend)
    , _secretStore(secretStore)
    , _profileStore(profileStore)
    , _recentStore(recentStore)
    , _ownsDependencies(false)
{
    Q_ASSERT(_backend);
    Q_ASSERT(_secretStore);
    Q_ASSERT(_profileStore);
    Q_ASSERT(_recentStore);
    connect(_secretStore, &SecretStore::readFinished,
            this, &ConnectionController::handleSecretRead);
    connect(_backend, &OpcUaBackend::endpointsDiscovered,
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
/// \brief Gives access to the underlying OPC UA backend.
/// \return The OPC UA backend.
///
OpcUaBackend *ConnectionController::backend() const
{
    return _backend;
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
/// \param decider Trust decider, forwarded to the backend.
///
void ConnectionController::setCertificateTrustDecider(CertificateTrustDecider *decider)
{
    _backend->setCertificateTrustDecider(decider);
}

///
/// \brief Applies the profile's request timeout to the backend, then connects.
/// \param profile Profile to connect with.
/// \param password User password, if any.
/// \param privateKeyPassword Private-key password, if any.
///
void ConnectionController::connectBackend(const ConnectionProfile &profile,
                                          const QString &password,
                                          const QString &privateKeyPassword)
{
    _backend->setRequestTimeout(profile.requestTimeoutMs);
    _backend->connectToEndpoint(profile, password, privateKeyPassword);
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
    connectBackend(profile, password, privateKeyPassword);
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
/// \brief Connects a saved profile with user-supplied credentials, skipping the keychain.
/// \param profile Saved profile to connect with.
/// \param password User password, if any.
/// \param privateKeyPassword Private-key password, if any.
///
void ConnectionController::connectSavedProfileWithCredentials(const ConnectionProfile &profile,
                                                              const QString &password,
                                                              const QString &privateKeyPassword)
{
    _pendingProfile = profile;
    _pendingPassword = password;
    _pendingPrivateKeyPassword = privateKeyPassword;
    _pendingSecretReads = 0;
    _waitingForDiscovery = false;

    _recentStore->record(profile);
    emit recentsChanged();
    touchFavorite(profile);

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
        if (other.id != profile.id && other.isSameEndpoint(profile))
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
/// \brief Removes the saved favourite with the given id, along with its secrets.
/// \param id Identifier of the favourite to remove.
///
void ConnectionController::removeFavorite(const QString &id)
{
    const QList<ConnectionProfile> existing = _profileStore->profiles();
    const bool present = std::any_of(
        existing.cbegin(), existing.cend(),
        [&id](const ConnectionProfile &profile) { return profile.id == id; });
    if (!present)
        return;
    forgetProfile(id);
    emit profilesChanged();
}

///
/// \brief Persists a new favourites display order, emitting profilesChanged() on success.
/// \param orderedIds Favourite identifiers in their desired order.
///
void ConnectionController::reorderFavorites(const QStringList &orderedIds)
{
    if (_profileStore->setOrder(orderedIds))
        emit profilesChanged();
    else
        emit errorOccurred(tr("Could not save the favourites order."));
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
        if (!favorite.isSameEndpoint(profile))
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
    connectBackend(_pendingProfile, _pendingPassword, _pendingPrivateKeyPassword);
}

///
/// \brief Starts endpoint discovery for the pending profile and arms the discovery handler.
///
void ConnectionController::discoverPendingProfile()
{
    _waitingForDiscovery = true;
    _backend->discoverEndpoints(_pendingProfile.endpointUrl, _pendingProfile.backend,
                                _pendingProfile.endpointTimeoutMs);
}
