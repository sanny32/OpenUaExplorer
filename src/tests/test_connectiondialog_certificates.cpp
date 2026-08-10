// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_connectiondialog_certificates.cpp
/// \brief Tests ConnectionDialog certificate and trust behavior.
///

#include <QTest>

#include "test_connectiondialog_support.h"

///
/// \brief Tests the related behavior in an isolated Qt Test executable.
///
class TestConnectionDialogCertificates : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();
    void clientCertificateActionFollowsSelection();
    void clientCertificateSelectorFillsRow();
    void certificateStatusRowsAlignBadgeToRight();
    void serverTrustStateFollowsTrustList();
    void trustIsRefusedForACertificateOutsideItsValidity();

private:
    QTemporaryDir _settingsDirectory;
};

void TestConnectionDialogCertificates::initTestCase()
{
    QVERIFY(_settingsDirectory.isValid());
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName(QStringLiteral("OpenUaExplorerTests"));
    QCoreApplication::setApplicationName(QStringLiteral("TestConnectionDialogCertificates"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       _settingsDirectory.path());
}

void TestConnectionDialogCertificates::cleanup()
{
    while (QGuiApplication::overrideCursor())
        QGuiApplication::restoreOverrideCursor();
    SettingsStore settings;
    settings.clear();
}


void TestConnectionDialogCertificates::clientCertificateActionFollowsSelection()
{
    ConnectionDialog dialog;
    auto *certificateMode = dialog.findChild<QComboBox *>(
        QStringLiteral("clientCertificateComboBox"));
    auto *certificateAction = dialog.findChild<QPushButton *>(
        QStringLiteral("clientCertificateViewButton"));
    auto *certificateEdit = dialog.findChild<QLineEdit *>(QStringLiteral("certificateEdit"));
    auto *privateKeyEdit = dialog.findChild<QLineEdit *>(QStringLiteral("privateKeyEdit"));
    auto *certificateWidget = dialog.findChild<CertificateSummaryWidget *>(
        QStringLiteral("clientCertificateWidget"));
    QVERIFY(certificateMode);
    QVERIFY(certificateAction);
    QVERIFY(certificateEdit);
    QVERIFY(privateKeyEdit);
    QVERIFY(certificateWidget);

    certificateMode->setCurrentIndex(0);
    QCOMPARE(certificateAction->text(), QStringLiteral("Generate..."));

    certificateMode->setCurrentIndex(1);
    QCOMPARE(certificateAction->text(), QStringLiteral("Import..."));

    PkiManager pki;
    QString preflightCertificateFile;
    QString preflightPrivateKeyFile;
    QString preflightError;
    if (!pki.generateClientCertificate(
            PkiManager::clientCertificateCommonName(),
            PkiManager::applicationUri(),
            &preflightCertificateFile, &preflightPrivateKeyFile, &preflightError)) {
        QSKIP(qPrintable(preflightError));
    }
    QVERIFY(QFile::remove(preflightCertificateFile));
    QVERIFY(QFile::remove(preflightPrivateKeyFile));

    certificateMode->setCurrentIndex(0);
    QTest::mouseClick(certificateAction, Qt::LeftButton);
    const ConnectionProfile profile = dialog.profile();
    QVERIFY(QFile::exists(profile.clientCertificateFile));
    QVERIFY(QFile::exists(profile.privateKeyFile));
    QCOMPARE(certificateMode->currentIndex(), 0);
    QCOMPARE(certificateMode->itemText(0),
             QStringLiteral("Auto-generated (%1)")
                 .arg(QFileInfo(profile.clientCertificateFile).fileName()));
    QVERIFY(certificateEdit->text().isEmpty());
    QVERIFY(privateKeyEdit->text().isEmpty());

    QFile generatedCertificate(profile.clientCertificateFile);
    QVERIFY(generatedCertificate.open(QIODevice::ReadOnly));
    const QList<QSslCertificate> chain =
        QSslCertificate::fromData(generatedCertificate.readAll(), QSsl::Der);
    generatedCertificate.close();
    QVERIFY(!chain.isEmpty());
    QCOMPARE(certificateWidget->certificate(), chain.constFirst().toDer());

    QVERIFY(QFile::remove(profile.clientCertificateFile));
    QVERIFY(QFile::remove(profile.privateKeyFile));
}

void TestConnectionDialogCertificates::clientCertificateSelectorFillsRow()
{
    ConnectionDialog dialog;
    auto *layout = dialog.findChild<QHBoxLayout *>(QStringLiteral("clientCertificateLayout"));
    auto *certificateMode = dialog.findChild<QComboBox *>(
        QStringLiteral("clientCertificateComboBox"));
    auto *certificateAction = dialog.findChild<QPushButton *>(
        QStringLiteral("clientCertificateViewButton"));
    QVERIFY(layout);
    QVERIFY(certificateMode);
    QVERIFY(certificateAction);

    QCOMPARE(layout->count(), 3);
    QCOMPARE(layout->stretch(1), 1);
    QCOMPARE(layout->itemAt(1)->widget(), certificateMode);
    QCOMPARE(layout->itemAt(2)->widget(), certificateAction);
}

void TestConnectionDialogCertificates::certificateStatusRowsAlignBadgeToRight()
{
    ConnectionDialog dialog;

    verifyRightAlignedCertificateStatus(dialog, QStringLiteral("clientCertificateWidget"));
    verifyRightAlignedCertificateStatus(dialog, QStringLiteral("serverCertificateWidget"));
}

void TestConnectionDialogCertificates::serverTrustStateFollowsTrustList()
{
    const QByteArray certificate = generateCertificate();
    if (certificate.isEmpty())
        QSKIP("Certificate generation is unavailable.");

    DialogFakeBackend backend;
    ConnectionDialog dialog;
    dialog.setBackend(&backend);

    auto *trustSection = dialog.findChild<QWidget *>(QStringLiteral("serverTrustSection"));
    auto *trustStatus = dialog.findChild<QLabel *>(QStringLiteral("serverTrustStatusLabel"));
    auto *trustButton = dialog.findChild<QPushButton *>(QStringLiteral("serverTrustButton"));
    QVERIFY(trustSection);
    QVERIFY(trustStatus);
    QVERIFY(trustButton);

    // Without an endpoint there is no certificate to reason about, so the section stays away.
    QVERIFY(!trustSection->isVisibleTo(&dialog));

    EndpointInfo endpoint = makeDialogEndpoint(QStringLiteral("opc.tcp://localhost:4840"));
    endpoint.securityMode = QStringLiteral("Sign & Encrypt");
    endpoint.securityModeValue = 3;
    endpoint.serverCertificate = certificate;
    emit backend.endpointsDiscovered({endpoint}, {});

    QVERIFY(trustSection->isVisibleTo(&dialog));
    QVERIFY(!isInTrustList(certificate));
    QCOMPARE(trustStatus->text(), QStringLiteral("Not in trust list"));
    QCOMPARE(trustButton->text(), QStringLiteral("Trust"));

    // Declining the confirmation leaves the trust list untouched.
    answerNextQuestion(DialogButtonBox::No);
    QTest::mouseClick(trustButton, Qt::LeftButton);
    QVERIFY(!isInTrustList(certificate));
    QCOMPARE(trustButton->text(), QStringLiteral("Trust"));

    answerNextQuestion(DialogButtonBox::Yes);
    QTest::mouseClick(trustButton, Qt::LeftButton);
    QVERIFY(isInTrustList(certificate));
    QCOMPARE(trustStatus->text(), QStringLiteral("In trust list"));
    QCOMPARE(trustButton->text(), QStringLiteral("Remove from trust list"));

    answerNextQuestion(DialogButtonBox::No);
    QTest::mouseClick(trustButton, Qt::LeftButton);
    QVERIFY(isInTrustList(certificate));
    QCOMPARE(trustButton->text(), QStringLiteral("Remove from trust list"));

    answerNextQuestion(DialogButtonBox::Yes);
    QTest::mouseClick(trustButton, Qt::LeftButton);
    QVERIFY(!isInTrustList(certificate));
    QCOMPARE(trustStatus->text(), QStringLiteral("Not in trust list"));
    QCOMPARE(trustButton->text(), QStringLiteral("Trust"));
}

void TestConnectionDialogCertificates::trustIsRefusedForACertificateOutsideItsValidity()
{
    DialogFakeBackend backend;
    ConnectionDialog dialog;
    dialog.setBackend(&backend);

    auto *trustButton = dialog.findChild<QPushButton *>(QStringLiteral("serverTrustButton"));
    QVERIFY(trustButton);

    // A certificate that fails the validity check keeps failing it on every connection, so
    // trusting it would change nothing.
    EndpointInfo endpoint = makeDialogEndpoint(QStringLiteral("opc.tcp://localhost:4840"));
    endpoint.securityMode = QStringLiteral("Sign & Encrypt");
    endpoint.securityModeValue = 3;
    endpoint.serverCertificate = QByteArrayLiteral("not-a-certificate");
    emit backend.endpointsDiscovered({endpoint}, {});

    QCOMPARE(trustButton->text(), QStringLiteral("Trust"));
    QVERIFY(!trustButton->isEnabled());
    QVERIFY(!trustButton->toolTip().isEmpty());
}

QTEST_MAIN(TestConnectionDialogCertificates)

#include "test_connectiondialog_certificates.moc"
