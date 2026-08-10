// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file secretstore.cpp
/// \brief Implements secure storage for connection profile secrets.
///

#include <QCoreApplication>
#include <QTimer>

#include <qtkeychain/keychain.h>

#include "loggingcategories.h"
#include "secretstore.h"

namespace {
const char serviceName[] = "OpenUaExplorer";

// A locked credential store answers only once someone types its passphrase into a prompt, and
// on an unattended machine that never happens. Every job is therefore given up on after this
// long and reported as if the store had answered, so nothing waits on it forever.
constexpr int operationTimeoutMs = 5000;
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
/// \param profileId Name the secret is filed under.
/// \param secret Secret kind.
///
void SecretStore::read(const QString &profileId, Secret secret)
{
    auto *job = new QKeychain::ReadPasswordJob(QLatin1String(serviceName));
    job->setKey(key(profileId, secret));
    connect(job, &QKeychain::Job::finished, this, [this, job, profileId, secret]() {
        // Nothing stored is the normal answer for a profile whose secret was never kept,
        // so it is reported as an empty value rather than as a failure of the store.
        const bool succeeded = job->error() == QKeychain::NoError
            || job->error() == QKeychain::EntryNotFound;
        const QString error = succeeded ? QString() : job->errorString();
        emit readFinished(profileId, secret, job->textData(), error);
    });
    // A store that stays silent is reported as holding nothing: the caller then asks the user
    // for the secret, which is what it would do for a profile that never had one stored.
    connect(watchdogFor(job), &QTimer::timeout, this, [this, profileId, secret]() {
        qCWarning(lcClient)
            << "The credential store did not answer; asking for the secret instead.";
        emit readFinished(profileId, secret, QString(), QString());
    });
    job->start();
}

///
/// \brief Asynchronously stores a profile secret; completion arrives via writeFinished().
/// \param profileId Name the secret is filed under.
/// \param secret Secret kind.
/// \param value Secret value.
///
void SecretStore::write(const QString &profileId, Secret secret, const QString &value)
{
    auto *job = new QKeychain::WritePasswordJob(QLatin1String(serviceName));
    job->setKey(key(profileId, secret));
    job->setTextData(value);
    connect(job, &QKeychain::Job::finished, this, [this, job, profileId, secret]() {
        const QString error = job->error() == QKeychain::NoError ? QString() : job->errorString();
        emit writeFinished(profileId, secret, error);
    });
    connect(watchdogFor(job), &QTimer::timeout, this, [this, profileId, secret]() {
        emit writeFinished(profileId, secret, tr("The credential store did not respond."));
    });
    job->start();
}

///
/// \brief Asynchronously deletes a profile secret; completion arrives via writeFinished().
/// \param profileId Name the secret is filed under.
/// \param secret Secret kind.
///
void SecretStore::remove(const QString &profileId, Secret secret)
{
    auto *job = new QKeychain::DeletePasswordJob(QLatin1String(serviceName));
    job->setKey(key(profileId, secret));
    connect(job, &QKeychain::Job::finished, this, [this, job, profileId, secret]() {
        const QString error = job->error() == QKeychain::NoError ? QString() : job->errorString();
        emit writeFinished(profileId, secret, error);
    });
    connect(watchdogFor(job), &QTimer::timeout, this, [this, profileId, secret]() {
        emit writeFinished(profileId, secret, tr("The credential store did not respond."));
    });
    job->start();
}

///
/// \brief Starts the timer that abandons a job the credential store leaves unanswered.
///
/// The job itself is deliberately parentless and self-deleting: an operation already handed to
/// the platform store cannot be cancelled, and its callback would reach into a deleted job if
/// the store went away first. Detaching from the job instead lets it finish into nothing, both
/// when it times out and when this SecretStore is destroyed while it is still in flight.
///
/// \param job Job to watch; it must not have been started yet.
/// \return Timer whose timeout() the caller connects its own fallback to.
///
QTimer *SecretStore::watchdogFor(QKeychain::Job *job)
{
    job->setAutoDelete(true);

    auto *watchdog = new QTimer(this);
    watchdog->setSingleShot(true);
    connect(watchdog, &QTimer::timeout, this, [this, job]() { job->disconnect(this); });
    connect(job, &QKeychain::Job::finished, watchdog, [watchdog]() {
        watchdog->stop();
        watchdog->deleteLater();
    });
    watchdog->start(operationTimeoutMs);
    return watchdog;
}

///
/// \brief Builds the stable keychain key for a profile secret.
/// \param profileId Name the secret is filed under.
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
