// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_dialogopcuainfo.cpp
/// \brief UI tests for the OPC UA information dialog.
///

#include <QApplication>
#include <QClipboard>
#include <QLabel>
#include <QPushButton>
#include <QTest>

#include "dialogs/dialogopcuainfo.h"

///
/// \brief Unit tests for the OPC UA information dialog.
///
class TestDialogOpcUaInfo : public QObject
{
    Q_OBJECT

private slots:
    void showsSectionsAndRuntimeValues();
    void exposesExternalLinks();
    void copiesDiagnosticSummary();
};

///
/// \brief Verifies the dialog title, section labels and runtime fields.
///
void TestDialogOpcUaInfo::showsSectionsAndRuntimeValues()
{
    DialogOpcUaInfo dialog;

    QCOMPARE(dialog.windowTitle(), QStringLiteral("OPC UA Info"));
    QVERIFY(dialog.findChild<QLabel *>(QStringLiteral("stackTitleLabel")));
    QVERIFY(dialog.findChild<QLabel *>(QStringLiteral("securityTitleLabel")));
    QVERIFY(dialog.findChild<QLabel *>(QStringLiteral("specificationTitleLabel")));
    QVERIFY(dialog.findChild<QLabel *>(QStringLiteral("resourcesTitleLabel")));

    auto *sdkValue = dialog.findChild<QLabel *>(QStringLiteral("sdkValue"));
    auto *sdkVersionValue = dialog.findChild<QLabel *>(QStringLiteral("sdkVersionValue"));
    auto *securityPoliciesValue =
        dialog.findChild<QLabel *>(QStringLiteral("securityPoliciesValue"));
    QVERIFY(sdkValue);
    QVERIFY(sdkVersionValue);
    QVERIFY(securityPoliciesValue);

    QCOMPARE(sdkValue->text(), QStringLiteral("open62541"));
    QCOMPARE(sdkVersionValue->text(), QStringLiteral("Not available"));
    QVERIFY(!dialog.findChild<QLabel *>(QStringLiteral("backendValue")));
    QVERIFY(!securityPoliciesValue->text().isEmpty());
}

///
/// \brief Resource labels expose the expected external URLs.
///
void TestDialogOpcUaInfo::exposesExternalLinks()
{
    DialogOpcUaInfo dialog;

    auto *foundationLink = dialog.findChild<QLabel *>(QStringLiteral("foundationLinkLabel"));
    auto *specificationLink =
        dialog.findChild<QLabel *>(QStringLiteral("specificationLinkLabel"));
    QVERIFY(foundationLink);
    QVERIFY(specificationLink);

    QVERIFY(foundationLink->openExternalLinks());
    QVERIFY(specificationLink->openExternalLinks());
    QVERIFY(foundationLink->text().contains(QStringLiteral("https://opcfoundation.org")));
    QVERIFY(specificationLink->text().contains(
        QStringLiteral("https://reference.opcfoundation.org")));
}

///
/// \brief Copy Info places the same diagnostic summary on the clipboard.
///
void TestDialogOpcUaInfo::copiesDiagnosticSummary()
{
    DialogOpcUaInfo dialog;
    auto *copyButton = dialog.findChild<QPushButton *>(QStringLiteral("copyButton"));
    QVERIFY(copyButton);

    copyButton->click();
    const QString clipboardText = QApplication::clipboard()->text();

    QCOMPARE(clipboardText, dialog.copyText());
    QVERIFY(clipboardText.contains(QStringLiteral("OPC UA Info")));
    QVERIFY(clipboardText.contains(QStringLiteral("OPC UA SDK: open62541")));
    QVERIFY(clipboardText.contains(QStringLiteral("Security Policies:")));
    QVERIFY(clipboardText.contains(QStringLiteral("OPC UA Specification: Part 1-14")));
    QVERIFY(clipboardText.contains(QStringLiteral("https://opcfoundation.org")));
}

QTEST_MAIN(TestDialogOpcUaInfo)

#include "test_dialogopcuainfo.moc"
