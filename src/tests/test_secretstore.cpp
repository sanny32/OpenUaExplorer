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
    void isAvailableReflectsBuild();
    void roundTripWriteReadRemove();
    void distinctSecretsDoNotCollide();

private:
    // Unique per run so the test never clobbers real stored credentials.
    QString uniqueProfileId() const
    {
        return QStringLiteral("ouaexp-unit-test-")
            + QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

    // Performs a synchronous write, returning the reported error string.
    // Empty string means success.
    QString writeSync(SecretStore &store, const QString &profileId,
                      SecretStore::Secret secret, const QString &value)
    {
        QSignalSpy spy(&store, &SecretStore::writeFinished);
        store.write(profileId, secret, value);
        if (!spy.wait(5000))
            return QStringLiteral("timeout");
        return spy.takeFirst().at(2).toString();
    }

    // Performs a synchronous read, writing the value out and returning the error.
    QString readSync(SecretStore &store, const QString &profileId,
                     SecretStore::Secret secret, QString *valueOut)
    {
        QSignalSpy spy(&store, &SecretStore::readFinished);
        store.read(profileId, secret);
        if (!spy.wait(5000))
            return QStringLiteral("timeout");
        const QList<QVariant> args = spy.takeFirst();
        if (valueOut)
            *valueOut = args.at(2).toString();
        return args.at(3).toString();
    }
};

///
/// \brief isAvailable() matches whether QtKeychain was compiled in.
///
void TestSecretStore::isAvailableReflectsBuild()
{
    SecretStore store;
#ifdef OUAEXP_HAS_QTKEYCHAIN
    QVERIFY(store.isAvailable());
#else
    QVERIFY(!store.isAvailable());
    // Without a backend every operation must still answer (with an error).
    QString value;
    QVERIFY(!readSync(store, QStringLiteral("any"),
                      SecretStore::Secret::Password, &value).isEmpty());
#endif
}

///
/// \brief A secret survives a write -> read cycle and is gone after remove.
///
void TestSecretStore::roundTripWriteReadRemove()
{
    SecretStore store;
    if (!store.isAvailable())
        QSKIP("QtKeychain support is not available in this build.");

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
    QSignalSpy removeSpy(&store, &SecretStore::writeFinished);
    store.remove(profileId, SecretStore::Secret::Password);
    QVERIFY(removeSpy.wait(5000));
}

///
/// \brief Password and PrivateKeyPassword are stored under distinct keys.
///
void TestSecretStore::distinctSecretsDoNotCollide()
{
    SecretStore store;
    if (!store.isAvailable())
        QSKIP("QtKeychain support is not available in this build.");

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

    store.remove(profileId, SecretStore::Secret::Password);
    QSignalSpy cleanupSpy(&store, &SecretStore::writeFinished);
    store.remove(profileId, SecretStore::Secret::PrivateKeyPassword);
    cleanupSpy.wait(5000);
}

QTEST_MAIN(TestSecretStore)

#include "test_secretstore.moc"
