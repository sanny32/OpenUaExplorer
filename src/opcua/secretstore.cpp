// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file secretstore.cpp
/// \brief Implements secure storage for connection profile secrets.
///

#include <QCoreApplication>

#include <qtkeychain/keychain.h>

#include "secretstore.h"

namespace {
const char serviceName[] = "OpenUaExplorer";
}

///
/// \brief Constructs the store and registers the Secret metatype.
/// \param parent Parent object.
///
SecretStore::SecretStore(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<SecretStore::Secret>();
}

///
/// \brief Reports whether the credential store is usable.
/// \return True because QtKeychain is a required dependency.
///
bool SecretStore::isAvailable() const
{
    return true;
}

///
/// \brief Asynchronously reads a profile secret; the result arrives via readFinished().
/// \param profileId Profile identifier.
/// \param secret Secret kind.
///
void SecretStore::read(const QString &profileId, Secret secret)
{
    auto *job = new QKeychain::ReadPasswordJob(QLatin1String(serviceName), this);
    job->setKey(key(profileId, secret));
    connect(job, &QKeychain::Job::finished, this, [this, job, profileId, secret]() {
        const QString error = job->error() == QKeychain::NoError ? QString() : job->errorString();
        emit readFinished(profileId, secret, job->textData(), error);
        job->deleteLater();
    });
    job->start();
}

///
/// \brief Asynchronously stores a profile secret; completion arrives via writeFinished().
/// \param profileId Profile identifier.
/// \param secret Secret kind.
/// \param value Secret value.
///
void SecretStore::write(const QString &profileId, Secret secret, const QString &value)
{
    auto *job = new QKeychain::WritePasswordJob(QLatin1String(serviceName), this);
    job->setKey(key(profileId, secret));
    job->setTextData(value);
    connect(job, &QKeychain::Job::finished, this, [this, job, profileId, secret]() {
        const QString error = job->error() == QKeychain::NoError ? QString() : job->errorString();
        emit writeFinished(profileId, secret, error);
        job->deleteLater();
    });
    job->start();
}

///
/// \brief Asynchronously deletes a profile secret; completion arrives via writeFinished().
/// \param profileId Profile identifier.
/// \param secret Secret kind.
///
void SecretStore::remove(const QString &profileId, Secret secret)
{
    auto *job = new QKeychain::DeletePasswordJob(QLatin1String(serviceName), this);
    job->setKey(key(profileId, secret));
    connect(job, &QKeychain::Job::finished, this, [this, job, profileId, secret]() {
        const QString error = job->error() == QKeychain::NoError ? QString() : job->errorString();
        emit writeFinished(profileId, secret, error);
        job->deleteLater();
    });
    job->start();
}

///
/// \brief Builds the stable keychain key for a profile secret.
/// \param profileId Profile identifier.
/// \param secret Secret kind.
/// \return Stable keychain key.
///
QString SecretStore::key(const QString &profileId, Secret secret) const
{
    const QString suffix = secret == Secret::Password
        ? QStringLiteral("password")
        : QStringLiteral("private-key-password");
    return QStringLiteral("profile/%1/%2").arg(profileId, suffix);
}
