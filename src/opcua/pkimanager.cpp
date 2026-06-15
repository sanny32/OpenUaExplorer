// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file pkimanager.cpp
/// \brief Implements OPC UA PKI directory and certificate management.
///

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>
#include <QStringList>
#include <QSysInfo>

#ifdef OUAEXP_HAS_OPENSSL
#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#endif

#include "pkimanager.h"

#ifdef OUAEXP_HAS_OPENSSL
namespace {

QString openSslError()
{
    QStringList messages;
    for (unsigned long code = ERR_get_error(); code != 0; code = ERR_get_error()) {
        char buffer[256] = {};
        ERR_error_string_n(code, buffer, sizeof(buffer));
        messages.append(QString::fromLatin1(buffer));
    }
    return messages.join(QStringLiteral("; "));
}

} // namespace
#endif

///
/// \brief PkiManager::applicationUri
/// \return Application URI matching the four-part format expected by Qt OPC UA.
///
QString PkiManager::applicationUri()
{
    QString hostName = QSysInfo::machineHostName().trimmed().toLower();
    if (hostName.isEmpty())
        hostName = QStringLiteral("localhost");
    return QStringLiteral("urn:%1:OpenUaExplorer:OpenUaExplorer").arg(hostName);
}

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
/// \brief PkiManager::existingClientCertificate
/// \param certificateFile Receives the existing DER certificate path.
/// \param privateKeyFile Receives the existing PEM private key path.
/// \return True when the generated key pair exists and has the current application URI.
///
bool PkiManager::existingClientCertificate(QString *certificateFile,
                                           QString *privateKeyFile) const
{
#ifndef OUAEXP_HAS_OPENSSL
    Q_UNUSED(certificateFile)
    Q_UNUSED(privateKeyFile)
    return false;
#else
    const Paths p = paths();
    const QString certPath = p.ownCertificates + QStringLiteral("/openuaexplorer.der");
    const QString keyPath = p.ownPrivate + QStringLiteral("/openuaexplorer.pem");
    if (!QFileInfo::exists(certPath) || !QFileInfo::exists(keyPath))
        return false;

    BIO *certificateBio = BIO_new_file(QFile::encodeName(certPath).constData(), "rb");
    X509 *certificate = certificateBio ? d2i_X509_bio(certificateBio, nullptr) : nullptr;
    BIO_free(certificateBio);
    if (!certificate)
        return false;

    bool uriMatches = false;
    GENERAL_NAMES *names = static_cast<GENERAL_NAMES *>(
        X509_get_ext_d2i(certificate, NID_subject_alt_name, nullptr, nullptr));
    const QByteArray expectedUri = applicationUri().toUtf8();
    for (int index = 0; names && index < sk_GENERAL_NAME_num(names); ++index) {
        const GENERAL_NAME *name = sk_GENERAL_NAME_value(names, index);
        if (name->type != GEN_URI)
            continue;
        const ASN1_IA5STRING *uri = name->d.uniformResourceIdentifier;
        const QByteArray value(
            reinterpret_cast<const char *>(ASN1_STRING_get0_data(uri)),
            ASN1_STRING_length(uri));
        if (value == expectedUri) {
            uriMatches = true;
            break;
        }
    }
    GENERAL_NAMES_free(names);
    X509_free(certificate);
    if (!uriMatches)
        return false;

    if (certificateFile)
        *certificateFile = certPath;
    if (privateKeyFile)
        *privateKeyFile = keyPath;
    return true;
#endif
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
        if (error) {
            *error = QObject::tr("OpenSSL could not generate the RSA private key: %1")
                .arg(openSslError());
        }
        EVP_PKEY_CTX_free(keyContext);
        return false;
    }
    EVP_PKEY_CTX_free(keyContext);

    certificate = X509_new();
    if (certificate) {
        unsigned char serialBytes[16] = {};
        const bool serialGenerated = RAND_bytes(serialBytes, sizeof(serialBytes)) == 1;
        serialBytes[0] &= 0x7f;
        serialBytes[0] |= 0x01;
        BIGNUM *serialNumber = serialGenerated
            ? BN_bin2bn(serialBytes, sizeof(serialBytes), nullptr)
            : nullptr;
        const bool certificateInitialized =
            X509_set_version(certificate, 2) == 1
            && serialNumber
            && BN_to_ASN1_INTEGER(serialNumber, X509_get_serialNumber(certificate))
            && X509_gmtime_adj(X509_get_notBefore(certificate), 0)
            && X509_gmtime_adj(X509_get_notAfter(certificate), 3650L * 24L * 60L * 60L)
            && X509_set_pubkey(certificate, key) == 1;
        BN_free(serialNumber);

        X509_NAME *subject = X509_get_subject_name(certificate);
        const QByteArray cn = commonName.toUtf8();
        const bool subjectInitialized =
            X509_NAME_add_entry_by_txt(
                subject, "CN", MBSTRING_UTF8,
                reinterpret_cast<const unsigned char *>(cn.constData()),
                cn.size(), -1, 0) == 1
            && X509_set_issuer_name(certificate, subject) == 1;

        const QByteArray san = QStringLiteral("URI:%1,DNS:%2")
            .arg(applicationUri, QSysInfo::machineHostName()).toUtf8();
        X509V3_CTX extensionContext;
        X509V3_set_ctx(&extensionContext, certificate, certificate, nullptr, nullptr, 0);
        const auto addExtension =
            [certificate, &extensionContext](int nid, const QByteArray &value) {
            X509_EXTENSION *extension =
                X509V3_EXT_conf_nid(nullptr, &extensionContext, nid,
                                    const_cast<char *>(value.constData()));
            if (!extension)
                return false;
            const bool added = X509_add_ext(certificate, extension, -1) == 1;
            X509_EXTENSION_free(extension);
            return added;
        };
        const bool extensionsAdded =
            addExtension(NID_basic_constraints, QByteArrayLiteral("critical,CA:FALSE"))
            && addExtension(
                NID_key_usage,
                QByteArrayLiteral(
                    "critical,nonRepudiation,digitalSignature,keyEncipherment,dataEncipherment"))
            && addExtension(NID_ext_key_usage, QByteArrayLiteral("clientAuth"))
            && addExtension(NID_subject_alt_name, san)
            && addExtension(NID_subject_key_identifier, QByteArrayLiteral("hash"))
            && addExtension(NID_authority_key_identifier,
                            QByteArrayLiteral("keyid:always,issuer:always"));

        const Paths p = paths();
        const QString keyPath = p.ownPrivate + QStringLiteral("/openuaexplorer.pem");
        const QString certPath = p.ownCertificates + QStringLiteral("/openuaexplorer.der");
        BIO *keyBio = BIO_new_file(QFile::encodeName(keyPath).constData(), "wb");
        BIO *certBio = BIO_new_file(QFile::encodeName(certPath).constData(), "wb");
        success = certificateInitialized && subjectInitialized && extensionsAdded
            && keyBio && certBio
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
    if (!success && error) {
        *error = QObject::tr("OpenSSL could not create the client certificate: %1")
            .arg(openSslError());
    }
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
