// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>

#include "connectionprofile.h"
#include "opcuatypes.h"
#include "secretstore.h"

class CertificateTrustDecider;
class ConnectionCredentialsProvider;
class ConnectionProfileStore;
class OpcUaBackend;
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
    /// \param backend OPC UA backend.
    /// \param secretStore Secret store for profile passwords.
    /// \param profileStore Persistent profile store.
    /// \param recentStore Persistent recent-connection store.
    /// \param parent Owning QObject.
    ///
    ConnectionController(OpcUaBackend *backend,
                         SecretStore *secretStore,
                         ConnectionProfileStore *profileStore,
                         RecentConnectionStore *recentStore,
                         QObject *parent = nullptr);

    ///
    /// \brief Destroys the controller, deleting the profile store only when it was self-created.
    ///
    ~ConnectionController() override;

    ///
    /// \brief Gives access to the underlying OPC UA backend.
    /// \return The OPC UA backend.
    ///
    OpcUaBackend *backend() const;

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
    /// \brief Returns the unique session name of the current or most recent connection attempt.
    /// \return The effective session name, or an empty string before the first attempt.
    ///
    QString activeSessionName() const;

    ///
    /// \brief Sets the delegate that decides whether to trust a server certificate.
    /// \param decider Trust decider, forwarded to the backend.
    ///
    void setCertificateTrustDecider(CertificateTrustDecider *decider);

    ///
    /// \brief Sets the delegate asked for credentials the credential store does not hold.
    /// \param provider Credentials provider; without one the connection is attempted as is.
    ///
    void setCredentialsProvider(ConnectionCredentialsProvider *provider);

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
    ///
    /// Credentials the keychain does not hold are asked for through the credentials provider,
    /// so a profile with stored secrets reconnects without interrupting the user.
    /// \param profile Saved profile to connect with.
    ///
    void connectSavedProfile(const ConnectionProfile &profile);

    ///
    /// \brief Connects a saved profile with user-supplied credentials, skipping the keychain.
    /// \param profile Saved profile to connect with.
    /// \param password User password, if any.
    /// \param privateKeyPassword Private-key password, if any.
    ///
    void connectSavedProfileWithCredentials(const ConnectionProfile &profile,
                                            const QString &password,
                                            const QString &privateKeyPassword);

    ///
    /// \brief Reconnects the active profile with the credentials its last attempt used.
    /// \return True when an attempt was started; false without an endpoint to return to.
    ///
    bool reconnectActiveProfile();

    ///
    /// \brief Persists a profile and its password, emitting profilesChanged() on success.
    ///
    /// Only the user password is kept. A private-key password would be useless later: the
    /// backend refuses encrypted keys, so it is collected per attempt instead.
    ///
    /// \param profile Profile to store.
    /// \param password User password to store, if non-empty.
    ///
    void saveProfile(const ConnectionProfile &profile, const QString &password);

    ///
    /// \brief Keeps a password for a profile without making the profile a favourite.
    ///
    /// Lets a plain connection be restored unattended later: a session saved from it carries
    /// the profile identifier the password is filed under.
    ///
    /// \param profile Profile the password belongs to.
    /// \param password Password to store; nothing is written when it is empty.
    ///
    void rememberPassword(const ConnectionProfile &profile, const QString &password);

    ///
    /// \brief Removes the saved favourite with the given id, along with its secrets.
    /// \param id Identifier of the favourite to remove.
    ///
    void removeFavorite(const QString &id);

    ///
    /// \brief Persists a new favourites display order, emitting profilesChanged() on success.
    /// \param orderedIds Favourite identifiers in their desired order.
    ///
    void reorderFavorites(const QStringList &orderedIds);

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

    ///
    /// \brief Emitted when the user supplies credentials the stored ones could not cover.
    ///
    /// Raised for a first prompt as well as for one the server's refusal forced, so an open
    /// session can record that its connection no longer matches what was saved with it.
    ///
    void credentialsEntered();

    ///
    /// \brief Emitted when a connection is given up on before it is attempted.
    ///
    /// Raised when the user declines to supply the credentials a saved profile is missing,
    /// so callers waiting for that connection can stop waiting.
    ///
    void connectionAborted();

private slots:
    void handleSecretRead(const QString &profileId, SecretStore::Secret secret,
                          const QString &value, const QString &error);
    void handleEndpoints(const QList<EndpointInfo> &endpoints, const QString &error);
    void handleAuthenticationRejected(const QString &message);
    void handleConnectionState(OpcUaConnectionState state);

private:
    void connectBackend(const ConnectionProfile &profile, const QString &password,
                        const QString &privateKeyPassword);
    bool pendingCredentialsMissing() const;
    QString pendingCredentialsWarning() const;
    void retryWithNewCredentials(const ConnectionProfile &profile, const QString &rejection);
    void storePendingCredentials();
    void startPendingConnection();
    void discoverPendingProfile();
    void forgetProfile(const ConnectionProfile &profile);
    void touchFavorite(const ConnectionProfile &profile);

    OpcUaBackend *_backend;
    ConnectionCredentialsProvider *_credentialsProvider = nullptr;
    SecretStore *_secretStore;
    ConnectionProfileStore *_profileStore;
    RecentConnectionStore *_recentStore;
    bool _ownsDependencies;
    ConnectionProfile _activeProfile;
    QString _activeSessionName;
    QString _activePassword;
    QString _activePrivateKeyPassword;
    QString _instanceSessionSuffix;
    ConnectionProfile _pendingProfile;
    QString _pendingPassword;
    QString _pendingPrivateKeyPassword;
    QString _pendingRejection;
    QString _rejectionMessage;
    int _pendingSecretReads = 0;
    bool _waitingForDiscovery = false;
};
