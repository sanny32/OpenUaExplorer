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

class ConnectionController : public QObject
{
    Q_OBJECT

public:
    explicit ConnectionController(QObject *parent = nullptr);
    ConnectionController(OpcUaClientService *clientService,
                         SecretStore *secretStore,
                         ConnectionProfileStore *profileStore,
                         QObject *parent = nullptr);
    ~ConnectionController() override;

    OpcUaClientService *clientService() const;
    QList<ConnectionProfile> profiles() const;
    const ConnectionProfile &activeProfile() const;
    void setCertificateTrustDecider(CertificateTrustDecider *decider);

    void connectNewProfile(const ConnectionProfile &profile,
                           const QString &password,
                           const QString &privateKeyPassword);
    void connectSavedProfile(const ConnectionProfile &profile);
    void saveProfile(const ConnectionProfile &profile,
                     const QString &password,
                     const QString &privateKeyPassword);

signals:
    void profilesChanged();
    void errorOccurred(QString message);

private slots:
    void handleSecretRead(const QString &profileId, SecretStore::Secret secret,
                          const QString &value, const QString &error);
    void handleEndpoints(const QList<EndpointInfo> &endpoints, const QString &error);

private:
    void discoverPendingProfile();

    OpcUaClientService *_clientService;
    SecretStore *_secretStore;
    ConnectionProfileStore *_profileStore;
    bool _ownsDependencies;
    ConnectionProfile _activeProfile;
    ConnectionProfile _pendingProfile;
    QString _pendingPassword;
    QString _pendingPrivateKeyPassword;
    int _pendingSecretReads = 0;
    bool _waitingForDiscovery = false;
};
