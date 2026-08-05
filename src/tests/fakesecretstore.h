// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file fakesecretstore.h
/// \brief Provides an in-memory credential store for tests.
///

#pragma once

#include <QHash>
#include <QString>

#include "opcua/secretstore.h"

///
/// \brief Secret store double backed by an in-memory map, resolving reads synchronously.
///
/// Keeps suites off the credential store of the machine they run on, which holds real
/// credentials and, on an unattended machine, may not answer at all.
///
class FakeSecretStore : public SecretStore
{
public:
    using SecretStore::SecretStore;

    bool isAvailable() const override { return true; }

    void read(const QString &profileId, Secret secret) override
    {
        emit readFinished(profileId, secret, values.value(key(profileId, secret)),
                          errors.value(key(profileId, secret)));
    }

    void write(const QString &profileId, Secret secret, const QString &value) override
    {
        values.insert(key(profileId, secret), value);
        emit writeFinished(profileId, secret, {});
    }

    void remove(const QString &profileId, Secret secret) override
    {
        values.remove(key(profileId, secret));
        emit writeFinished(profileId, secret, {});
    }

    ///
    /// \brief Builds the map key a profile secret is filed under.
    /// \param profileId Profile identifier.
    /// \param secret Secret kind.
    /// \return Key into values and errors.
    ///
    static QString key(const QString &profileId, Secret secret)
    {
        return profileId + QLatin1Char('/') + QString::number(static_cast<int>(secret));
    }

    /// \brief Stored secrets, by key().
    QHash<QString, QString> values;
    /// \brief Errors to report from read(), by key().
    QHash<QString, QString> errors;
};
