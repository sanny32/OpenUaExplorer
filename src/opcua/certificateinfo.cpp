// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include <QCryptographicHash>
#include <QSslCertificate>

#include "certificateinfo.h"

namespace {
QString firstValue(const QStringList &values)
{
    return values.isEmpty() ? QString() : values.constFirst();
}
}

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
