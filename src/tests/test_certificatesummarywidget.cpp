// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_certificatesummarywidget.cpp
/// \brief Unit tests for the certificate summary panel's empty, hint, and populated states.
///

#include <QLabel>
#include <QSignalSpy>
#include <QTest>
#include <QWidget>

#include "widgets/certificatesummarywidget.h"
#include "widgets/elidedlabel.h"
#include "widgets/themedtoolbutton.h"

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
    void viewDetailsButtonEmitsRequest();
    void longFieldsElideAndShowTooltips_data();
    void longFieldsElideAndShowTooltips();
    void placeholderTooltipTracksChanges();
};

void TestCertificateSummaryWidget::hintModeHidesDetailsWhenEmpty()
{
    CertificateSummaryWidget widget;
    widget.setTitle(QStringLiteral("Server Certificate"));
    widget.setHint(QStringLiteral("Select an endpoint that provides a server certificate."));
    widget.clear();

    auto *hintLabel = widget.findChild<QLabel *>(QStringLiteral("hintLabel"));
    auto *detailsWidget = widget.findChild<QWidget *>(QStringLiteral("detailsWidget"));
    auto *viewDetailsButton = widget.findChild<ThemedToolButton *>(QStringLiteral("viewDetailsButton"));
    QVERIFY(hintLabel);
    QVERIFY(detailsWidget);
    QVERIFY(viewDetailsButton);

    QVERIFY(!hintLabel->isHidden());
    QVERIFY(detailsWidget->isHidden());
    QCOMPARE(viewDetailsButton->text(), QStringLiteral("View details"));
    QCOMPARE(viewDetailsButton->iconName(), QStringLiteral("file-text"));
    QVERIFY(viewDetailsButton->linkStyle());
    QVERIFY(viewDetailsButton->styleSheet().isEmpty());
    QCOMPARE(viewDetailsButton->cursor().shape(), Qt::PointingHandCursor);
    QVERIFY(!viewDetailsButton->isEnabled());
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
    auto *viewDetailsButton = widget.findChild<ThemedToolButton *>(QStringLiteral("viewDetailsButton"));
    QVERIFY(hintLabel);
    QVERIFY(detailsWidget);
    QVERIFY(subjectEdit);
    QVERIFY(validIcon);
    QVERIFY(viewDetailsButton);

    QVERIFY(hintLabel->isHidden());
    QVERIFY(!detailsWidget->isHidden());
    QCOMPARE(subjectEdit->text(), QStringLiteral("No client certificate"));
    QVERIFY(validIcon->isHidden());
    QVERIFY(!viewDetailsButton->isEnabled());
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
    auto *viewDetailsButton = widget.findChild<ThemedToolButton *>(QStringLiteral("viewDetailsButton"));
    QVERIFY(subjectEdit);
    QVERIFY(validEdit);
    QVERIFY(serialNumberEdit);
    QVERIFY(viewDetailsButton);

    QCOMPARE(subjectEdit->text(), QStringLiteral("Unable to read certificate"));
    QCOMPARE(validEdit->text(), QStringLiteral("%1 bytes").arg(garbage.size()));
    QCOMPARE(serialNumberEdit->text(), QStringLiteral("Unavailable"));
    QVERIFY(viewDetailsButton->isEnabled());
    QCOMPARE(widget.certificate(), garbage);
}

void TestCertificateSummaryWidget::viewDetailsButtonEmitsRequest()
{
    CertificateSummaryWidget widget;
    widget.setCertificate(QByteArrayLiteral("not a certificate"));

    auto *viewDetailsButton =
        widget.findChild<ThemedToolButton *>(QStringLiteral("viewDetailsButton"));
    QVERIFY(viewDetailsButton);

    QSignalSpy spy(&widget, &CertificateSummaryWidget::viewDetailsRequested);
    viewDetailsButton->click();
    QCOMPARE(spy.count(), 1);
}

///
/// \brief Covers summary values that previously clipped without an ellipsis.
///
void TestCertificateSummaryWidget::longFieldsElideAndShowTooltips_data()
{
    QTest::addColumn<QString>("fieldName");
    QTest::newRow("subject") << QStringLiteral("subjectEdit");
    QTest::newRow("issuer") << QStringLiteral("issuerEdit");
    QTest::newRow("valid-until") << QStringLiteral("validEdit");
}

///
/// \brief Verifies elision follows panel width and cleared fields lose their tooltips.
///
void TestCertificateSummaryWidget::longFieldsElideAndShowTooltips()
{
    QFETCH(QString, fieldName);
    CertificateSummaryWidget widget;
    widget.setCertificate(QByteArrayLiteral("invalid"));
    auto *value = widget.findChild<ElidedLabel *>(fieldName);
    QVERIFY(value);
    const QString text = QStringLiteral("OpenUaExplorer@long-host-name.local (self-signed)").repeated(5);
    value->setText(text);
    widget.resize(520, 260);
    widget.show();

    QTRY_VERIFY(value->isElided());
    QCOMPARE(value->text(), text);
    QCOMPARE(value->toolTip(), text);
    QVERIFY(!value->wordWrap());
    QCOMPARE(widget.width(), 520);

    widget.resize(520 + value->fontMetrics().horizontalAdvance(text), widget.height());
    QTRY_VERIFY(!value->isElided());
    QVERIFY(value->toolTip().isEmpty());
    widget.resize(520, 260);
    QTRY_VERIFY(value->isElided());
    QCOMPARE(value->toolTip(), text);

    widget.clear();
    QTRY_VERIFY(!value->isElided());
    QVERIFY(value->text().isEmpty());
    QVERIFY(value->toolTip().isEmpty());
}

///
/// \brief Verifies replacement and hint states cannot retain a previous hover value.
///
void TestCertificateSummaryWidget::placeholderTooltipTracksChanges()
{
    CertificateSummaryWidget widget;
    const QString text = QStringLiteral("No client certificate is configured. ").repeated(10);
    widget.setEmptyText(text);
    widget.resize(520, 260);
    widget.show();
    auto *value = widget.findChild<ElidedLabel *>(QStringLiteral("subjectEdit"));
    QVERIFY(value);
    QTRY_VERIFY(value->isElided());
    QCOMPARE(value->toolTip(), text);

    const QString replacement = text + QStringLiteral("Select a certificate.");
    widget.setEmptyText(replacement);
    QCOMPARE(value->toolTip(), replacement);
    widget.setHint(QStringLiteral("Select an endpoint."));
    QVERIFY(value->toolTip().isEmpty());
    QVERIFY(value->text().isEmpty());
    widget.setHint({});
    QCOMPARE(value->text(), replacement);
    widget.setEmptyText(QStringLiteral("None"));
    QTRY_VERIFY(!value->isElided());
    QVERIFY(value->toolTip().isEmpty());
}

QTEST_MAIN(TestCertificateSummaryWidget)

#include "test_certificatesummarywidget.moc"
