// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_certificatedetailsdialog.cpp
/// \brief Unit tests for the certificate details dialog shell.
///

#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QTest>

#include "dialogs/certificatedetailsdialog.h"
#include "widgets/certificatedetailswidget.h"
#include "widgets/themedpushbutton.h"

///
/// \brief Tests the certificate details dialog shell behavior.
///
class TestCertificateDetailsDialog : public QObject
{
    Q_OBJECT

private slots:
    void forwardsCertificateAndCopiesWidgetText();
};

///
/// \brief Verifies the dialog delegates display state and copy text to its details widget.
///
void TestCertificateDetailsDialog::forwardsCertificateAndCopiesWidgetText()
{
    CertificateDetailsDialog dialog;
    const QByteArray certificate = QByteArrayLiteral("not a certificate");
    const QString certificatePath = QStringLiteral("C:/pki/server.der");
    dialog.setCertificate(certificate, certificatePath);

    auto *detailsWidget =
        dialog.findChild<CertificateDetailsWidget *>(QStringLiteral("detailsWidget"));
    auto *copyButton = dialog.findChild<ThemedPushButton *>(QStringLiteral("copyButton"));
    auto *exportButton = dialog.findChild<ThemedPushButton *>(QStringLiteral("exportButton"));
    QVERIFY(detailsWidget);
    QVERIFY(copyButton);
    QVERIFY(exportButton);

    QCOMPARE(detailsWidget->certificate(), certificate);
    QVERIFY(detailsWidget->detailsText().contains(QDir::toNativeSeparators(certificatePath)));
    QCOMPARE(copyButton->iconName(), QStringLiteral("copy"));
    QCOMPARE(exportButton->iconName(), QStringLiteral("export"));

    copyButton->click();
    QCOMPARE(QApplication::clipboard()->text(), detailsWidget->detailsText());
}

QTEST_MAIN(TestCertificateDetailsDialog)

#include "test_certificatedetailsdialog.moc"
