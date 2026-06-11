// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file pkimanager.h
/// \brief Declares OPC UA PKI directory and certificate management.
///

#pragma once

#include <QByteArray>
#include <QString>

///
/// \brief Creates the application PKI tree and manages certificates.
///
class PkiManager
{
public:
    struct Paths {
        QString root;
        QString ownCertificates;
        QString ownPrivate;
        QString trustedCertificates;
        QString trustedCrl;
        QString rejectedCertificates;
        QString issuerCertificates;
        QString issuerCrl;
    };

    Paths paths() const;
    bool ensureDirectories(QString *error = nullptr) const;
    bool generateClientCertificate(const QString &commonName,
                                   const QString &applicationUri,
                                   QString *certificateFile,
                                   QString *privateKeyFile,
                                   QString *error = nullptr) const;
    bool trustServerCertificate(const QByteArray &certificate, QString *error = nullptr) const;
};
