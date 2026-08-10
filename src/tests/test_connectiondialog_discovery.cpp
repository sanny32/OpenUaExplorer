// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_connectiondialog_discovery.cpp
/// \brief Tests ConnectionDialog endpoint discovery and selection.
///

#include <QTest>

#include "test_connectiondialog_support.h"

///
/// \brief Tests the related behavior in an isolated Qt Test executable.
///
class TestConnectionDialogDiscovery : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();
    void discoveryPopulatesEndpointModelAndAuthentication();
    void endpointsWidgetSelectsAPolicyAndModePair();
    void closingDuringDiscoveryRestoresCursor();

private:
    QTemporaryDir _settingsDirectory;
};

void TestConnectionDialogDiscovery::initTestCase()
{
    QVERIFY(_settingsDirectory.isValid());
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName(QStringLiteral("OpenUaExplorerTests"));
    QCoreApplication::setApplicationName(QStringLiteral("TestConnectionDialogDiscovery"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       _settingsDirectory.path());
}

void TestConnectionDialogDiscovery::cleanup()
{
    while (QGuiApplication::overrideCursor())
        QGuiApplication::restoreOverrideCursor();
    SettingsStore settings;
    settings.clear();
}


void TestConnectionDialogDiscovery::discoveryPopulatesEndpointModelAndAuthentication()
{
    DialogFakeBackend backend;
    ConnectionDialog dialog;
    dialog.setBackend(&backend);

    auto *endpointView = dialog.findChild<QTableView *>(
        QStringLiteral("endpointListWidget"));
    auto *authentication = dialog.findChild<QComboBox *>(
        QStringLiteral("authenticationComboBox"));
    auto *discoverButton = dialog.findChild<QPushButton *>(
        QStringLiteral("getEndpointsButton"));
    auto *connectButton = dialog.findChild<QPushButton *>(
        QStringLiteral("connectButton"));
    QVERIFY(endpointView);
    QVERIFY(authentication);
    QVERIFY(discoverButton);
    QVERIFY(connectButton);

    QVERIFY(QMetaObject::invokeMethod(&dialog, "discoverEndpoints"));
    QCOMPARE(backend.discoveryCalls, 1);
    QVERIFY(!discoverButton->isEnabled());
    QVERIFY(!connectButton->isEnabled());
    QVERIFY(QGuiApplication::overrideCursor());
    QCOMPARE(QGuiApplication::overrideCursor()->shape(), Qt::WaitCursor);

    EndpointInfo endpoint;
    endpoint.endpointUrl = QStringLiteral("opc.tcp://localhost:4840");
    endpoint.securityPolicy =
        QStringLiteral("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");
    endpoint.securityMode = QStringLiteral("Sign & Encrypt");
    endpoint.securityModeValue = 3;
    endpoint.supportsAnonymous = true;
    endpoint.supportsUsername = true;
    endpoint.supportsCertificate = true;
    emit backend.endpointsDiscovered({endpoint}, {});

    QCOMPARE(endpointView->model()->rowCount(), 1);
    QCOMPARE(endpointView->currentIndex().row(), 0);
    QCOMPARE(authentication->count(), 3);
    QVERIFY(discoverButton->isEnabled());
    QVERIFY(connectButton->isEnabled());
    QVERIFY(!QGuiApplication::overrideCursor());
    QCOMPARE(dialog.profile().endpointUrl, endpoint.endpointUrl);

    QVERIFY(QMetaObject::invokeMethod(&dialog, "discoverEndpoints"));
    QVERIFY(QGuiApplication::overrideCursor());
    emit backend.endpointsDiscovered({}, QStringLiteral("Discovery failed"));
    QVERIFY(!QGuiApplication::overrideCursor());
}

///
/// \brief Verifies the pre-selection the find-servers dialog hands back after discovery.
///
void TestConnectionDialogDiscovery::endpointsWidgetSelectsAPolicyAndModePair()
{
    ConnectionDialog dialog;
    auto *endpoints = dialog.findChild<EndpointDiscoveryWidget *>();
    QVERIFY(endpoints);

    const QString policyPrefix =
        QStringLiteral("http://opcfoundation.org/UA/SecurityPolicy#");
    auto makeEndpoint = [&policyPrefix](const QString &policy, int modeValue) {
        EndpointInfo endpoint;
        endpoint.endpointUrl = QStringLiteral("opc.tcp://localhost:4840");
        endpoint.securityPolicy = policyPrefix + policy;
        endpoint.securityMode = modeValue == 3 ? QStringLiteral("Sign & Encrypt")
                                               : QStringLiteral("Sign");
        endpoint.securityModeValue = modeValue;
        endpoint.supportsAnonymous = true;
        return endpoint;
    };

    endpoints->setEndpoints({makeEndpoint(QStringLiteral("Aes256_Sha256_RsaPss"), 3),
                             makeEndpoint(QStringLiteral("Aes256_Sha256_RsaPss"), 2),
                             makeEndpoint(QStringLiteral("Basic256Sha256"), 3)});
    QCOMPARE(endpoints->currentRow(), 0);

    QVERIFY(endpoints->selectEndpoint(policyPrefix + QStringLiteral("Basic256Sha256"), 3));
    QCOMPARE(endpoints->currentEndpoint().securityPolicy,
             policyPrefix + QStringLiteral("Basic256Sha256"));

    QVERIFY(!endpoints->selectEndpoint(policyPrefix + QStringLiteral("Basic128Rsa15"), 3));
    QCOMPARE(endpoints->currentEndpoint().securityPolicy,
             policyPrefix + QStringLiteral("Basic256Sha256"));
}

void TestConnectionDialogDiscovery::closingDuringDiscoveryRestoresCursor()
{
    DialogFakeBackend backend;
    {
        ConnectionDialog dialog;
        dialog.setBackend(&backend);
        QVERIFY(QMetaObject::invokeMethod(&dialog, "discoverEndpoints"));
        QVERIFY(QGuiApplication::overrideCursor());
    }
    QVERIFY(!QGuiApplication::overrideCursor());
}

QTEST_MAIN(TestConnectionDialogDiscovery)

#include "test_connectiondialog_discovery.moc"
