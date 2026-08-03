// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include <cstring>

#include <QCryptographicHash>
#include <QHostAddress>
#include <QSslCertificate>
#include <QtEndian>

#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include "certificateinfo.h"

namespace {
///
/// \brief Returns the first entry of a string list, or an empty string when empty.
/// \param values List to read from.
/// \return First value or an empty string.
///
QString firstValue(const QStringList &values)
{
    return values.isEmpty() ? QString() : values.constFirst();
}

///
/// \brief Reads the public key size of a certificate via OpenSSL.
/// \param certificate Parsed certificate.
/// \return Key size in bits, or zero when it cannot be determined.
/// \note Uses OpenSSL directly because Qt's QSslKey cannot read keys when the
///       Qt build and the available OpenSSL runtime disagree on ABI (e.g. Qt 5
///       linked for OpenSSL 1.1 running against OpenSSL 3).
///
int publicKeyBits(X509 *certificate)
{
    EVP_PKEY *publicKey = X509_get_pubkey(certificate);
    const int bits = publicKey ? EVP_PKEY_bits(publicKey) : 0;
    EVP_PKEY_free(publicKey);
    return bits > 0 ? bits : 0;
}

///
/// \brief Reports whether a certificate is issued by itself and signed by its own key.
/// \param certificate Parsed certificate.
/// \return True when the certificate is self-signed.
/// \note Does not use QSslCertificate::isSelfSigned(), which relies on OpenSSL's
///       X509_check_issued() and therefore only accepts an issuer carrying the keyCertSign
///       key usage. An OPC UA application instance certificate is an end entity without it,
///       so a genuinely self-signed one would be reported as issued by someone else.
///
bool isSelfSigned(X509 *certificate)
{
    bool selfSigned = X509_NAME_cmp(X509_get_subject_name(certificate),
                                    X509_get_issuer_name(certificate)) == 0;
    if (selfSigned) {
        EVP_PKEY *publicKey = X509_get_pubkey(certificate);
        selfSigned = publicKey && X509_verify(certificate, publicKey) == 1;
        EVP_PKEY_free(publicKey);
    }
    return selfSigned;
}

///
/// \brief Reads the signature algorithm name of a certificate via OpenSSL.
/// \param certificate Parsed certificate.
/// \return Algorithm name such as "sha256WithRSAEncryption", or an empty string.
/// \note Does not use QSslCertificate::toText(), which returns nothing unless Qt
///       runs on its OpenSSL TLS backend.
///
QString signatureAlgorithmName(const X509 *certificate)
{
    const X509_ALGOR *algorithm = nullptr;
    X509_get0_signature(nullptr, &algorithm, certificate);
    if (!algorithm)
        return QString();

    const ASN1_OBJECT *object = nullptr;
    X509_ALGOR_get0(&object, nullptr, nullptr, algorithm);
    if (!object)
        return QString();

    char name[128] = {};
    if (OBJ_obj2txt(name, sizeof(name), object, 0) <= 0)
        return QString();
    return QString::fromLatin1(name);
}

///
/// \brief Formats an iPAddress general name.
/// \param address Address octets, four for IPv4 and sixteen for IPv6.
/// \return Address text, or an empty string for any other length.
///
QString ipAddress(const ASN1_OCTET_STRING *address)
{
    const unsigned char *data = ASN1_STRING_get0_data(address);
    const int length = ASN1_STRING_length(address);
    if (length == 4)
        return QHostAddress(qFromBigEndian<quint32>(data)).toString();
    if (length == 16) {
        Q_IPV6ADDR octets;
        std::memcpy(octets.c, data, sizeof(octets.c));
        return QHostAddress(octets).toString();
    }
    return QString();
}

///
/// \brief Reads the subject alternative names of a certificate via OpenSSL.
/// \param certificate Parsed certificate.
/// \return GeneralName tag and value pairs in certificate order; unsupported name
///         forms such as otherName and directoryName are left out.
/// \note Does not use QSslCertificate::extensions(), whose parser only reports
///       email, DNS, and IP names unless Qt runs on its OpenSSL TLS backend.
///
QList<QPair<int, QString>> readSubjectAlternativeNames(X509 *certificate)
{
    QList<QPair<int, QString>> names;
    GENERAL_NAMES *alternativeNames = static_cast<GENERAL_NAMES *>(
        X509_get_ext_d2i(certificate, NID_subject_alt_name, nullptr, nullptr));
    if (!alternativeNames)
        return names;

    const int count = sk_GENERAL_NAME_num(alternativeNames);
    for (int index = 0; index < count; ++index) {
        const GENERAL_NAME *name = sk_GENERAL_NAME_value(alternativeNames, index);
        if (!name)
            continue;

        QString value;
        switch (name->type) {
        case GEN_EMAIL:
        case GEN_DNS:
        case GEN_URI: {
            const ASN1_IA5STRING *text = name->d.ia5;
            value = QString::fromLatin1(
                reinterpret_cast<const char *>(ASN1_STRING_get0_data(text)),
                ASN1_STRING_length(text));
            break;
        }
        case GEN_IPADD:
            value = ipAddress(name->d.iPAddress);
            break;
        default:
            break;
        }

        if (!value.isEmpty())
            names.append({name->type, value});
    }
    GENERAL_NAMES_free(alternativeNames);
    return names;
}
}

