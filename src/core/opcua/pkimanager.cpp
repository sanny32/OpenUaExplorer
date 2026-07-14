// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file pkimanager.cpp
/// \brief Implements OPC UA PKI directory and certificate management.
///

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QSslCertificate>
#include <QStandardPaths>
#include <QStringList>
#include <QSysInfo>

#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include "pkimanager.h"
#include "utils.h"

namespace {

constexpr int maximumCommonNameBytes = 64;

///
/// \brief Returns the machine host name, falling back to "localhost".
/// \return Trimmed host name.
///
QString hostName()
{
    QString host = QSysInfo::machineHostName().trimmed();
    if (host.isEmpty())
        host = QStringLiteral("localhost");
    return host;
}

///
/// \brief Removes all spaces from a string.
/// \param value Input string (taken by value).
/// \return The string without spaces.
///
QString withoutSpaces(QString value)
{
    value.remove(QLatin1Char(' '));
    return value;
}

///
/// \brief Returns the longest prefix of a string whose UTF-8 encoding fits a byte budget.
/// \param value String to truncate.
/// \param maximumBytes Maximum allowed UTF-8 size.
/// \return Prefix that never splits a character across the limit.
///
QString utf8Prefix(const QString &value, int maximumBytes)
{
    QByteArray result;
    for (const QChar ch : value) {
        const QByteArray encoded = QString(ch).toUtf8();
        if (result.size() + encoded.size() > maximumBytes)
            break;
        result += encoded;
    }
    return QString::fromUtf8(result);
}

///
/// \brief Builds the client-certificate common name, hash-truncating it to the 64-byte limit.
/// \param host Host the certificate identifies.
/// \return "product@host" name, shortened with a host hash suffix when it would overflow.
///
QString clientCertificateCommonNameForHost(const QString &host)
{
    const QString productName = withoutSpaces(QString::fromUtf8(APP_PRODUCT_NAME));
    const QString normalizedHost = withoutSpaces(host);
    const QString commonName = QStringLiteral("%1@%2").arg(productName, normalizedHost);
    if (commonName.toUtf8().size() <= maximumCommonNameBytes)
        return commonName;

    const QString hashSuffix = QStringLiteral("-%1").arg(QString::fromLatin1(
        QCryptographicHash::hash(normalizedHost.toUtf8(), QCryptographicHash::Sha256)
            .toHex()
            .left(10)));
    const int productLimit = maximumCommonNameBytes - 1 - hashSuffix.toUtf8().size() - 1;
    const QString productPrefix = utf8Prefix(productName, qMax(1, productLimit));
    const int hostLimit =
        maximumCommonNameBytes - productPrefix.toUtf8().size() - 1 - hashSuffix.toUtf8().size();
    const QString hostPrefix = utf8Prefix(normalizedHost, qMax(0, hostLimit));
    return QStringLiteral("%1@%2%3").arg(productPrefix, hostPrefix, hashSuffix);
}

///
/// \brief Extracts the subject common name from an X.509 certificate.
/// \param certificate Certificate to read.
/// \return UTF-8 common name, or empty when absent.
///
QString certificateCommonName(X509 *certificate)
{
    X509_NAME *subject = X509_get_subject_name(certificate);
    const int index = subject ? X509_NAME_get_index_by_NID(subject, NID_commonName, -1) : -1;
    if (index < 0)
        return {};

    X509_NAME_ENTRY *entry = X509_NAME_get_entry(subject, index);
    ASN1_STRING *value = entry ? X509_NAME_ENTRY_get_data(entry) : nullptr;
    if (!value)
        return {};

    unsigned char *utf8 = nullptr;
    const int length = ASN1_STRING_to_UTF8(&utf8, value);
    if (length < 0 || !utf8)
        return {};

    const QString result = QString::fromUtf8(reinterpret_cast<const char *>(utf8), length);
    OPENSSL_free(utf8);
    return result;
}

///
/// \brief Checks whether a certificate's subjectAltName contains the expected URI.
/// \param certificate Certificate to inspect.
/// \param expectedUri Application URI to match.
/// \return True when a matching URI SAN entry is present.
///
bool certificateHasApplicationUri(X509 *certificate, const QByteArray &expectedUri)
{
    bool uriMatches = false;
    GENERAL_NAMES *names = static_cast<GENERAL_NAMES *>(
        X509_get_ext_d2i(certificate, NID_subject_alt_name, nullptr, nullptr));
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
    return uriMatches;
}

///
/// \brief Verifies that a certificate is self-issued and signed by its own key.
/// \param certificate Certificate to verify.
/// \return True when the certificate is genuinely self-signed.
///
bool certificateIsSelfSigned(X509 *certificate)
{
    if (X509_NAME_cmp(X509_get_subject_name(certificate),
                      X509_get_issuer_name(certificate)) != 0) {
        return false;
    }

    EVP_PKEY *publicKey = X509_get_pubkey(certificate);
    const bool verified = publicKey && X509_verify(certificate, publicKey) == 1;
    EVP_PKEY_free(publicKey);
    return verified;
}

///
/// \brief Checks that a certificate is an end-entity application instance certificate.
/// \param certificate Certificate to inspect.
/// \return True when the certificate is not a CA certificate.
/// \note OPC UA Part 6 requires basicConstraints CA:FALSE and no keyCertSign key usage;
///       strict servers reject CA certificates offered as a client identity.
///
bool certificateIsApplicationInstance(X509 *certificate)
{
    return X509_check_ca(certificate) == 0;
}

///
/// \brief Adds a configured X.509v3 extension to a certificate.
/// \param certificate Certificate to extend.
/// \param context Extension context for the certificate.
/// \param nid OpenSSL NID of the extension.
/// \param value Extension value in OpenSSL config syntax.
/// \return True when the extension was created and added.
///
bool addExtension(X509 *certificate, X509V3_CTX *context, int nid, const QByteArray &value)
{
    X509_EXTENSION *extension =
        X509V3_EXT_conf_nid(nullptr, context, nid, const_cast<char *>(value.constData()));
    if (!extension)
        return false;

    const bool added = X509_add_ext(certificate, extension, -1) == 1;
    X509_EXTENSION_free(extension);
    return added;
}

///
/// \brief Drains and formats the OpenSSL error queue.
/// \return Semicolon-separated error messages.
///
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

///
/// \brief Writes bytes to a file atomically.
/// \param path Destination file path.
/// \param data Bytes to write.
/// \return True when the file was written and committed.
///
bool writeFileAtomically(const QString &path, const QByteArray &data)
{
    QSaveFile file(path);
    return file.open(QIODevice::WriteOnly)
        && file.write(data) == data.size()
        && file.commit();
}

///
/// \brief Reads a DER or PEM certificate file and returns it in DER encoding.
/// \param path Certificate file path.
/// \return DER-encoded certificate, or an empty array when it cannot be read.
///
QByteArray readCertificateAsDer(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    const QByteArray raw = file.readAll();

    QList<QSslCertificate> chain = QSslCertificate::fromData(raw, QSsl::Der);
    if (chain.isEmpty())
        chain = QSslCertificate::fromData(raw, QSsl::Pem);
    return chain.isEmpty() ? QByteArray() : chain.constFirst().toDer();
}

} // namespace

