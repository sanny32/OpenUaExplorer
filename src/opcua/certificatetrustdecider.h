// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QByteArray>
#include <QString>

enum class CertificateTrustDecision {
    Reject,
    TrustOnce,
    TrustPermanently
};

class CertificateTrustDecider
{
public:
    virtual ~CertificateTrustDecider() = default;
    virtual CertificateTrustDecision decide(const QByteArray &certificate,
                                            const QString &message) = 0;
};
