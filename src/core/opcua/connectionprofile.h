// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file connectionprofile.h
/// \brief Defines persistent OPC UA connection profile settings.
///

#pragma once

#include <QCryptographicHash>
#include <QDateTime>
#include <QString>

///
/// \brief Persistent settings for one OPC UA server connection.
///
struct ConnectionProfile
{
    ///
    /// \brief User authentication mode used for a connection.
    ///
    enum class Authentication {
        Anonymous,
        Username,
        Certificate
    };

    /// \brief Stable profile identifier.
    QString id;

    /// \brief User-visible profile name.
    QString name;

    /// \brief Session name reported to the server.
    QString sessionName;

    /// \brief Preferred Qt OPC UA backend.
    QString backend = QStringLiteral("open62541");

    /// \brief Endpoint URL.
    QString endpointUrl;

    /// \brief Security policy selected for the endpoint.
    QString securityPolicy;

    /// \brief Message security mode numeric value.
    int securityMode = 1;

    /// \brief Authentication mode.
    Authentication authentication = Authentication::Anonymous;

    /// \brief Username for username authentication.
    QString username;

    /// \brief Client certificate file path.
    QString clientCertificateFile;

    /// \brief Private key file path.
    QString privateKeyFile;

    /// \brief Session timeout in milliseconds.
    int sessionTimeoutMs = 600000;

    /// \brief Connection timeout in milliseconds.
    int connectTimeoutMs = 10000;

    /// \brief Secure-channel lifetime in milliseconds.
    int secureChannelLifetimeMs = 600000;

    /// \brief Endpoint discovery timeout in milliseconds.
    int endpointTimeoutMs = 10000;

    /// \brief Read/write request timeout in milliseconds.
    int requestTimeoutMs = 15000;

    /// \brief Maximum message size accepted on the connection, in bytes.
    int maxMessageSizeBytes = 4194304;

    /// \brief Whether the profile should be persisted after connecting.
    bool saveProfile = false;
    
    /// \brief Timestamp of the most recent successful use, if any.
    QDateTime lastUsed;

    ///
    /// \brief Tests whether another profile identifies the same favourite.
    ///
    /// A favourite is identified by its endpoint URL together with the security policy, mode,
    /// and authentication method, so the same server can be saved several times with different
    /// security settings or authentication methods.
    /// \param other Profile to compare against.
    /// \return True when both describe the same favourite.
    ///
    bool isSameEndpoint(const ConnectionProfile &other) const
    {
        return endpointUrl == other.endpointUrl
            && securityPolicy == other.securityPolicy
            && securityMode == other.securityMode
            && authentication == other.authentication;
    }

    ///
    /// \brief Returns the name a stored secret of this profile is filed under.
    ///
    /// A secret is only safe to replay at the endpoint it was given to, so the name covers the
    /// endpoint identity as well as the profile identifier. A profile whose URL, security or
    /// authentication was changed therefore no longer finds the secret of the old one, and
    /// neither does a session file that carries a known identifier next to a foreign URL.
    ///
    /// The hashed fields are exactly those isSameEndpoint() compares: two profiles that count
    /// as the same favourite must resolve the same secret.
    ///
    /// \return Secret name, or an empty string for a profile without an identifier.
    ///
    QString secretScope() const
    {
        if (id.isEmpty())
            return {};

        const QString identity = QStringLiteral("%1|%2|%3|%4")
            .arg(endpointUrl, securityPolicy)
            .arg(securityMode)
            .arg(static_cast<int>(authentication));
        const QByteArray digest =
            QCryptographicHash::hash(identity.toUtf8(), QCryptographicHash::Sha256).toHex();
        return id + QLatin1Char('@') + QString::fromLatin1(digest.left(16));
    }
};
