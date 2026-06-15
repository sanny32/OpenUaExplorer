// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include "connectionprofilevalidator.h"

ConnectionProfileValidator::Error ConnectionProfileValidator::validate(
    const ConnectionProfile &profile)
{
    if (profile.endpointUrl.trimmed().isEmpty())
        return Error::MissingEndpoint;
    if (profile.authentication == ConnectionProfile::Authentication::Username
        && profile.username.trimmed().isEmpty()) {
        return Error::MissingUsername;
    }
    const bool needsCertificate = profile.securityMode > 1
        || profile.authentication == ConnectionProfile::Authentication::Certificate;
    if (needsCertificate
        && (profile.clientCertificateFile.isEmpty() || profile.privateKeyFile.isEmpty())) {
        return Error::MissingClientCertificate;
    }
    return Error::None;
}
