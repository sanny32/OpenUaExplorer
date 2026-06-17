// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_certificatesummarywidget.cpp
/// \brief Unit tests for the certificate summary panel's empty, hint, and populated states.
///

#include <QLabel>
#include <QTest>
#include <QWidget>

#include "widgets/certificatesummarywidget.h"

///
/// \brief Tests the panel's hint/inline empty states and populated details.
///
class TestCertificateSummaryWidget : public QObject
{
    Q_OBJECT

private slots:
    void hintModeHidesDetailsWhenEmpty();
    void inlineModeShowsPlaceholderWhenEmpty();
    void unreadableCertificateFillsDetails();
};

void TestCertificateSummaryWidget::hintModeHidesDetailsWhenEmpty()
{
    CertificateSummaryWidget widget;
    widget.setTitle(QStringLiteral("Server Certificate"));
    widget.setHint(QStringLiteral("Select an endpoint that provides a server certificate."));
    widget.clear();

    auto *hintLabel = widget.findChild<QLabel *>(QStringLiteral("hintLabel"));
    auto *detailsWidget = widget.findChild<QWidget *>(QStringLiteral("detailsWidget"));
    QVERIFY(hintLabel);
    QVERIFY(detailsWidget);

    QVERIFY(!hintLabel->isHidden());
    QVERIFY(detailsWidget->isHidden());
    QVERIFY(!widget.findChild<QWidget *>(QStringLiteral("viewButton")));
    QVERIFY(widget.certificate().isEmpty());
}

void TestCertificateSummaryWidget::inlineModeShowsPlaceholderWhenEmpty()
{
    CertificateSummaryWidget widget;
    widget.setTitle(QStringLiteral("Client Certificate"));
    widget.setEmptyText(QStringLiteral("No client certificate"));
    widget.clear();

    auto *hintLabel = widget.findChild<QLabel *>(QStringLiteral("hintLabel"));
    auto *detailsWidget = widget.findChild<QWidget *>(QStringLiteral("detailsWidget"));
    auto *subjectEdit = widget.findChild<QLabel *>(QStringLiteral("subjectEdit"));
    auto *validIcon = widget.findChild<QLabel *>(QStringLiteral("validIcon"));
    QVERIFY(hintLabel);
    QVERIFY(detailsWidget);
    QVERIFY(subjectEdit);
    QVERIFY(validIcon);

    QVERIFY(hintLabel->isHidden());
    QVERIFY(!detailsWidget->isHidden());
    QCOMPARE(subjectEdit->text(), QStringLiteral("No client certificate"));
    QVERIFY(validIcon->isHidden());
}

void TestCertificateSummaryWidget::unreadableCertificateFillsDetails()
{
    CertificateSummaryWidget widget;
    widget.setTitle(QStringLiteral("Client Certificate"));
    widget.setEmptyText(QStringLiteral("No client certificate"));

    const QByteArray garbage = QByteArrayLiteral("not a certificate");
    widget.setCertificate(garbage);

    auto *subjectEdit = widget.findChild<QLabel *>(QStringLiteral("subjectEdit"));
    auto *validEdit = widget.findChild<QLabel *>(QStringLiteral("validEdit"));
    auto *serialNumberEdit = widget.findChild<QLabel *>(QStringLiteral("serialNumberEdit"));
    QVERIFY(subjectEdit);
    QVERIFY(validEdit);
    QVERIFY(serialNumberEdit);

    QCOMPARE(subjectEdit->text(), QStringLiteral("Unable to read certificate"));
    QCOMPARE(validEdit->text(), QStringLiteral("%1 bytes").arg(garbage.size()));
    QCOMPARE(serialNumberEdit->text(), QStringLiteral("Unavailable"));
    QCOMPARE(widget.certificate(), garbage);
}

QTEST_MAIN(TestCertificateSummaryWidget)

#include "test_certificatesummarywidget.moc"
