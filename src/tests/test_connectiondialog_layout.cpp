// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_connectiondialog_layout.cpp
/// \brief Tests ConnectionDialog advanced settings, layout, and style behavior.
///

#include <QTest>
#include <QGridLayout>
#include <QGroupBox>
#include <QScopeGuard>

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
    void authenticationControlsFillAvailableWidth_data();
    void authenticationControlsFillAvailableWidth();

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

///
/// \brief Exercises every authentication page with the same form width constraints.
///
void TestConnectionDialogLayout::authenticationControlsFillAvailableWidth_data()
{
    QTest::addColumn<int>("mode");
    QTest::newRow("anonymous") << static_cast<int>(ConnectionProfile::Authentication::Anonymous);
    QTest::newRow("username") << static_cast<int>(ConnectionProfile::Authentication::Username);
    QTest::newRow("certificate") << static_cast<int>(ConnectionProfile::Authentication::Certificate);
}

///
/// \brief Verifies authentication fields fill the form and grow with the dialog.
///
void TestConnectionDialogLayout::authenticationControlsFillAvailableWidth()
{
    QFETCH(int, mode);
    ConnectionDialog dialog;
    const auto hideDialog = qScopeGuard([&dialog] { dialog.hide(); });
    auto *authentication = dialog.findChild<QComboBox *>(QStringLiteral("authenticationComboBox"));
    auto *group = dialog.findChild<QGroupBox *>(QStringLiteral("authenticationGroupBox"));
    auto *grid = dialog.findChild<QGridLayout *>(QStringLiteral("authenticationLayout"));
    QVERIFY(authentication);
    QVERIFY(group);
    QVERIFY(grid);
    authentication->setCurrentIndex(mode);

    QStringList fieldNames;
    QStringList rightAlignedNames = {QStringLiteral("authenticationComboBox")};
    if (mode == static_cast<int>(ConnectionProfile::Authentication::Anonymous)) {
        fieldNames = {QStringLiteral("anonymousNotice")};
        rightAlignedNames.append(fieldNames);
    } else if (mode == static_cast<int>(ConnectionProfile::Authentication::Username)) {
        fieldNames = {QStringLiteral("usernameEdit"), QStringLiteral("passwordEdit")};
        rightAlignedNames.append(fieldNames);
    } else {
        fieldNames = {QStringLiteral("certificateEdit"), QStringLiteral("privateKeyEdit"),
                      QStringLiteral("privateKeyPasswordEdit")};
        rightAlignedNames.append({QStringLiteral("certificateBrowseButton"),
                                  QStringLiteral("privateKeyBrowseButton"),
                                  QStringLiteral("privateKeyPasswordEdit")});
    }
    fieldNames.append(QStringLiteral("authenticationComboBox"));
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));

    QList<int> initialWidths;
    for (const QString &name : fieldNames) {
        auto *field = dialog.findChild<QWidget *>(name);
        QVERIFY(field);
        initialWidths.append(field->width());
    }
    for (const QString &name : rightAlignedNames) {
        auto *field = dialog.findChild<QWidget *>(name);
        QVERIFY(field);
        QCOMPARE(field->mapTo(group, QPoint(field->width() - 1, 0)).x(),
                 group->contentsRect().right() - grid->contentsMargins().right());
    }

    dialog.resize(dialog.width() + 400, dialog.height());
    QCoreApplication::processEvents();
    for (int i = 0; i < fieldNames.size(); ++i) {
        auto *field = dialog.findChild<QWidget *>(fieldNames.at(i));
        QVERIFY2(field->width() > initialWidths.at(i), qPrintable(fieldNames.at(i)));
    }
    for (const QString &name : rightAlignedNames) {
        auto *field = dialog.findChild<QWidget *>(name);
        QCOMPARE(field->mapTo(group, QPoint(field->width() - 1, 0)).x(),
                 group->contentsRect().right() - grid->contentsMargins().right());
    }
    dialog.hide();
}

QTEST_MAIN(TestConnectionDialogLayout)

#include "test_connectiondialog_layout.moc"
