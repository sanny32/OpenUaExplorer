// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "connectionprofile.h"

///
/// \brief Validates connection profiles for required credentials and certificates.
///
class ConnectionProfileValidator
{
public:
    ///
    /// \brief Validation outcomes for a connection profile.
    ///
    enum class Error {
        None,
        MissingEndpoint,
        MissingUsername,
        MissingClientCertificate
    };

    ///
    /// \brief Checks a connection profile for the fields required by its security and auth settings.
    /// \param profile Profile to validate.
    /// \return The first problem found, or Error::None when the profile is complete.
    ///
    static Error validate(const ConnectionProfile &profile);
};
