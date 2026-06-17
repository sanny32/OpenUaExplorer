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
    ///
    /// \brief Absolute paths of the application PKI directory tree.
    ///
    struct Paths {
        /// \brief PKI root directory.
        QString root;
        /// \brief Directory for own certificates.
        QString ownCertificates;
        /// \brief Directory for own private keys.
        QString ownPrivate;
        /// \brief Directory for trusted server certificates.
        QString trustedCertificates;
        /// \brief Directory for trusted certificate revocation lists.
        QString trustedCrl;
        /// \brief Directory for rejected server certificates.
        QString rejectedCertificates;
        /// \brief Directory for issuer certificates.
        QString issuerCertificates;
        /// \brief Directory for issuer certificate revocation lists.
        QString issuerCrl;
    };

    ///
    /// \brief Returns the application URI used in generated certificates.
    /// \return Application URI matching the four-part format expected by Qt OPC UA.
    ///
    static QString applicationUri();

    ///
    /// \brief Returns the common name for the auto-generated client certificate.
    /// \return Common name used by the auto-generated client certificate.
    ///
    static QString clientCertificateCommonName();

    ///
    /// \brief Returns the absolute paths of the application PKI tree.
    /// \return Absolute paths of the application PKI tree.
    ///
    Paths paths() const;

    ///
    /// \brief Creates the PKI directories if they do not exist.
    /// \param error Receives an error message.
    /// \return True when every PKI directory exists.
    ///
    bool ensureDirectories(QString *error = nullptr) const;

    ///
    /// \brief Reports whether a matching auto-generated client certificate already exists.
    /// \param certificateFile Receives the existing DER certificate path.
    /// \param privateKeyFile Receives the existing PEM private key path.
    /// \return True when the generated key pair exists and has the current application URI.
    ///
    bool existingClientCertificate(QString *certificateFile,
                                   QString *privateKeyFile) const;

    ///
    /// \brief Generates a self-signed client key pair and certificate.
    /// \param commonName Certificate common name.
    /// \param applicationUri OPC UA application URI.
    /// \param certificateFile Receives the generated DER certificate path.
    /// \param privateKeyFile Receives the generated PEM private key path.
    /// \param error Receives an error message.
    /// \return True when a key pair and self-signed certificate were created.
    ///
    bool generateClientCertificate(const QString &commonName,
                                   const QString &applicationUri,
                                   QString *certificateFile,
                                   QString *privateKeyFile,
                                   QString *error = nullptr) const;

    ///
    /// \brief Stores a server certificate in the trust list.
    /// \param certificate DER-encoded server certificate.
    /// \param error Receives an error message.
    /// \return True when the certificate was stored in the trust list.
    ///
    bool trustServerCertificate(const QByteArray &certificate, QString *error = nullptr) const;
};
