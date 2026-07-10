// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QString>

///
/// \brief Display and validity details parsed from a certificate.
///
struct CertificateInfo
{
    ///
    /// \brief Certificate validity classification.
    ///
    enum class Status {
        Invalid,
        NotYetValid,
        Expired,
        Valid
    };

    /// \brief Whether the certificate could be parsed.
    bool readable = false;
    /// \brief Whether subject and issuer match.
    bool selfSigned = false;
    /// \brief Subject display text.
    QString subject;
    /// \brief Issuer display text.
    QString issuer;
    /// \brief Certificate serial number display text.
    QString serialNumber;
    /// \brief SHA-256 fingerprint display text.
    QString fingerprint;
    /// \brief Start of the certificate validity window.
    QDateTime effectiveDate;
    /// \brief End of the certificate validity window.
    QDateTime expiryDate;
    /// \brief Validity classification.
    Status status = Status::Invalid;
    /// \brief Public key size in bits, or zero when unavailable.
    int keyBits = 0;

    ///
    /// \brief Parses a DER certificate into display fields, serial number, fingerprint, and validity status.
    /// \param der Certificate bytes in DER encoding.
    /// \param now Reference time used to classify validity.
    /// \return Populated info; the fingerprint is set even when the certificate cannot be parsed.
    ///
    static CertificateInfo fromDer(const QByteArray &der,
                                   const QDateTime &now = QDateTime::currentDateTimeUtc());

    ///
    /// \brief Classifies a certificate's validity from its date range.
    /// \param effectiveDate Start of the validity window.
    /// \param expiryDate End of the validity window.
    /// \param now Reference time to compare against.
    /// \return Invalid for unset dates, otherwise NotYetValid, Expired, or Valid.
    ///
    static Status statusForDates(const QDateTime &effectiveDate,
                                 const QDateTime &expiryDate,
                                 const QDateTime &now);
};