///
/// \brief Returns the application URI used in generated certificates.
/// \return Application URI matching the four-part format expected by Qt OPC UA.
///
QString PkiManager::applicationUri()
{
    const QString productName = withoutSpaces(QString::fromUtf8(APP_PRODUCT_NAME));
    return QStringLiteral("urn:%1:%2:%2").arg(hostName().toLower(), productName);
}

///
/// \brief Returns the application name the client presents to OPC UA servers.
/// \return "product@host" name identifying this installation.
///
QString PkiManager::applicationName()
{
    const QString productName = withoutSpaces(QString::fromUtf8(APP_PRODUCT_NAME));
    return QStringLiteral("%1@%2").arg(productName, withoutSpaces(hostName()));
}

///
/// \brief Returns the product URI reported in the application identity.
/// \return Product URI in the "vendor:product" form Qt OPC UA derives from certificates.
///
QString PkiManager::productUri()
{
    const QString productName = withoutSpaces(QString::fromUtf8(APP_PRODUCT_NAME));
    return QStringLiteral("%1:%1").arg(productName);
}

///
/// \brief Returns the common name for the auto-generated client certificate.
/// \return Common name used by the auto-generated client certificate.
///
QString PkiManager::clientCertificateCommonName()
{
    return clientCertificateCommonNameForHost(hostName());
}

