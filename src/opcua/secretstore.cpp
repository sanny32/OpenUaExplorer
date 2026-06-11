// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file secretstore.cpp
/// \brief Implements secure storage for connection profile secrets.
///

#include <QCoreApplication>
#include <QMetaObject>

#ifdef OUAEXP_HAS_QTKEYCHAIN
#include <qtkeychain/keychain.h>
#endif

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
/// \return True when QtKeychain support was compiled in.
///
bool SecretStore::isAvailable() const
{
#ifdef OUAEXP_HAS_QTKEYCHAIN
    return true;
#else
    return false;
#endif
}

///
/// \brief SecretStore::read
/// \param profileId Profile identifier.
/// \param secret Secret kind.
///
void SecretStore::read(const QString &profileId, Secret secret)
{
#ifdef OUAEXP_HAS_QTKEYCHAIN
    auto *job = new QKeychain::ReadPasswordJob(QLatin1String(serviceName), this);
    job->setKey(key(profileId, secret));
    connect(job, &QKeychain::Job::finished, this, [this, job, profileId, secret]() {
        const QString error = job->error() == QKeychain::NoError ? QString() : job->errorString();
        emit readFinished(profileId, secret, job->textData(), error);
        job->deleteLater();
    });
    job->start();
#else
    QMetaObject::invokeMethod(this, [this, profileId, secret]() {
        emit readFinished(profileId, secret, QString(),
                          tr("QtKeychain support is not available in this build."));
    }, Qt::QueuedConnection);
#endif
}

///
/// \brief SecretStore::write
/// \param profileId Profile identifier.
/// \param secret Secret kind.
/// \param value Secret value.
///
void SecretStore::write(const QString &profileId, Secret secret, const QString &value)
{
#ifdef OUAEXP_HAS_QTKEYCHAIN
    auto *job = new QKeychain::WritePasswordJob(QLatin1String(serviceName), this);
    job->setKey(key(profileId, secret));
    job->setTextData(value);
    connect(job, &QKeychain::Job::finished, this, [this, job, profileId, secret]() {
        const QString error = job->error() == QKeychain::NoError ? QString() : job->errorString();
        emit writeFinished(profileId, secret, error);
        job->deleteLater();
    });
    job->start();
#else
    Q_UNUSED(value)
    QMetaObject::invokeMethod(this, [this, profileId, secret]() {
        emit writeFinished(profileId, secret,
                           tr("QtKeychain support is not available in this build."));
    }, Qt::QueuedConnection);
#endif
}

///
/// \brief SecretStore::remove
/// \param profileId Profile identifier.
/// \param secret Secret kind.
///
void SecretStore::remove(const QString &profileId, Secret secret)
{
#ifdef OUAEXP_HAS_QTKEYCHAIN
    auto *job = new QKeychain::DeletePasswordJob(QLatin1String(serviceName), this);
    job->setKey(key(profileId, secret));
    connect(job, &QKeychain::Job::finished, this, [this, job, profileId, secret]() {
        const QString error = job->error() == QKeychain::NoError ? QString() : job->errorString();
        emit writeFinished(profileId, secret, error);
        job->deleteLater();
    });
    job->start();
#else
    QMetaObject::invokeMethod(this, [this, profileId, secret]() {
        emit writeFinished(profileId, secret,
                           tr("QtKeychain support is not available in this build."));
    }, Qt::QueuedConnection);
#endif
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
