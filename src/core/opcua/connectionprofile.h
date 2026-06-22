// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file connectionprofile.h
/// \brief Defines persistent OPC UA connection profile settings.
///

#pragma once

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
    int sessionTimeoutMs = 60000;
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
};