///
/// \brief Returns the absolute paths of the application PKI tree.
/// \return Absolute paths of the application PKI tree.
///
PkiManager::Paths PkiManager::paths() const
{
    const QString root =
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
        + QLatin1Char('/') + QString::fromUtf8(APP_PRODUCT_NAME)
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
/// \brief Creates the PKI directories if they do not exist.
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
/// \brief Reports whether a matching auto-generated client certificate already exists.
/// \param certificateFile Receives the existing DER certificate path.
/// \param privateKeyFile Receives the existing PEM private key path.
/// \return True when the generated key pair exists, has the current application URI and is a
///         usable end-entity certificate.
///
bool PkiManager::existingClientCertificate(QString *certificateFile,
                                           QString *privateKeyFile) const
{
    const Paths p = paths();
    const QString fileBaseName = Utils::executableBaseName();
    const QString certPath = p.ownCertificates + QLatin1Char('/') + fileBaseName
        + QStringLiteral(".der");
    const QString keyPath = p.ownPrivate + QLatin1Char('/') + fileBaseName
        + QStringLiteral(".pem");
    if (!QFileInfo::exists(certPath) || !QFileInfo::exists(keyPath))
        return false;

    BIO *certificateBio = BIO_new_file(QFile::encodeName(certPath).constData(), "rb");
    X509 *certificate = certificateBio ? d2i_X509_bio(certificateBio, nullptr) : nullptr;
    BIO_free(certificateBio);
    if (!certificate)
        return false;

    const QByteArray expectedUri = applicationUri().toUtf8();
    const bool identityMatches =
        certificateHasApplicationUri(certificate, expectedUri)
        && certificateCommonName(certificate) == clientCertificateCommonName()
        && certificateIsSelfSigned(certificate)
        && certificateIsApplicationInstance(certificate);
    X509_free(certificate);
    if (!identityMatches)
        return false;

    if (certificateFile)
        *certificateFile = certPath;
    if (privateKeyFile)
        *privateKeyFile = keyPath;
    return true;
}

///
/// \brief Generates a self-signed client key pair and certificate.
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
            && X509_gmtime_adj(X509_getm_notBefore(certificate), 0)
            && X509_gmtime_adj(X509_getm_notAfter(certificate), 3650L * 24L * 60L * 60L)
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
            .arg(applicationUri, hostName()).toUtf8();
        X509V3_CTX extensionContext;
        X509V3_set_ctx(&extensionContext, certificate, certificate, nullptr, nullptr, 0);
        const bool extensionsAdded =
            addExtension(certificate, &extensionContext,
                         NID_basic_constraints, QByteArrayLiteral("critical,CA:FALSE"))
            && addExtension(
                certificate, &extensionContext,
                NID_key_usage,
                QByteArrayLiteral(
                    "critical,nonRepudiation,digitalSignature,keyEncipherment,"
                    "dataEncipherment"))
            && addExtension(certificate, &extensionContext,
                            NID_ext_key_usage, QByteArrayLiteral("clientAuth"))
            && addExtension(certificate, &extensionContext, NID_subject_alt_name, san)
            && addExtension(certificate, &extensionContext,
                            NID_subject_key_identifier, QByteArrayLiteral("hash"))
            && addExtension(certificate, &extensionContext, NID_authority_key_identifier,
                            QByteArrayLiteral("keyid:always,issuer:always"));

        const Paths p = paths();
        const QString fileBaseName = Utils::executableBaseName();
        const QString keyPath = p.ownPrivate + QLatin1Char('/') + fileBaseName
            + QStringLiteral(".pem");
        const QString certPath = p.ownCertificates + QLatin1Char('/') + fileBaseName
            + QStringLiteral(".der");
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
}

///
/// \brief Stores a server certificate in the trust list.
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
    return setCertificateCategory(certificate, Category::Trusted, error);
}

///
/// \brief Returns the SHA-256 fingerprint used to name a certificate on disk.
/// \param certificate DER-encoded certificate.
/// \return Lower-case hexadecimal fingerprint.
///
QString PkiManager::fingerprint(const QByteArray &certificate)
{
    return QString::fromLatin1(
        QCryptographicHash::hash(certificate, QCryptographicHash::Sha256).toHex());
}

///
/// \brief Reads every DER certificate stored in a trust-store category.
/// \param category Trust-store category to enumerate.
/// \return DER-encoded certificates found in the category directory.
///
QList<QByteArray> PkiManager::certificates(Category category) const
{
    const Paths p = paths();
    const QString directory = category == Category::Trusted
        ? p.trustedCertificates
        : p.rejectedCertificates;

    QList<QByteArray> result;
    const QStringList files =
        QDir(directory).entryList({QStringLiteral("*.der")}, QDir::Files, QDir::Name);
    for (const QString &name : files) {
        QFile file(directory + QLatin1Char('/') + name);
        if (file.open(QIODevice::ReadOnly))
            result.append(file.readAll());
    }
    return result;
}

