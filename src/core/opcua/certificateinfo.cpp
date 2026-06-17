// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include <QCryptographicHash>
#include <QSslCertificate>

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
}

///
/// \brief Parses a DER certificate into display fields, fingerprint, and validity status.
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
    result.selfSigned = certificate.isSelfSigned();

    result.effectiveDate = certificate.effectiveDate();
    result.expiryDate = certificate.expiryDate();
    result.status = statusForDates(result.effectiveDate, result.expiryDate, now);
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
