// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_secretstore.cpp
/// \brief Tests the asynchronous OS credential-store adapter.
///

#include <QSignalSpy>
#include <QTest>
#include <QUuid>

#include "opcua/secretstore.h"

namespace {
// SecretStore gives up on an unresponsive store by itself, so every wait here outlasts that:
// the outcome is then always the one the store reported, never a wait that ran out first.
constexpr int waitMs = 15000;
}

///
/// \brief Unit tests for SecretStore.
///
/// The round-trip test talks to the real platform keychain, which is not
/// guaranteed to be available on headless CI machines. Those cases are skipped
/// rather than failed when no working backend is present.
///
class TestSecretStore : public QObject
{
    Q_OBJECT

private slots:
    void isAvailableReflectsRequiredDependency();
    void roundTripWriteReadRemove();
    void distinctSecretsDoNotCollide();
    void unstoredSecretReadsAsEmptyWithoutAnError();

private:
    // Unique per run so the test never clobbers real stored credentials.
    QString uniqueProfileId() const
    {
        return QStringLiteral("ouaexp-unit-test-")
            + QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

    // Performs a synchronous write, returning the reported error string ("" on success).
    QString writeSync(SecretStore &store, const QString &profileId,
                      SecretStore::Secret secret, const QString &value)
    {
        QSignalSpy spy(&store, &SecretStore::writeFinished);
        store.write(profileId, secret, value);
        if (!spy.wait(waitMs))
            return QStringLiteral("timeout");
        return spy.takeFirst().at(2).toString();
    }

    // Deletes a secret and waits for the store to confirm, so nothing is left in flight.
    void removeSync(SecretStore &store, const QString &profileId, SecretStore::Secret secret)
    {
        QSignalSpy spy(&store, &SecretStore::writeFinished);
        store.remove(profileId, secret);
        spy.wait(waitMs);
    }

    // Performs a synchronous read, writing the value out and returning the error.
    QString readSync(SecretStore &store, const QString &profileId,
                     SecretStore::Secret secret, QString *valueOut)
    {
        QSignalSpy spy(&store, &SecretStore::readFinished);
        store.read(profileId, secret);
        if (!spy.wait(waitMs))
            return QStringLiteral("timeout");
        const QList<QVariant> args = spy.takeFirst();
        if (valueOut)
            *valueOut = args.at(2).toString();
        return args.at(3).toString();
    }
};

///
/// \brief isAvailable() reports the required QtKeychain adapter.
///
void TestSecretStore::isAvailableReflectsRequiredDependency()
{
    SecretStore store;
    QVERIFY(store.isAvailable());
}

///
/// \brief A secret survives a write -> read cycle and is gone after remove.
///
void TestSecretStore::roundTripWriteReadRemove()
{
    SecretStore store;

    const QString profileId = uniqueProfileId();
    const QString secret = QStringLiteral("s3cr3t-value");

    const QString writeError =
        writeSync(store, profileId, SecretStore::Secret::Password, secret);
    if (!writeError.isEmpty())
        QSKIP(qPrintable(QStringLiteral("No usable keychain backend: ") + writeError));

    QString readValue;
    const QString readError =
        readSync(store, profileId, SecretStore::Secret::Password, &readValue);
    QVERIFY2(readError.isEmpty(), qPrintable(readError));
    QCOMPARE(readValue, secret);

    // Clean up the entry we created.
    removeSync(store, profileId, SecretStore::Secret::Password);
}

///
/// \brief Password and PrivateKeyPassword are stored under distinct keys.
///
void TestSecretStore::distinctSecretsDoNotCollide()
{
    SecretStore store;

    const QString profileId = uniqueProfileId();
    const QString password = QStringLiteral("login-password");
    const QString keyPassword = QStringLiteral("private-key-password");

    if (!writeSync(store, profileId, SecretStore::Secret::Password, password).isEmpty())
        QSKIP("No usable keychain backend.");
    QVERIFY(writeSync(store, profileId,
                      SecretStore::Secret::PrivateKeyPassword, keyPassword).isEmpty());

    QString readPassword;
    QString readKeyPassword;
    QVERIFY(readSync(store, profileId,
                     SecretStore::Secret::Password, &readPassword).isEmpty());
    QVERIFY(readSync(store, profileId,
                     SecretStore::Secret::PrivateKeyPassword, &readKeyPassword).isEmpty());

    QCOMPARE(readPassword, password);
    QCOMPARE(readKeyPassword, keyPassword);
    QVERIFY(readPassword != readKeyPassword);

    removeSync(store, profileId, SecretStore::Secret::Password);
    removeSync(store, profileId, SecretStore::Secret::PrivateKeyPassword);
}

///
/// \brief A profile with nothing stored reads back empty, so callers can ask the user instead.
///
void TestSecretStore::unstoredSecretReadsAsEmptyWithoutAnError()
{
    SecretStore store;

    QString value = QStringLiteral("untouched");
    const QString readError =
        readSync(store, uniqueProfileId(), SecretStore::Secret::Password, &value);

    QVERIFY2(readError.isEmpty(), qPrintable(readError));
    QVERIFY(value.isEmpty());
}

QTEST_MAIN(TestSecretStore)

#include "test_secretstore.moc"
