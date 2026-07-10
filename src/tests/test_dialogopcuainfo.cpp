// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_dialogopcuainfo.cpp
/// \brief UI tests for the OPC UA information dialog.
///

#include <QApplication>
#include <QClipboard>
#include <QImage>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QTest>

#include "application.h"
#include "appsettings.h"
#include "dialogs/dialogopcuainfo.h"

///
/// \brief Unit tests for the OPC UA information dialog.
///
class TestDialogOpcUaInfo : public QObject
{
    Q_OBJECT

private slots:
    void showsSectionsAndRuntimeValues();
    void logoStaysVisibleOnDarkPalette();
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

    auto *logo = dialog.findChild<QLabel *>(QStringLiteral("logoLabel"));
    auto *sdkValue = dialog.findChild<QLabel *>(QStringLiteral("sdkValue"));
    auto *sdkVersionValue = dialog.findChild<QLabel *>(QStringLiteral("sdkVersionValue"));
    auto *securityPoliciesValue =
        dialog.findChild<QLabel *>(QStringLiteral("securityPoliciesValue"));
    QVERIFY(logo);
    QVERIFY(sdkValue);
    QVERIFY(sdkVersionValue);
    QVERIFY(securityPoliciesValue);

    QVERIFY(!logo->pixmap().isNull());
    QCOMPARE(sdkValue->text(), QStringLiteral("open62541"));
#ifdef OPCUA_OPEN62541_VERSION
    const QString expectedSdkVersion = QStringLiteral(OPCUA_OPEN62541_VERSION).isEmpty()
        ? QStringLiteral("Not available")
        : QStringLiteral(OPCUA_OPEN62541_VERSION);
#else
    const QString expectedSdkVersion = QStringLiteral("Not available");
#endif
    QCOMPARE(sdkVersionValue->text(), expectedSdkVersion);
    QVERIFY(!dialog.findChild<QLabel *>(QStringLiteral("backendValue")));
    QVERIFY(!securityPoliciesValue->text().isEmpty());
}

///
/// \brief The OPC UA logo keeps visible light pixels on a dark palette.
///
void TestDialogOpcUaInfo::logoStaysVisibleOnDarkPalette()
{
    const AppSettings::ThemeMode previousMode = AppSettings().themeMode();
    theApp()->theme().setColorSchemePreference(AppSettings::ThemeMode::Dark);
    if (!theApp()->theme().isDark()) {
        theApp()->theme().setColorSchemePreference(previousMode);
        QSKIP("Dark application theme is not available on this platform.");
    }

    DialogOpcUaInfo dialog;

    auto *logo = dialog.findChild<QLabel *>(QStringLiteral("logoLabel"));
    QVERIFY(logo);
    const QPixmap pixmap = logo->pixmap();
    QVERIFY(!pixmap.isNull());

    const QImage image = pixmap.toImage();
    bool hasLightPixel = false;
    for (int y = 0; y < image.height() && !hasLightPixel; ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor color = image.pixelColor(x, y);
            if (color.alpha() > 0 && color.lightness() > 200) {
                hasLightPixel = true;
                break;
            }
        }
    }
    QVERIFY(hasLightPixel);

    theApp()->theme().setColorSchemePreference(previousMode);
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

///
/// \brief Runs the suite under a real Application so theme-aware resources are available.
/// \param argc Argument count.
/// \param argv Argument vector.
/// \return Test exit code.
///
int main(int argc, char *argv[])
{
    Application app(argc, argv);
    TestDialogOpcUaInfo test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_dialogopcuainfo.moc"
