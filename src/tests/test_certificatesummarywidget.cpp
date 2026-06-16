// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include <QLabel>
#include <QPushButton>
#include <QSignalSpy>
#include <QTest>
#include <QWidget>

#include "widgets/certificatesummarywidget.h"

class TestCertificateSummaryWidget : public QObject
{
    Q_OBJECT

private slots:
    void hintModeHidesDetailsWhenEmpty();
    void inlineModeShowsPlaceholderWhenEmpty();
    void unreadableCertificateFillsDetails();
    void viewButtonEmitsViewRequested();
};

void TestCertificateSummaryWidget::hintModeHidesDetailsWhenEmpty()
{
    CertificateSummaryWidget widget;
    widget.setTitle(QStringLiteral("Server Certificate"));
    widget.setHint(QStringLiteral("Select an endpoint that provides a server certificate."));
    widget.clear();

    auto *hintLabel = widget.findChild<QLabel *>(QStringLiteral("hintLabel"));
    auto *detailsWidget = widget.findChild<QWidget *>(QStringLiteral("detailsWidget"));
    auto *viewButton = widget.findChild<QPushButton *>(QStringLiteral("viewButton"));
    QVERIFY(hintLabel);
    QVERIFY(detailsWidget);
    QVERIFY(viewButton);

    QVERIFY(!hintLabel->isHidden());
    QVERIFY(detailsWidget->isHidden());
    QVERIFY(!viewButton->isEnabled());
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
    auto *fingerprintEdit = widget.findChild<QLabel *>(QStringLiteral("fingerprintEdit"));
    auto *viewButton = widget.findChild<QPushButton *>(QStringLiteral("viewButton"));
    QVERIFY(subjectEdit);
    QVERIFY(validEdit);
    QVERIFY(fingerprintEdit);
    QVERIFY(viewButton);

    QCOMPARE(subjectEdit->text(), QStringLiteral("Unable to read certificate"));
    QCOMPARE(validEdit->text(), QStringLiteral("%1 bytes").arg(garbage.size()));
    QVERIFY(!fingerprintEdit->text().isEmpty());
    QVERIFY(viewButton->isEnabled());
    QCOMPARE(widget.certificate(), garbage);
}

void TestCertificateSummaryWidget::viewButtonEmitsViewRequested()
{
    CertificateSummaryWidget widget;
    widget.setCertificate(QByteArrayLiteral("not a certificate"));

    auto *viewButton = widget.findChild<QPushButton *>(QStringLiteral("viewButton"));
    QVERIFY(viewButton);
    QVERIFY(viewButton->isEnabled());

    QSignalSpy spy(&widget, &CertificateSummaryWidget::viewRequested);
    viewButton->click();
    QCOMPARE(spy.count(), 1);
}

QTEST_MAIN(TestCertificateSummaryWidget)

#include "test_certificatesummarywidget.moc"
