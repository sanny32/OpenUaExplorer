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
/// \brief SecretStore::SecretStore
/// \param parent Parent object.
///
SecretStore::SecretStore(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<SecretStore::Secret>();
}

///
/// \brief SecretStore::isAvailable
/// \return True because QtKeychain is a required dependency.
///
bool SecretStore::isAvailable() const
{
    return true;
}

///
/// \brief SecretStore::read
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
/// \brief SecretStore::write
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
/// \brief SecretStore::remove
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
/// \brief SecretStore::key
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
