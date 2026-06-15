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
    virtual ~ConnectionProfileStore() = default;

    virtual QList<ConnectionProfile> profiles() const;
    virtual bool save(const ConnectionProfile &profile);
    virtual bool remove(const QString &id);
};
