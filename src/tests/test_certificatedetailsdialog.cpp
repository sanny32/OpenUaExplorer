// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_certificatedetailsdialog.cpp
/// \brief Unit tests for the certificate details dialog.
///

#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

#include "dialogs/certificatedetailsdialog.h"
#include "opcua/pkimanager.h"
#include "widgets/themedpushbutton.h"

///
/// \brief Tests certificate detail parsing and copy output.
///
class TestCertificateDetailsDialog : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void invalidCertificateShowsInvalidStatus();
    void generatedCertificateFillsDetailsAndCopiesText();

private:
    QTemporaryDir _settingsDirectory;
};

///
/// \brief Configures isolated settings and standard paths for PKI files.
///
void TestCertificateDetailsDialog::initTestCase()
{
    QVERIFY(_settingsDirectory.isValid());
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName(QStringLiteral("OpenUaExplorerTests"));
    QCoreApplication::setApplicationName(QStringLiteral("CertificateDetailsDialog"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       _settingsDirectory.path());
}

///
/// \brief Verifies corrupt DER data is reported as invalid.
///
void TestCertificateDetailsDialog::invalidCertificateShowsInvalidStatus()
{
    CertificateDetailsDialog dialog;
    dialog.setCertificate(QByteArrayLiteral("not a certificate"));

    auto *statusValue = dialog.findChild<QLabel *>(QStringLiteral("statusValue"));
    auto *nameValue = dialog.findChild<QLabel *>(QStringLiteral("nameValue"));
    auto *serialNumberValue = dialog.findChild<QLabel *>(QStringLiteral("serialNumberValue"));
    QVERIFY(statusValue);
    QVERIFY(nameValue);
    QVERIFY(serialNumberValue);

    QCOMPARE(statusValue->text(), QStringLiteral("Invalid"));
    QCOMPARE(nameValue->text(), QStringLiteral("Unavailable"));
    QCOMPARE(serialNumberValue->text(), QStringLiteral("Unavailable"));
}

///
/// \brief Verifies a generated certificate populates summary, technical, and copy fields.
///
void TestCertificateDetailsDialog::generatedCertificateFillsDetailsAndCopiesText()
{
    PkiManager pki;
    QString certificateFile;
    QString privateKeyFile;
    QString error;
    const QString commonName = PkiManager::clientCertificateCommonName();
    if (!pki.generateClientCertificate(commonName, PkiManager::applicationUri(),
                                       &certificateFile, &privateKeyFile, &error)) {
        QSKIP(qPrintable(error));
    }

    QFile file(certificateFile);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QByteArray certificate = file.readAll();
    file.close();

    CertificateDetailsDialog dialog;
    dialog.setCertificate(certificate);

    auto *statusValue = dialog.findChild<QLabel *>(QStringLiteral("statusValue"));
    auto *nameValue = dialog.findChild<QLabel *>(QStringLiteral("nameValue"));
    auto *keySizeValue = dialog.findChild<QLabel *>(QStringLiteral("keySizeValue"));
    auto *serialNumberValue = dialog.findChild<QLabel *>(QStringLiteral("serialNumberValue"));
    auto *copyButton = dialog.findChild<ThemedPushButton *>(QStringLiteral("copyButton"));
    auto *exportButton = dialog.findChild<ThemedPushButton *>(QStringLiteral("exportButton"));
    QVERIFY(statusValue);
    QVERIFY(nameValue);
    QVERIFY(keySizeValue);
    QVERIFY(serialNumberValue);
    QVERIFY(copyButton);
    QVERIFY(exportButton);

    QCOMPARE(statusValue->text(), QStringLiteral("Trusted"));
    QCOMPARE(nameValue->text(), commonName);
    QCOMPARE(keySizeValue->text(), QStringLiteral("2048"));
    QVERIFY(serialNumberValue->text() != QStringLiteral("Unavailable"));
    QCOMPARE(copyButton->iconName(), QStringLiteral("copy.svg"));
    QCOMPARE(exportButton->iconName(), QStringLiteral("export.svg"));

    copyButton->click();
    QVERIFY(QApplication::clipboard()->text().contains(commonName));
    QVERIFY(QApplication::clipboard()->text().contains(QStringLiteral("Serial Number:")));

    QVERIFY(QFile::remove(certificateFile));
    QVERIFY(QFile::remove(privateKeyFile));
}

QTEST_MAIN(TestCertificateDetailsDialog)

#include "test_certificatedetailsdialog.moc"
