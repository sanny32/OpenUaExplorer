// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QString>

struct CertificateInfo
{
    enum class Status {
        Invalid,
        NotYetValid,
        Expired,
        Valid
    };

    bool readable = false;
    bool selfSigned = false;
    QString subject;
    QString issuer;
    QString fingerprint;
    QDateTime effectiveDate;
    QDateTime expiryDate;
    Status status = Status::Invalid;

    static CertificateInfo fromDer(const QByteArray &der,
                                   const QDateTime &now = QDateTime::currentDateTimeUtc());
    static Status statusForDates(const QDateTime &effectiveDate,
                                 const QDateTime &expiryDate,
                                 const QDateTime &now);
};
