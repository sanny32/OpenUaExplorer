// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file connectioncredentialsprovider.h
/// \brief Declares the delegate asked for credentials a saved profile has none stored for.
///

#pragma once

#include <QString>

#include "connectionprofile.h"

///
/// \brief Credentials supplied for a connection the credential store could not complete.
///
struct ConnectionCredentials
{
    /// \brief Whether the user supplied credentials instead of giving up on the connection.
    bool accepted = false;

    /// \brief Profile carrying any credential fields the user corrected.
    ConnectionProfile profile;

    /// \brief User password.
    QString password;

    /// \brief Private-key password.
    QString privateKeyPassword;

    /// \brief Whether the secrets should be kept for the next connection of this profile.
    bool remember = false;
};

///
/// \brief Interface for collecting the credentials a saved profile needs to connect.
///
class ConnectionCredentialsProvider
{
public:
    ///
    /// \brief Default destructor.
    ///
    virtual ~ConnectionCredentialsProvider() = default;

    ///
    /// \brief Asks for the credentials the profile cannot connect without.
    /// \param profile Profile waiting to connect.
    /// \param reason What went wrong, ready to show; empty when nothing was configured yet.
    /// \return The supplied credentials, or a rejected result when the user gives up.
    ///
    virtual ConnectionCredentials requestCredentials(const ConnectionProfile &profile,
                                                     const QString &reason) = 0;
};
