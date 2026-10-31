// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_certificatedetailsdialog.cpp
/// \brief Unit tests for the certificate details dialog shell.
///

#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
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
    void longValuesRemainReadable_data();
    void longValuesRemainReadable();
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

///
/// \brief Exercises wrapping at different dialog widths and with unbroken values.
///
void TestCertificateDetailsDialog::longValuesRemainReadable_data()
{
    QTest::addColumn<int>("width");
    QTest::addColumn<QString>("value");
    QTest::addColumn<bool>("unbroken");
    QTest::newRow("path")
        << 680 << QStringLiteral("/Application Support/certificates").repeated(20) << false;
    QTest::newRow("unbroken") << 680 << QString(1200, QLatin1Char('a')) << true;
    QTest::newRow("wide")
        << 900 << QStringLiteral("[6, urn:host:application], ").repeated(30) << false;
}

///
/// \brief Verifies long fields receive their full wrapped height and remain reachable.
///
void TestCertificateDetailsDialog::longValuesRemainReadable()
{
    QFETCH(int, width);
    QFETCH(QString, value);
    QFETCH(bool, unbroken);
    CertificateDetailsDialog dialog;
    dialog.setCertificate(QByteArrayLiteral("invalid"), value);
    auto *details = dialog.findChild<CertificateDetailsWidget *>();
    QVERIFY(details);
    for (QLabel *label : details->findChildren<QLabel *>()) {
        if (label->objectName().endsWith(QStringLiteral("Value"))
            && label->objectName() != QStringLiteral("statusValue"))
            label->setText(value);
    }
    dialog.resize(width, 560);
    dialog.show();
    QCoreApplication::processEvents();

    QCOMPARE(dialog.width(), width);
    auto *scroll = details->findChild<QScrollArea *>();
    QVERIFY(scroll);
    if (unbroken) {
        QTRY_VERIFY(scroll->horizontalScrollBar()->maximum() > 0);
    } else {
        QTRY_VERIFY(scroll->verticalScrollBar()->maximum() > 0);
        QCOMPARE(scroll->horizontalScrollBar()->maximum(), 0);
    }
    for (QLabel *label : details->findChildren<QLabel *>()) {
        if (!label->objectName().endsWith(QStringLiteral("Value"))
            || label->objectName() == QStringLiteral("statusValue"))
            continue;
        QVERIFY2(label->wordWrap(), qPrintable(label->objectName()));
        if (unbroken) {
            QVERIFY2(label->width() >= label->fontMetrics().horizontalAdvance(value),
                     qPrintable(label->objectName()));
        } else {
            QVERIFY2(label->heightForWidth(label->width()) > label->fontMetrics().height(),
                     qPrintable(label->objectName()));
        }
        QVERIFY2(label->height() >= label->heightForWidth(label->width()),
                 qPrintable(label->objectName()));
    }
    auto *thumbprint = details->findChild<QLabel *>(QStringLiteral("thumbprintValue"));
    QVERIFY(thumbprint);
    scroll->ensureWidgetVisible(thumbprint);
    QCoreApplication::processEvents();
    if (!unbroken)
        QVERIFY(scroll->verticalScrollBar()->value() > 0);
}

QTEST_MAIN(TestCertificateDetailsDialog)

#include "test_certificatedetailsdialog.moc"
