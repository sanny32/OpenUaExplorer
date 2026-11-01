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
#include <QScreen>
#include <QTest>

#include "dialogs/certificatedetailsdialog.h"
#include "widgets/certificatedetailswidget.h"
#include "widgets/elidedlabel.h"
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
    void openingFitsContentToScreen_data();
    void openingFitsContentToScreen();
};

///
/// \brief Verifies the dialog delegates display state and copy text to its details widget.
///
void TestCertificateDetailsDialog::forwardsCertificateAndCopiesWidgetText()
{
    CertificateDetailsDialog dialog;
    const QByteArray certificate = QByteArrayLiteral("not a certificate");
    const QString certificatePath =
        QStringLiteral("C:/pki/%1/server.der").arg(QString(512, QLatin1Char('a')));
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
/// \brief Exercises elision at different dialog widths and with unbroken values.
///
void TestCertificateDetailsDialog::longValuesRemainReadable_data()
{
    QTest::addColumn<int>("width");
    QTest::addColumn<QString>("value");
    QTest::newRow("path")
        << 680 << QStringLiteral("/Application Support/certificates").repeated(20);
    QTest::newRow("unbroken") << 680 << QString(1200, QLatin1Char('a'));
    QTest::newRow("wide")
        << 900 << QStringLiteral("[6, urn:host:application], ").repeated(30);
}

///
/// \brief Verifies long fields stay on one line and expose their full value on hover.
///
void TestCertificateDetailsDialog::longValuesRemainReadable()
{
    QFETCH(int, width);
    QFETCH(QString, value);
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
    QTRY_COMPARE(scroll->verticalScrollBar()->maximum(), 0);
    QCOMPARE(scroll->horizontalScrollBar()->maximum(), 0);
    const auto values = details->findChildren<ElidedLabel *>();
    QVERIFY(!values.isEmpty());
    for (ElidedLabel *label : values) {
        QVERIFY(!label->wordWrap());
        QTRY_VERIFY2(label->isElided(), qPrintable(label->objectName()));
        QCOMPARE(label->text(), value);
        QCOMPARE(label->toolTip(), value);
    }
}

///
/// \brief Covers certificates that fit the screen and those that exceed it.
///
void TestCertificateDetailsDialog::openingFitsContentToScreen_data()
{
    QTest::addColumn<int>("pathRepetitions");
    QTest::addColumn<bool>("needsScrolling");
    QTest::addColumn<int>("frameWidth");
    QTest::newRow("ordinary") << 2 << false << 0;
    QTest::newRow("framed-viewport") << 2 << false << 7;
    QTest::newRow("long-value") << 200 << false << 0;
    QTest::newRow("oversized-font") << 2 << true << 0;
}

///
/// \brief Verifies opening grows the dialog and retains scrolling only beyond screen limits.
///
void TestCertificateDetailsDialog::openingFitsContentToScreen()
{
    QFETCH(int, pathRepetitions);
    QFETCH(bool, needsScrolling);
    QFETCH(int, frameWidth);
    CertificateDetailsDialog dialog;
    dialog.setCertificate(QByteArrayLiteral("invalid"),
                          QStringLiteral("/Application Support/certificates").repeated(pathRepetitions));
    auto *details = dialog.findChild<CertificateDetailsWidget *>();
    QVERIFY(details);
    if (needsScrolling) {
        QFont font = details->font();
        font.setPixelSize(dialog.screen()->availableGeometry().height() / 8);
        details->setFont(font);
    }
    auto *scroll = details->findChild<QScrollArea *>();
    QVERIFY(scroll);
    scroll->setFrameShape(frameWidth > 0 ? QFrame::Box : QFrame::NoFrame);
    scroll->setLineWidth(frameWidth);
    dialog.resize(680, 250);
    dialog.show();
    QCoreApplication::processEvents();

    QCOMPARE(dialog.width(), 680);
    QVERIFY(dialog.height() > 250);
    QVERIFY(dialog.frameGeometry().height() <= dialog.screen()->availableGeometry().height());
    if (needsScrolling) {
        QTRY_VERIFY(scroll->verticalScrollBar()->maximum() > 0);
    } else {
        QTRY_COMPARE(scroll->verticalScrollBar()->maximum(), 0);
        QCOMPARE(scroll->horizontalScrollBar()->maximum(), 0);
        QVERIFY(details->height() >= details->contentHeightForWidth(details->width()));
    }
}

QTEST_MAIN(TestCertificateDetailsDialog)

#include "test_certificatedetailsdialog.moc"
