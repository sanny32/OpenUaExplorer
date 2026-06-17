// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file connectionprofilestore.h
/// \brief Declares persistent storage for OPC UA connection profiles.
///

#pragma once

#include <QList>

#include "connectionprofile.h"

///
/// \brief Reads and writes non-secret connection profile settings.
///
class ConnectionProfileStore
{
public:
    ///
    /// \brief Virtual destructor for polymorphic stores.
    ///
    virtual ~ConnectionProfileStore() = default;

    ///
    /// \brief Returns the saved connection profiles.
    /// \return Saved connection profiles.
    ///
    virtual QList<ConnectionProfile> profiles() const;

    ///
    /// \brief Persists a connection profile, replacing any with the same id.
    /// \param profile Profile to persist.
    /// \return True when the profile identifier is valid.
    ///
    virtual bool save(const ConnectionProfile &profile);

    ///
    /// \brief Removes a stored profile by id.
    /// \param id Profile identifier.
    /// \return True when the settings backend completed successfully.
    ///
    virtual bool remove(const QString &id);
};
