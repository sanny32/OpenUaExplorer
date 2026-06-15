// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "connectionprofile.h"

class ConnectionProfileValidator
{
public:
    enum class Error {
        None,
        MissingEndpoint,
        MissingUsername,
        MissingClientCertificate
    };

    static Error validate(const ConnectionProfile &profile);
};