///
/// \brief Parses a DER certificate into display fields, serial number, fingerprint, and validity status.
/// \param der Certificate bytes in DER encoding.
/// \param now Reference time used to classify validity.
/// \return Populated info; the fingerprint is set even when the certificate cannot be parsed.
///
CertificateInfo CertificateInfo::fromDer(const QByteArray &der, const QDateTime &now)
{
    CertificateInfo result;
    result.fingerprint = QString::fromLatin1(
        QCryptographicHash::hash(der, QCryptographicHash::Sha256)
            .toHex(':').toUpper());

    const QList<QSslCertificate> chain = QSslCertificate::fromData(der, QSsl::Der);
    if (chain.isEmpty())
        return result;

    const QSslCertificate certificate = chain.constFirst();
    result.readable = true;
    result.serialNumber = QString::fromLatin1(certificate.serialNumber()).toUpper();
    result.subject = firstValue(
        certificate.subjectInfo(QSslCertificate::CommonName));
    if (result.subject.isEmpty()) {
        result.subject = firstValue(
            certificate.subjectInfo(QSslCertificate::Organization));
    }
    if (result.subject.isEmpty())
        result.subject = certificate.subjectDisplayName();

    result.issuer = firstValue(
        certificate.issuerInfo(QSslCertificate::CommonName));
    if (result.issuer.isEmpty()) {
        result.issuer = firstValue(
            certificate.issuerInfo(QSslCertificate::Organization));
    }
    if (result.issuer.isEmpty())
        result.issuer = certificate.issuerDisplayName();

    result.effectiveDate = certificate.effectiveDate();
    result.expiryDate = certificate.expiryDate();
    result.status = statusForDates(result.effectiveDate, result.expiryDate, now);

    const unsigned char *data = reinterpret_cast<const unsigned char *>(der.constData());
    if (X509 *parsed = d2i_X509(nullptr, &data, der.size())) {
        result.selfSigned = isSelfSigned(parsed);
        result.keyBits = publicKeyBits(parsed);
        result.signatureAlgorithm = signatureAlgorithmName(parsed);
        result.subjectAlternativeNames = readSubjectAlternativeNames(parsed);
        X509_free(parsed);
    }

    for (const auto &alternativeName : std::as_const(result.subjectAlternativeNames)) {
        if (alternativeName.first == GEN_URI) {
            result.applicationUri = alternativeName.second;
            break;
        }
    }
    return result;
}

///
/// \brief Classifies a certificate's validity from its date range.
/// \param effectiveDate Start of the validity window.
/// \param expiryDate End of the validity window.
/// \param now Reference time to compare against.
/// \return Invalid for unset dates, otherwise NotYetValid, Expired, or Valid.
///
CertificateInfo::Status CertificateInfo::statusForDates(
    const QDateTime &effectiveDate, const QDateTime &expiryDate,
    const QDateTime &now)
{
    if (!effectiveDate.isValid() || !expiryDate.isValid())
        return Status::Invalid;
    if (now < effectiveDate)
        return Status::NotYetValid;
    if (now > expiryDate)
        return Status::Expired;
    return Status::Valid;
}
