// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_connectiondialog_layout.cpp
/// \brief Tests ConnectionDialog advanced settings, layout, and style behavior.
///

#include <QTest>

#include "test_connectiondialog_support.h"

///
/// \brief Tests the related behavior in an isolated Qt Test executable.
///
class TestConnectionDialogLayout : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();
    void advancedSettingsControlsAlignToGrid();
    void advancedSettingsSeedFromStoredDefaults();
    void endpointHoverDoesNotUseSelectionBackgroundInFusion();

private:
    QTemporaryDir _settingsDirectory;
};

void TestConnectionDialogLayout::initTestCase()
{
    QVERIFY(_settingsDirectory.isValid());
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName(QStringLiteral("OpenUaExplorerTests"));
    QCoreApplication::setApplicationName(QStringLiteral("TestConnectionDialogLayout"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       _settingsDirectory.path());
}

void TestConnectionDialogLayout::cleanup()
{
    while (QGuiApplication::overrideCursor())
        QGuiApplication::restoreOverrideCursor();
    SettingsStore settings;
    settings.clear();
}


void TestConnectionDialogLayout::advancedSettingsControlsAlignToGrid()
{
    ConnectionDialog dialog;
    dialog.resize(dialog.minimumSize());
    QVERIFY(dialog.layout()->activate());

    auto *sessionTimeout = dialog.findChild<QSpinBox *>(QStringLiteral("sessionTimeoutSpinBox"));
    auto *connectTimeout = dialog.findChild<QSpinBox *>(QStringLiteral("connectTimeoutSpinBox"));
    auto *secureChannelLifetime =
        dialog.findChild<QSpinBox *>(QStringLiteral("secureChannelLifetimeSpinBox"));
    auto *endpointTimeout = dialog.findChild<QSpinBox *>(QStringLiteral("endpointTimeoutSpinBox"));
    auto *requestTimeout = dialog.findChild<QSpinBox *>(QStringLiteral("requestTimeoutSpinBox"));
    auto *maxMessageSize = dialog.findChild<QSpinBox *>(QStringLiteral("maxMessageSizeSpinBox"));
    QVERIFY(sessionTimeout);
    QVERIFY(connectTimeout);
    QVERIFY(secureChannelLifetime);
    QVERIFY(endpointTimeout);
    QVERIFY(requestTimeout);
    QVERIFY(maxMessageSize);

    QCOMPARE(sessionTimeout->x(), connectTimeout->x());
    QCOMPARE(sessionTimeout->x(), secureChannelLifetime->x());
    QCOMPARE(endpointTimeout->x(), requestTimeout->x());
    QCOMPARE(endpointTimeout->x(), maxMessageSize->x());

    QCOMPARE(sessionTimeout->y(), endpointTimeout->y());
    QCOMPARE(connectTimeout->y(), requestTimeout->y());
    QCOMPARE(secureChannelLifetime->y(), maxMessageSize->y());
}

void TestConnectionDialogLayout::advancedSettingsSeedFromStoredDefaults()
{
    AppSettings::SessionDefaults defaults;
    defaults.sessionTimeoutMs = 90000;
    defaults.endpointTimeoutMs = 7000;
    defaults.connectTimeoutMs = 12000;
    defaults.requestTimeoutMs = 4000;
    defaults.secureChannelLifetimeMs = 300000;
    defaults.maxMessageSizeBytes = 8388608;
    AppSettings().setSessionDefaults(defaults);

    ConnectionDialog dialog;
    QCOMPARE(dialog.findChild<QSpinBox *>(QStringLiteral("sessionTimeoutSpinBox"))->value(),
             defaults.sessionTimeoutMs);
    QCOMPARE(dialog.findChild<QSpinBox *>(QStringLiteral("endpointTimeoutSpinBox"))->value(),
             defaults.endpointTimeoutMs);
    QCOMPARE(dialog.findChild<QSpinBox *>(QStringLiteral("connectTimeoutSpinBox"))->value(),
             defaults.connectTimeoutMs);
    QCOMPARE(dialog.findChild<QSpinBox *>(QStringLiteral("requestTimeoutSpinBox"))->value(),
             defaults.requestTimeoutMs);
    QCOMPARE(dialog.findChild<QSpinBox *>(QStringLiteral("secureChannelLifetimeSpinBox"))->value(),
             defaults.secureChannelLifetimeMs);
    QCOMPARE(dialog.findChild<QSpinBox *>(QStringLiteral("maxMessageSizeSpinBox"))->value(),
             defaults.maxMessageSizeBytes);
}

void TestConnectionDialogLayout::endpointHoverDoesNotUseSelectionBackgroundInFusion()
{
    if (!hasStyle(QStringLiteral("Fusion")))
        QSKIP("Fusion style is unavailable.");
    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    ConnectionDialog dialog;
    dialog.resize(900, 600);
    auto *endpoints = dialog.findChild<EndpointDiscoveryWidget *>(
        QStringLiteral("endpointsWidget"));
    auto *endpointView = dialog.findChild<QTableView *>(
        QStringLiteral("endpointListWidget"));
    QVERIFY(endpoints);
    QVERIFY(endpointView);

    endpoints->setEndpoints({
        makeDialogEndpoint(QStringLiteral("opc.tcp://localhost:4840")),
        makeDialogEndpoint(QStringLiteral("opc.tcp://localhost:4841"))
    });

    QVERIFY(dialog.layout()->activate());
    endpointView->setProperty("hoveredRow", 1);

    QVERIFY(cellBackgroundColor(endpointView, 1, EndpointModel::PolicyColumn)
            != cellBackgroundColor(endpointView, 0, EndpointModel::PolicyColumn));
}

QTEST_MAIN(TestConnectionDialogLayout)

#include "test_connectiondialog_layout.moc"
