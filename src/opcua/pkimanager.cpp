// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file pkimanager.cpp
/// \brief Implements OPC UA PKI directory and certificate management.
///

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QStandardPaths>
#include <QSysInfo>

#ifdef OUAEXP_HAS_OPENSSL
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#endif

#include "pkimanager.h"

///
/// \brief PkiManager::paths
/// \return Absolute paths of the application PKI tree.
///
PkiManager::Paths PkiManager::paths() const
{
    const QString root =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
        + QStringLiteral("/pki");
    return {
        root,
        root + QStringLiteral("/own/certs"),
        root + QStringLiteral("/own/private"),
        root + QStringLiteral("/trusted/certs"),
        root + QStringLiteral("/trusted/crl"),
        root + QStringLiteral("/rejected/certs"),
        root + QStringLiteral("/issuers/certs"),
        root + QStringLiteral("/issuers/crl")
    };
}

///
/// \brief PkiManager::ensureDirectories
/// \param error Receives an error message.
/// \return True when every PKI directory exists.
///
bool PkiManager::ensureDirectories(QString *error) const
{
    const Paths p = paths();
    const QStringList directories = {
        p.ownCertificates,
        p.ownPrivate,
        p.trustedCertificates,
        p.trustedCrl,
        p.rejectedCertificates,
        p.issuerCertificates,
        p.issuerCrl
    };
    for (const QString &directory : directories) {
        if (!QDir().mkpath(directory)) {
            if (error)
                *error = QObject::tr("Could not create PKI directory: %1").arg(directory);
            return false;
        }
    }
    return true;
}

///
/// \brief PkiManager::generateClientCertificate
/// \param commonName Certificate common name.
/// \param applicationUri OPC UA application URI.
/// \param certificateFile Receives the generated DER certificate path.
/// \param privateKeyFile Receives the generated PEM private key path.
/// \param error Receives an error message.
/// \return True when a key pair and self-signed certificate were created.
///
bool PkiManager::generateClientCertificate(const QString &commonName,
                                           const QString &applicationUri,
                                           QString *certificateFile,
                                           QString *privateKeyFile,
                                           QString *error) const
{
#ifndef OUAEXP_HAS_OPENSSL
    Q_UNUSED(commonName)
    Q_UNUSED(applicationUri)
    Q_UNUSED(certificateFile)
    Q_UNUSED(privateKeyFile)
    if (error)
        *error = QObject::tr("OpenSSL Crypto support is not available in this build.");
    return false;
#else
    if (!ensureDirectories(error))
        return false;

    EVP_PKEY_CTX *keyContext = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    EVP_PKEY *key = nullptr;
    X509 *certificate = nullptr;
    bool success = false;

    if (!keyContext
        || EVP_PKEY_keygen_init(keyContext) <= 0
        || EVP_PKEY_CTX_set_rsa_keygen_bits(keyContext, 2048) <= 0
        || EVP_PKEY_keygen(keyContext, &key) <= 0) {
        if (error)
            *error = QObject::tr("OpenSSL could not generate the RSA private key.");
        EVP_PKEY_CTX_free(keyContext);
        return false;
    }
    EVP_PKEY_CTX_free(keyContext);

    certificate = X509_new();
    if (certificate) {
        X509_set_version(certificate, 2);
        ASN1_INTEGER_set(X509_get_serialNumber(certificate),
                         static_cast<long>(QDateTime::currentMSecsSinceEpoch() & 0x7fffffff));
        X509_gmtime_adj(X509_get_notBefore(certificate), 0);
        X509_gmtime_adj(X509_get_notAfter(certificate), 3650L * 24L * 60L * 60L);
        X509_set_pubkey(certificate, key);

        X509_NAME *subject = X509_get_subject_name(certificate);
        const QByteArray cn = commonName.toUtf8();
        X509_NAME_add_entry_by_txt(subject, "CN", MBSTRING_UTF8,
                                   reinterpret_cast<const unsigned char *>(cn.constData()),
                                   cn.size(), -1, 0);
        X509_set_issuer_name(certificate, subject);

        const QByteArray san = QStringLiteral("URI:%1,DNS:%2")
            .arg(applicationUri, QSysInfo::machineHostName()).toUtf8();
        X509_EXTENSION *extension =
            X509V3_EXT_conf_nid(nullptr, nullptr, NID_subject_alt_name,
                                const_cast<char *>(san.constData()));
        if (extension) {
            X509_add_ext(certificate, extension, -1);
            X509_EXTENSION_free(extension);
        }

        const Paths p = paths();
        const QString keyPath = p.ownPrivate + QStringLiteral("/openuaexplorer.pem");
        const QString certPath = p.ownCertificates + QStringLiteral("/openuaexplorer.der");
        BIO *keyBio = BIO_new_file(QFile::encodeName(keyPath).constData(), "wb");
        BIO *certBio = BIO_new_file(QFile::encodeName(certPath).constData(), "wb");
        success = keyBio && certBio
            && X509_sign(certificate, key, EVP_sha256()) > 0
            && PEM_write_bio_PrivateKey(keyBio, key, nullptr, nullptr, 0, nullptr, nullptr) == 1
            && i2d_X509_bio(certBio, certificate) == 1;
        BIO_free(keyBio);
        BIO_free(certBio);

        if (success) {
            if (certificateFile)
                *certificateFile = certPath;
            if (privateKeyFile)
                *privateKeyFile = keyPath;
        }
    }

    X509_free(certificate);
    EVP_PKEY_free(key);
    if (!success && error)
        *error = QObject::tr("OpenSSL could not create the client certificate.");
    return success;
#endif
}

///
/// \brief PkiManager::trustServerCertificate
/// \param certificate DER-encoded server certificate.
/// \param error Receives an error message.
/// \return True when the certificate was stored in the trust list.
///
bool PkiManager::trustServerCertificate(const QByteArray &certificate, QString *error) const
{
    if (certificate.isEmpty()) {
        if (error)
            *error = QObject::tr("The server did not provide a certificate.");
        return false;
    }
    if (!ensureDirectories(error))
        return false;

    const QString fingerprint =
        QString::fromLatin1(QCryptographicHash::hash(certificate, QCryptographicHash::Sha256).toHex());
    QSaveFile file(paths().trustedCertificates + QLatin1Char('/') + fingerprint
                   + QStringLiteral(".der"));
    if (!file.open(QIODevice::WriteOnly) || file.write(certificate) != certificate.size()
        || !file.commit()) {
        if (error)
            *error = QObject::tr("Could not store the server certificate in the trust list.");
        return false;
    }
    return true;
}
