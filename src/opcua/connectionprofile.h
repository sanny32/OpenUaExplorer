// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file connectionprofile.h
/// \brief Defines persistent OPC UA connection profile settings.
///

#pragma once

#include <QString>

///
/// \brief Persistent settings for one OPC UA server connection.
///
struct ConnectionProfile
{
    enum class Authentication {
        Anonymous,
        Username,
        Certificate
    };

    QString id;
    QString name;
    QString backend = QStringLiteral("open62541");
    QString endpointUrl;
    QString securityPolicy;
    int securityMode = 1;
    Authentication authentication = Authentication::Anonymous;
    QString username;
    QString clientCertificateFile;
    QString privateKeyFile;
    int sessionTimeoutMs = 60000;
    int connectTimeoutMs = 10000;
    int secureChannelLifetimeMs = 600000;
    int endpointTimeoutMs = 10000;
    int requestTimeoutMs = 15000;
    bool saveProfile = false;
};
