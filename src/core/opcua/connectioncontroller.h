// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>

#include "connectionprofile.h"
#include "opcuatypes.h"
#include "secretstore.h"

class CertificateTrustDecider;
class ConnectionProfileStore;
class OpcUaClientService;
class RecentConnectionStore;

///
/// \brief Coordinates discovery, secret loading, and persistence for connection profiles.
///
class ConnectionController : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs the controller owning freshly created client, secret, and profile stores.
    /// \param parent Owning QObject.
    ///
    explicit ConnectionController(QObject *parent = nullptr);

    ///
    /// \brief Constructs the controller with injected dependencies, used for testing.
    /// \param clientService OPC UA client service.
    /// \param secretStore Secret store for profile passwords.
    /// \param profileStore Persistent profile store.
    /// \param recentStore Persistent recent-connection store.
    /// \param parent Owning QObject.
    ///
    ConnectionController(OpcUaClientService *clientService,
                         SecretStore *secretStore,
                         ConnectionProfileStore *profileStore,
                         RecentConnectionStore *recentStore,
                         QObject *parent = nullptr);

    ///
    /// \brief Destroys the controller, deleting the profile store only when it was self-created.
    ///
    ~ConnectionController() override;

    ///
    /// \brief Gives access to the underlying client service.
    /// \return The OPC UA client service.
    ///
    OpcUaClientService *clientService() const;

    ///
    /// \brief Returns the saved connection profiles.
    /// \return All persisted profiles.
    ///
    QList<ConnectionProfile> profiles() const;

    ///
    /// \brief Returns the most recent connections, most-recent first.
    /// \return Recent connection profiles.
    ///
    QList<ConnectionProfile> recentConnections() const;

    ///
    /// \brief Returns the profile of the current or most recent connection attempt.
    /// \return The active profile.
    ///
    const ConnectionProfile &activeProfile() const;

    ///
    /// \brief Sets the delegate that decides whether to trust a server certificate.
    /// \param decider Trust decider, forwarded to the client service.
    ///
    void setCertificateTrustDecider(CertificateTrustDecider *decider);

    ///
    /// \brief Connects immediately using credentials supplied by the user (no stored-secret lookup).
    /// \param profile Profile to connect with.
    /// \param password User password, if any.
    /// \param privateKeyPassword Private-key password, if any.
    ///
    void connectNewProfile(const ConnectionProfile &profile,
                           const QString &password,
                           const QString &privateKeyPassword);

    ///
    /// \brief Connects a saved profile, first loading any required secrets from the keychain.
    /// \param profile Saved profile to connect with.
    ///
    void connectSavedProfile(const ConnectionProfile &profile);

    ///
    /// \brief Persists a profile and its secrets, emitting profilesChanged() on success.
    /// \param profile Profile to store.
    /// \param password User password to store, if non-empty.
    /// \param privateKeyPassword Private-key password to store, if non-empty.
    ///
    void saveProfile(const ConnectionProfile &profile,
                     const QString &password,
                     const QString &privateKeyPassword);

    ///
    /// \brief Removes any saved profile matching an endpoint URL, along with its secrets.
    /// \param endpointUrl Endpoint URL whose favourites should be removed.
    ///
    void removeFavorite(const QString &endpointUrl);

signals:
    ///
    /// \brief Emitted when the set of saved profiles changes.
    ///
    void profilesChanged();

    ///
    /// \brief Emitted when the list of recent connections changes.
    ///
    void recentsChanged();

    ///
    /// \brief Emitted when an operation fails.
    /// \param message Error description.
    ///
    void errorOccurred(QString message);

private slots:
    void handleSecretRead(const QString &profileId, SecretStore::Secret secret,
                          const QString &value, const QString &error);
    void handleEndpoints(const QList<EndpointInfo> &endpoints, const QString &error);

private:
    void discoverPendingProfile();
    void forgetProfile(const QString &id);
    void touchFavorite(const ConnectionProfile &profile);

    OpcUaClientService *_clientService;
    SecretStore *_secretStore;
    ConnectionProfileStore *_profileStore;
    RecentConnectionStore *_recentStore;
    bool _ownsDependencies;
    ConnectionProfile _activeProfile;
    ConnectionProfile _pendingProfile;
    QString _pendingPassword;
    QString _pendingPrivateKeyPassword;
    int _pendingSecretReads = 0;
    bool _waitingForDiscovery = false;
};
