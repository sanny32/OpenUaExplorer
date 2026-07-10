// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QByteArray>
#include <QString>

///
/// \brief Trust policy selected for an untrusted server certificate.
///
enum class CertificateTrustDecision {
    Reject,
    TrustOnce,
    TrustPermanently
};

///
/// \brief Interface for deciding whether to trust a server certificate.
///
class CertificateTrustDecider
{
public:
    ///
    /// \brief Default destructor.
    ///
    virtual ~CertificateTrustDecider() = default;

    ///
    /// \brief Decides whether to trust a server certificate.
    /// \param certificate DER-encoded server certificate.
    /// \param message Validation message to present to the user.
    /// \return The chosen trust policy.
    ///
    virtual CertificateTrustDecision decide(const QByteArray &certificate,
                                            const QString &message) = 0;
};
