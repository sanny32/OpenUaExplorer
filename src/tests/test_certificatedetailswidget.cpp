// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_certificatedetailswidget.cpp
/// \brief Unit tests for the certificate details widget.
///

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

#include "opcua/pkimanager.h"
#include "widgets/certificatedetailswidget.h"

///
/// \brief Tests certificate detail parsing and copyable text.
///
class TestCertificateDetailsWidget : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void invalidCertificateShowsInvalidStatus();
    void generatedCertificateFillsDetailsAndDetailsText();

private:
    QTemporaryDir _settingsDirectory;
};

///
/// \brief Configures isolated settings and standard paths for PKI files.
///
void TestCertificateDetailsWidget::initTestCase()
{
    QVERIFY(_settingsDirectory.isValid());
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName(QStringLiteral("OpenUaExplorerTests"));
    QCoreApplication::setApplicationName(QStringLiteral("CertificateDetailsWidget"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       _settingsDirectory.path());
}

///
/// \brief Verifies corrupt DER data is reported as invalid.
///
void TestCertificateDetailsWidget::invalidCertificateShowsInvalidStatus()
{
    CertificateDetailsWidget widget;
    const QByteArray certificate = QByteArrayLiteral("not a certificate");
    widget.setCertificate(certificate);

    auto *statusValue = widget.findChild<QLabel *>(QStringLiteral("statusValue"));
    auto *nameValue = widget.findChild<QLabel *>(QStringLiteral("nameValue"));
    auto *certificatePathValue =
        widget.findChild<QLabel *>(QStringLiteral("certificatePathValue"));
    auto *serialNumberValue = widget.findChild<QLabel *>(QStringLiteral("serialNumberValue"));
    QVERIFY(statusValue);
    QVERIFY(nameValue);
    QVERIFY(certificatePathValue);
    QVERIFY(serialNumberValue);

    QCOMPARE(widget.certificate(), certificate);
    QCOMPARE(statusValue->text(), QStringLiteral("Invalid"));
    QCOMPARE(nameValue->text(), QStringLiteral("Unavailable"));
    QVERIFY(certificatePathValue->isHidden());
    QCOMPARE(serialNumberValue->text(), QStringLiteral("Unavailable"));
    QVERIFY(widget.detailsText().contains(QStringLiteral("Status: Invalid")));
    QVERIFY(!widget.detailsText().contains(QStringLiteral("Certificate Path:")));
}

///
/// \brief Verifies a generated certificate populates fields and copyable text.
///
void TestCertificateDetailsWidget::generatedCertificateFillsDetailsAndDetailsText()
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

    CertificateDetailsWidget widget;
    widget.setCertificate(certificate, certificateFile);

    auto *statusValue = widget.findChild<QLabel *>(QStringLiteral("statusValue"));
    auto *nameValue = widget.findChild<QLabel *>(QStringLiteral("nameValue"));
    auto *keySizeValue = widget.findChild<QLabel *>(QStringLiteral("keySizeValue"));
    auto *certificatePathValue =
        widget.findChild<QLabel *>(QStringLiteral("certificatePathValue"));
    auto *serialNumberValue = widget.findChild<QLabel *>(QStringLiteral("serialNumberValue"));
    QVERIFY(statusValue);
    QVERIFY(nameValue);
    QVERIFY(keySizeValue);
    QVERIFY(certificatePathValue);
    QVERIFY(serialNumberValue);

    QCOMPARE(widget.certificate(), certificate);
    QCOMPARE(statusValue->text(), QStringLiteral("Trusted"));
    QCOMPARE(nameValue->text(), commonName);
    QCOMPARE(keySizeValue->text(), QStringLiteral("2048"));
    QVERIFY(!certificatePathValue->isHidden());
    QCOMPARE(certificatePathValue->text(), QDir::toNativeSeparators(certificateFile));
    QVERIFY(serialNumberValue->text() != QStringLiteral("Unavailable"));

    const QString textWithPath = widget.detailsText();
    QVERIFY(textWithPath.contains(commonName));
    QVERIFY(textWithPath.contains(QStringLiteral("Certificate Path:")));
    QVERIFY(textWithPath.contains(QDir::toNativeSeparators(certificateFile)));
    QVERIFY(textWithPath.contains(QStringLiteral("Serial Number:")));

    widget.setCertificate(certificate);
    QVERIFY(certificatePathValue->isHidden());
    QVERIFY(!widget.detailsText().contains(QStringLiteral("Certificate Path:")));

    QVERIFY(QFile::remove(certificateFile));
    QVERIFY(QFile::remove(privateKeyFile));
}

QTEST_MAIN(TestCertificateDetailsWidget)

#include "test_certificatedetailswidget.moc"
