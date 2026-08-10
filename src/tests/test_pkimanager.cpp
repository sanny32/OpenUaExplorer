// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_pkimanager.cpp
/// \brief Tests the PKI store's handling of the client key pair on disk.
///

#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

#include "opcua/pkimanager.h"

///
/// \brief Unit tests for PkiManager.
///
class TestPkiManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void generatedPrivateKeyIsReadableOnlyByTheOwner();
    void importedPrivateKeyIsReadableOnlyByTheOwner();
    void generatedKeyPairIsRecognizedAsTheClientIdentity();

private:
    bool generateClientKeyPair(QString *certificateFile, QString *privateKeyFile);
    static void verifyOwnerOnlyAccess(const QString &path);

    PkiManager _pki;
};

///
/// \brief Fails the running test unless nobody but the file's owner may read it.
///
/// Checked as "no group or world access" rather than against an exact permission set: Unix
/// reports the owner bits through both the Owner and the User flags, and comparing the whole
/// set would only assert that duplication.
///
/// \param path File to inspect.
///
void TestPkiManager::verifyOwnerOnlyAccess(const QString &path)
{
    const QFileDevice::Permissions permissions = QFile(path).permissions();
    const QFileDevice::Permissions shared =
        QFileDevice::ReadGroup | QFileDevice::WriteGroup | QFileDevice::ExeGroup
        | QFileDevice::ReadOther | QFileDevice::WriteOther | QFileDevice::ExeOther;

    QVERIFY2(!(permissions & shared),
             qPrintable(QStringLiteral("%1 is accessible beyond its owner: 0x%2")
                            .arg(path)
                            .arg(static_cast<int>(permissions), 0, 16)));
    QVERIFY(permissions.testFlag(QFileDevice::ReadOwner));
    QVERIFY(permissions.testFlag(QFileDevice::WriteOwner));
}

///
/// \brief Redirects the PKI tree away from the data directory of the running user.
///
void TestPkiManager::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

///
/// \brief Generates the client key pair, skipping the test when OpenSSL cannot.
/// \param certificateFile Receives the generated certificate path.
/// \param privateKeyFile Receives the generated private key path.
/// \return True when the key pair was generated.
///
bool TestPkiManager::generateClientKeyPair(QString *certificateFile, QString *privateKeyFile)
{
    QString error;
    return _pki.generateClientCertificate(PkiManager::clientCertificateCommonName(),
                                          PkiManager::applicationUri(),
                                          certificateFile, privateKeyFile, &error)
        ? true
        : (qWarning("%s", qPrintable(error)), false);
}

///
/// \brief A generated private key is not readable by other users of the machine.
///
void TestPkiManager::generatedPrivateKeyIsReadableOnlyByTheOwner()
{
    QString certificateFile;
    QString privateKeyFile;
    if (!generateClientKeyPair(&certificateFile, &privateKeyFile))
        QSKIP("OpenSSL could not generate a client key pair");

    QVERIFY(QFileInfo::exists(privateKeyFile));
#ifdef Q_OS_WIN
    QSKIP("POSIX file permissions do not apply on Windows");
#else
    verifyOwnerOnlyAccess(privateKeyFile);
#endif
}

///
/// \brief An imported private key is restricted just like a generated one.
///
void TestPkiManager::importedPrivateKeyIsReadableOnlyByTheOwner()
{
    QString generatedCertificate;
    QString generatedKey;
    if (!generateClientKeyPair(&generatedCertificate, &generatedKey))
        QSKIP("OpenSSL could not generate a client key pair");

    // Importing overwrites the pair in place, so the generated one is copied out first and
    // handed back as the import source.
    QTemporaryDir source;
    QVERIFY(source.isValid());
    const QString certificateSource = source.filePath(QStringLiteral("client.der"));
    const QString keySource = source.filePath(QStringLiteral("client.pem"));
    QVERIFY(QFile::copy(generatedCertificate, certificateSource));
    QVERIFY(QFile::copy(generatedKey, keySource));

    QString error;
    QVERIFY2(_pki.importClientCertificate(certificateSource, keySource, &error),
             qPrintable(error));

    QString importedCertificate;
    QString importedKey;
    QVERIFY(_pki.clientCertificatePaths(&importedCertificate, &importedKey));
#ifdef Q_OS_WIN
    QSKIP("POSIX file permissions do not apply on Windows");
#else
    verifyOwnerOnlyAccess(importedKey);
#endif
}

///
/// \brief Buffering the key before writing it leaves a usable, self-consistent pair.
///
void TestPkiManager::generatedKeyPairIsRecognizedAsTheClientIdentity()
{
    QString certificateFile;
    QString privateKeyFile;
    if (!generateClientKeyPair(&certificateFile, &privateKeyFile))
        QSKIP("OpenSSL could not generate a client key pair");

    QFile key(privateKeyFile);
    QVERIFY(key.open(QIODevice::ReadOnly));
    QVERIFY(key.readAll().startsWith("-----BEGIN"));

    QString existingCertificate;
    QString existingKey;
    QVERIFY(_pki.existingClientCertificate(&existingCertificate, &existingKey));
    QCOMPARE(existingCertificate, certificateFile);
    QCOMPARE(existingKey, privateKeyFile);
    QCOMPARE(PkiManager::certificateApplicationUri(certificateFile), PkiManager::applicationUri());
}

QTEST_GUILESS_MAIN(TestPkiManager)
#include "test_pkimanager.moc"
