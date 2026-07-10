// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file pkimanager.h
/// \brief Declares OPC UA PKI directory and certificate management.
///

#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

///
/// \brief Creates the application PKI tree and manages certificates.
///
class PkiManager
{
public:
    ///
    /// \brief Trust-store category a certificate belongs to.
    ///
    enum class Category {
        Trusted,
        Rejected
    };

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
    /// \brief Returns the application name the client presents to OPC UA servers.
    /// \return "product@host" name identifying this installation.
    ///
    static QString applicationName();

    ///
    /// \brief Returns the product URI reported in the application identity.
    /// \return Product URI in the "vendor:product" form Qt OPC UA derives from certificates.
    ///
    static QString productUri();

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

    ///
    /// \brief Returns the SHA-256 fingerprint used to name a certificate on disk.
    /// \param certificate DER-encoded certificate.
    /// \return Lower-case hexadecimal fingerprint.
    ///
    static QString fingerprint(const QByteArray &certificate);

    ///
    /// \brief Reads every DER certificate stored in a trust-store category.
    /// \param category Trust-store category to enumerate.
    /// \return DER-encoded certificates found in the category directory.
    ///
    QList<QByteArray> certificates(Category category) const;

    ///
    /// \brief Files a certificate under a category, removing it from the other one.
    /// \param certificate DER-encoded certificate.
    /// \param category Destination category.
    /// \param error Receives an error message.
    /// \return True when the certificate is stored under the requested category.
    ///
    bool setCertificateCategory(const QByteArray &certificate, Category category,
                                QString *error = nullptr) const;

    ///
    /// \brief Removes a certificate from both trust-store categories.
    /// \param certificate DER-encoded certificate.
    /// \param error Receives an error message.
    /// \return True when no copy of the certificate remains.
    ///
    bool removeCertificate(const QByteArray &certificate, QString *error = nullptr) const;

    ///
    /// \brief Reports the current client certificate and key files, if both exist.
    /// \param certificateFile Receives the DER certificate path.
    /// \param privateKeyFile Receives the PEM private key path.
    /// \return True when both files are present.
    ///
    bool clientCertificatePaths(QString *certificateFile, QString *privateKeyFile) const;

    ///
    /// \brief Imports a client certificate and key, replacing any existing pair.
    /// \param certificateSource DER or PEM certificate file to import.
    /// \param privateKeySource PEM private key file to import.
    /// \param error Receives an error message.
    /// \return True when the certificate and key were stored as the client identity.
    ///
    bool importClientCertificate(const QString &certificateSource,
                                 const QString &privateKeySource,
                                 QString *error = nullptr) const;

    ///
    /// \brief Removes the client certificate and its private key.
    /// \param error Receives an error message.
    /// \return True when no client certificate or key remains.
    ///
    bool removeClientCertificate(QString *error = nullptr) const;
};
