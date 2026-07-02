// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_certificatesdialog.cpp
/// \brief UI tests for the PKI certificate management dialog.
///

#include <QAbstractItemModel>
#include <QAbstractScrollArea>
#include <QDialogButtonBox>
#include <QFile>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QScrollBar>
#include <QStyle>
#include <QTableView>
#include <QTest>

#include "dialogs/certificatesdialog.h"
#include "opcua/pkimanager.h"
#include "widgets/dialogbuttonbox.h"
#include "widgets/tableview.h"

///
/// \brief Drives the certificates dialog through its client and trust-store flows.
///
class TestCertificatesDialog : public QObject
{
    Q_OBJECT

private slots:
    void showsClientCertificate();
    void rejectsTrustedCertificateOnApply();
    void trustStoreTableKeepsLongCellsScrollable();
};

namespace {

/// \brief Generates a client certificate and returns its DER bytes, skipping on failure.
/// \param commonName Subject common name for the generated certificate.
QByteArray generateCertificate(const QString &commonName = PkiManager::clientCertificateCommonName())
{
    PkiManager pki;
    QString certificateFile;
    QString privateKeyFile;
    QString error;
    if (!pki.generateClientCertificate(commonName,
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

///
/// \brief Long trust-store fields keep their full column width behind a horizontal scrollbar.
///
void TestCertificatesDialog::trustStoreTableKeepsLongCellsScrollable()
{
    const QString commonName(64, QLatin1Char('W'));
    const QByteArray der = generateCertificate(commonName);
    if (der.isEmpty())
        QSKIP("Certificate generation is unavailable.");

    PkiManager pki;
    QString error;
    QVERIFY(pki.setCertificateCategory(der, PkiManager::Category::Trusted, &error));

    CertificatesDialog dialog;
    auto *table = dialog.findChild<TableView *>(QStringLiteral("tableView"));
    QVERIFY(table);
    QVERIFY(table->fullTextHorizontalScroll());

    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));

    QCOMPARE(table->sizeAdjustPolicy(), QAbstractScrollArea::AdjustIgnored);
    QCOMPARE(table->textElideMode(), Qt::ElideNone);
    QCOMPARE(table->horizontalScrollBarPolicy(), Qt::ScrollBarAsNeeded);
    QCOMPARE(table->horizontalHeader()->sectionResizeMode(0), QHeaderView::Interactive);
    QCOMPARE(table->horizontalHeader()->sectionResizeMode(1), QHeaderView::Interactive);

    const int textWidth = table->fontMetrics().horizontalAdvance(commonName);
    const int textMargin = table->style()->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, table) + 1;
    const int minimumFullTextWidth = textWidth + textMargin * 2 + 16;
    QVERIFY(table->columnWidth(0) >= minimumFullTextWidth);
    QVERIFY(table->columnWidth(1) >= minimumFullTextWidth);
    QVERIFY(table->horizontalHeader()->length() > table->viewport()->width());
    QVERIFY(table->horizontalScrollBar()->maximum() > 0);

    pki.removeCertificate(der);
}

QTEST_MAIN(TestCertificatesDialog)

#include "test_certificatesdialog.moc"
