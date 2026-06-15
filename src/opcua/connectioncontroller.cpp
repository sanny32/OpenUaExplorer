// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include "connectioncontroller.h"

#include "connectionprofilestore.h"
#include "opcuaclientservice.h"

ConnectionController::ConnectionController(QObject *parent)
    : ConnectionController(new OpcUaClientService,
                           new SecretStore,
                           new ConnectionProfileStore,
                           parent)
{
    _clientService->setParent(this);
    _secretStore->setParent(this);
    _ownsDependencies = true;
}

ConnectionController::ConnectionController(OpcUaClientService *clientService,
                                           SecretStore *secretStore,
                                           ConnectionProfileStore *profileStore,
                                           QObject *parent)
    : QObject(parent)
    , _clientService(clientService)
    , _secretStore(secretStore)
    , _profileStore(profileStore)
    , _ownsDependencies(false)
{
    Q_ASSERT(_clientService);
    Q_ASSERT(_secretStore);
    Q_ASSERT(_profileStore);
    connect(_secretStore, &SecretStore::readFinished,
            this, &ConnectionController::handleSecretRead);
    connect(_clientService, &OpcUaClientService::endpointsDiscovered,
            this, &ConnectionController::handleEndpoints);
}

ConnectionController::~ConnectionController()
{
    if (_ownsDependencies)
        delete _profileStore;
}

OpcUaClientService *ConnectionController::clientService() const
{
    return _clientService;
}

QList<ConnectionProfile> ConnectionController::profiles() const
{
    return _profileStore->profiles();
}

const ConnectionProfile &ConnectionController::activeProfile() const
{
    return _activeProfile;
}

void ConnectionController::setCertificateTrustDecider(CertificateTrustDecider *decider)
{
    _clientService->setCertificateTrustDecider(decider);
}

void ConnectionController::connectNewProfile(const ConnectionProfile &profile,
                                             const QString &password,
                                             const QString &privateKeyPassword)
{
    _waitingForDiscovery = false;
    _activeProfile = profile;
    _clientService->connectToEndpoint(profile, password, privateKeyPassword);
}

void ConnectionController::connectSavedProfile(const ConnectionProfile &profile)
{
    _pendingProfile = profile;
    _pendingPassword.clear();
    _pendingPrivateKeyPassword.clear();
    _pendingSecretReads = 0;
    _waitingForDiscovery = false;

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

void ConnectionController::saveProfile(const ConnectionProfile &profile,
                                       const QString &password,
                                       const QString &privateKeyPassword)
{
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

void ConnectionController::discoverPendingProfile()
{
    _waitingForDiscovery = true;
    _clientService->discoverEndpointsWithTimeout(
        _pendingProfile.endpointUrl, _pendingProfile.backend,
        _pendingProfile.endpointTimeoutMs);
}
