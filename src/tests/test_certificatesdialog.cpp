// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_certificatesdialog.cpp
/// \brief UI tests for the PKI certificate management dialog.
///

#include <QAbstractItemModel>
#include <QDialogButtonBox>
#include <QFile>
#include <QLabel>
#include <QPushButton>
#include <QTableView>
#include <QTest>

#include "dialogs/certificatesdialog.h"
#include "opcua/pkimanager.h"
#include "widgets/dialogbuttonbox.h"

///
/// \brief Drives the certificates dialog through its client and trust-store flows.
///
class TestCertificatesDialog : public QObject
{
    Q_OBJECT

private slots:
    void showsClientCertificate();
    void rejectsTrustedCertificateOnApply();
};

namespace {

/// \brief Generates a client certificate and returns its DER bytes, skipping on failure.
QByteArray generateCertificate()
{
    PkiManager pki;
    QString certificateFile;
    QString privateKeyFile;
    QString error;
    if (!pki.generateClientCertificate(PkiManager::clientCertificateCommonName(),
                                       PkiManager::applicationUri(),
                                       &certificateFile, &privateKeyFile, &error)) {
        return {};
    }
    QFile file(certificateFile);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

} // namespace

///
/// \brief Verifies the client-certificate card reflects a present, valid identity.
///
void TestCertificatesDialog::showsClientCertificate()
{
    if (generateCertificate().isEmpty())
        QSKIP("Certificate generation is unavailable.");

    CertificatesDialog dialog;
    auto *fileValue = dialog.findChild<QLabel *>(QStringLiteral("certificateFileValue"));
    auto *statusValue = dialog.findChild<QLabel *>(QStringLiteral("statusValue"));
    auto *importButton = dialog.findChild<QPushButton *>(QStringLiteral("importClientButton"));
    auto *removeButton = dialog.findChild<QPushButton *>(QStringLiteral("removeClientButton"));
    QVERIFY(fileValue);
    QVERIFY(statusValue);
    QVERIFY(importButton);
    QVERIFY(removeButton);

    QVERIFY(fileValue->text().endsWith(QStringLiteral(".der")));
    QCOMPARE(statusValue->text(), QStringLiteral("Valid"));
    QVERIFY(!importButton->isEnabled());
    QVERIFY(removeButton->isEnabled());
}

///
/// \brief Rejecting a trusted certificate and applying moves it to the rejected store.
///
void TestCertificatesDialog::rejectsTrustedCertificateOnApply()
{
    const QByteArray der = generateCertificate();
    if (der.isEmpty())
        QSKIP("Certificate generation is unavailable.");

    PkiManager pki;
    const QString fingerprint = PkiManager::fingerprint(der);
    QString error;
    QVERIFY(pki.setCertificateCategory(der, PkiManager::Category::Trusted, &error));

    CertificatesDialog dialog;
    auto *table = dialog.findChild<QTableView *>(QStringLiteral("tableView"));
    auto *rejectButton = dialog.findChild<QPushButton *>(QStringLiteral("rejectButton"));
    auto *buttonBox = dialog.findChild<DialogButtonBox *>(QStringLiteral("buttonBox"));
    QVERIFY(table);
    QVERIFY(rejectButton);
    QVERIFY(buttonBox);
    QPushButton *applyButton = buttonBox->button(QDialogButtonBox::Apply);
    QVERIFY(applyButton);

    QAbstractItemModel *model = table->model();
    int targetRow = -1;
    for (int row = 0; row < model->rowCount(); ++row) {
        const QByteArray rowDer = model->index(row, 0).data(Qt::UserRole).toByteArray();
        if (PkiManager::fingerprint(rowDer) == fingerprint) {
            targetRow = row;
            break;
        }
    }
    QVERIFY(targetRow >= 0);

    table->selectRow(targetRow);
    QVERIFY(rejectButton->isEnabled());
    rejectButton->click();
    QVERIFY(applyButton->isEnabled());
    applyButton->click();

    const auto contains = [&fingerprint](const QList<QByteArray> &certificates) {
        for (const QByteArray &certificate : certificates)
            if (PkiManager::fingerprint(certificate) == fingerprint)
                return true;
        return false;
    };
    QVERIFY(!contains(pki.certificates(PkiManager::Category::Trusted)));
    QVERIFY(contains(pki.certificates(PkiManager::Category::Rejected)));

    pki.removeCertificate(der);
}

QTEST_MAIN(TestCertificatesDialog)

#include "test_certificatesdialog.moc"