///
/// \brief Files a certificate under a category, removing it from the other one.
/// \param certificate DER-encoded certificate.
/// \param category Destination category.
/// \param error Receives an error message.
/// \return True when the certificate is stored under the requested category.
///
bool PkiManager::setCertificateCategory(const QByteArray &certificate, Category category,
                                        QString *error) const
{
    if (certificate.isEmpty()) {
        if (error)
            *error = QObject::tr("The certificate is empty.");
        return false;
    }
    if (!ensureDirectories(error))
        return false;

    const Paths p = paths();
    const QString fileName = fingerprint(certificate) + QStringLiteral(".der");
    const QString destination = category == Category::Trusted
        ? p.trustedCertificates
        : p.rejectedCertificates;
    const QString other = category == Category::Trusted
        ? p.rejectedCertificates
        : p.trustedCertificates;

    if (!writeFileAtomically(destination + QLatin1Char('/') + fileName, certificate)) {
        if (error)
            *error = QObject::tr("Could not store the certificate in the PKI store.");
        return false;
    }
    QFile::remove(other + QLatin1Char('/') + fileName);
    return true;
}

///
/// \brief Removes a certificate from both trust-store categories.
/// \param certificate DER-encoded certificate.
/// \param error Receives an error message.
/// \return True when no copy of the certificate remains.
///
bool PkiManager::removeCertificate(const QByteArray &certificate, QString *error) const
{
    const Paths p = paths();
    const QString fileName = fingerprint(certificate) + QStringLiteral(".der");
    bool removed = true;
    for (const QString &directory : {p.trustedCertificates, p.rejectedCertificates}) {
        const QString path = directory + QLatin1Char('/') + fileName;
        if (QFileInfo::exists(path))
            removed = QFile::remove(path) && removed;
    }
    if (!removed && error)
        *error = QObject::tr("Could not remove the certificate from the PKI store.");
    return removed;
}

///
/// \brief Reports the current client certificate and key files, if both exist.
/// \param certificateFile Receives the DER certificate path.
/// \param privateKeyFile Receives the PEM private key path.
/// \return True when both files are present.
///
bool PkiManager::clientCertificatePaths(QString *certificateFile, QString *privateKeyFile) const
{
    const Paths p = paths();
    const QString fileBaseName = Utils::executableBaseName();
    const QString certPath = p.ownCertificates + QLatin1Char('/') + fileBaseName
        + QStringLiteral(".der");
    const QString keyPath = p.ownPrivate + QLatin1Char('/') + fileBaseName
        + QStringLiteral(".pem");
    if (!QFileInfo::exists(certPath) || !QFileInfo::exists(keyPath))
        return false;

    if (certificateFile)
        *certificateFile = certPath;
    if (privateKeyFile)
        *privateKeyFile = keyPath;
    return true;
}

///
/// \brief Imports a client certificate and key, replacing any existing pair.
/// \param certificateSource DER or PEM certificate file to import.
/// \param privateKeySource PEM private key file to import.
/// \param error Receives an error message.
/// \return True when the certificate and key were stored as the client identity.
///
bool PkiManager::importClientCertificate(const QString &certificateSource,
                                         const QString &privateKeySource,
                                         QString *error) const
{
    if (!ensureDirectories(error))
        return false;

    const QByteArray certificate = readCertificateAsDer(certificateSource);
    if (certificate.isEmpty()) {
        if (error)
            *error = QObject::tr("The selected file is not a readable certificate.");
        return false;
    }

    QFile keyFile(privateKeySource);
    if (!keyFile.open(QIODevice::ReadOnly)) {
        if (error)
            *error = QObject::tr("The selected private key could not be read.");
        return false;
    }
    const QByteArray key = keyFile.readAll();

    const Paths p = paths();
    const QString fileBaseName = Utils::executableBaseName();
    const QString certPath = p.ownCertificates + QLatin1Char('/') + fileBaseName
        + QStringLiteral(".der");
    const QString keyPath = p.ownPrivate + QLatin1Char('/') + fileBaseName
        + QStringLiteral(".pem");
    if (!writeFileAtomically(certPath, certificate) || !writeFileAtomically(keyPath, key)) {
        if (error)
            *error = QObject::tr("Could not store the imported client certificate.");
        return false;
    }
    return true;
}

///
/// \brief Removes the client certificate and its private key.
/// \param error Receives an error message.
/// \return True when no client certificate or key remains.
///
bool PkiManager::removeClientCertificate(QString *error) const
{
    QString certPath;
    QString keyPath;
    if (!clientCertificatePaths(&certPath, &keyPath))
        return true;

    const bool removed = QFile::remove(certPath) & QFile::remove(keyPath);
    if (!removed && error)
        *error = QObject::tr("Could not remove the client certificate.");
    return removed;
}
