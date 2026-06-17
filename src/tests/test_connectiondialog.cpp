// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_connectiondialog.cpp
/// \brief UI tests for ConnectionDialog: discovery, certificate selection, and layout.
///

#include <QComboBox>
#include <QCoreApplication>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QSizePolicy>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTableView>
#include <QTemporaryDir>
#include <QTest>

#include "dialogs/connectiondialog.h"
#include "opcua/opcuabackend.h"
#include "opcua/opcuaclientservice.h"
#include "opcua/pkimanager.h"

///
/// \brief Minimal OPC UA backend double that only counts discovery calls.
///
class DialogFakeBackend : public OpcUaBackend
{
    Q_OBJECT

public:
    using OpcUaBackend::OpcUaBackend;

    bool isAvailable() const override { return true; }
    QStringList availableBackends() const override { return {QStringLiteral("fake")}; }
    OpcUaConnectionState state() const override
    {
        return OpcUaConnectionState::Disconnected;
    }
    QString lastError() const override { return {}; }
    void setCertificateTrustDecider(CertificateTrustDecider *) override {}

    void discoverEndpoints(const QString &, const QString &, int) override
    {
        ++discoveryCalls;
    }
    void connectToEndpoint(const ConnectionProfile &, const QString &,
                           const QString &) override {}
    void disconnectFromEndpoint() override {}
    void browse(const QString &, int) override {}
    void readNode(const QString &, int) override {}
    void readValues(const QStringList &, int) override {}
    void writeValue(const QString &, const QVariant &, int, int) override {}

    int discoveryCalls = 0;
};

///
/// \brief Drives the dialog through discovery, certificate, and layout scenarios.
///
class TestConnectionDialog : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();
    void discoveryPopulatesEndpointModelAndAuthentication();
    void clientCertificateActionFollowsSelection();
    void clientCertificateSelectorFillsRow();
    void certificateStatusRowsAlignBadgeToRight();
    void advancedSettingsControlsAlignToGrid();

private:
    QTemporaryDir _settingsDirectory;
};

namespace {

///
/// \brief Asserts a certificate panel's validity row right-aligns its icon and badge.
/// \param dialog Dialog under test.
/// \param panelName Object name of the certificate summary panel.
///
void verifyRightAlignedCertificateStatus(ConnectionDialog &dialog, const QString &panelName)
{
    auto *panel = dialog.findChild<QWidget *>(panelName);
    QVERIFY(panel);

    // The Subject/Issuer/Valid/Fingerprint rows now live inside the reusable
    // CertificateSummaryWidget, so scope the lookups to that panel.
    auto *layout = panel->findChild<QHBoxLayout *>(QStringLiteral("validLayout"));
    auto *dateLabel = panel->findChild<QLabel *>(QStringLiteral("validEdit"));
    auto *iconLabel = panel->findChild<QLabel *>(QStringLiteral("validIcon"));
    auto *badgeLabel = panel->findChild<QLabel *>(QStringLiteral("validBadge"));

    QVERIFY(layout);
    QVERIFY(dateLabel);
    QVERIFY(iconLabel);
    QVERIFY(badgeLabel);
    QCOMPARE(layout->count(), 4);
    QCOMPARE(dateLabel->sizePolicy().horizontalPolicy(), QSizePolicy::Maximum);
    QCOMPARE(iconLabel->sizePolicy().horizontalPolicy(), QSizePolicy::Maximum);
    QCOMPARE(badgeLabel->sizePolicy().horizontalPolicy(), QSizePolicy::Maximum);
    QVERIFY(layout->itemAt(1)->spacerItem());
    QVERIFY(layout->itemAt(1)->expandingDirections().testFlag(Qt::Horizontal));
    QCOMPARE(layout->itemAt(2)->widget(), iconLabel);
    QCOMPARE(layout->itemAt(3)->widget(), badgeLabel);
}

}

void TestConnectionDialog::initTestCase()
{
    QVERIFY(_settingsDirectory.isValid());
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName(QStringLiteral("OpenUaExplorerTests"));
    QCoreApplication::setApplicationName(QStringLiteral("ConnectionDialog"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       _settingsDirectory.path());
}

void TestConnectionDialog::cleanup()
{
    QSettings settings;
    settings.clear();
}

void TestConnectionDialog::discoveryPopulatesEndpointModelAndAuthentication()
{
    DialogFakeBackend backend;
    OpcUaClientService service(&backend);
    ConnectionDialog dialog;
    dialog.setClientService(&service);

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
    QCOMPARE(dialog.profile().endpointUrl, endpoint.endpointUrl);
}

void TestConnectionDialog::clientCertificateActionFollowsSelection()
{
    ConnectionDialog dialog;
    auto *certificateMode = dialog.findChild<QComboBox *>(
        QStringLiteral("clientCertificateComboBox"));
    auto *certificateAction = dialog.findChild<QPushButton *>(
        QStringLiteral("clientCertificateViewButton"));
    QVERIFY(certificateMode);
    QVERIFY(certificateAction);

    certificateMode->setCurrentIndex(0);
    QCOMPARE(certificateAction->text(), QStringLiteral("Generate..."));

    certificateMode->setCurrentIndex(1);
    QCOMPARE(certificateAction->text(), QStringLiteral("Import..."));

    PkiManager pki;
    QString preflightCertificateFile;
    QString preflightPrivateKeyFile;
    QString preflightError;
    if (!pki.generateClientCertificate(
            PkiManager::clientCertificateCommonName(),
            PkiManager::applicationUri(),
            &preflightCertificateFile, &preflightPrivateKeyFile, &preflightError)) {
        QSKIP(qPrintable(preflightError));
    }
    QVERIFY(QFile::remove(preflightCertificateFile));
    QVERIFY(QFile::remove(preflightPrivateKeyFile));

    certificateMode->setCurrentIndex(0);
    QTest::mouseClick(certificateAction, Qt::LeftButton);
    const ConnectionProfile profile = dialog.profile();
    QVERIFY(QFile::exists(profile.clientCertificateFile));
    QVERIFY(QFile::exists(profile.privateKeyFile));
    QVERIFY(QFile::remove(profile.clientCertificateFile));
    QVERIFY(QFile::remove(profile.privateKeyFile));
}

void TestConnectionDialog::clientCertificateSelectorFillsRow()
{
    ConnectionDialog dialog;
    auto *layout = dialog.findChild<QHBoxLayout *>(QStringLiteral("clientCertificateLayout"));
    auto *certificateMode = dialog.findChild<QComboBox *>(
        QStringLiteral("clientCertificateComboBox"));
    auto *certificateAction = dialog.findChild<QPushButton *>(
        QStringLiteral("clientCertificateViewButton"));
    QVERIFY(layout);
    QVERIFY(certificateMode);
    QVERIFY(certificateAction);

    QCOMPARE(layout->count(), 3);
    QCOMPARE(layout->stretch(1), 1);
    QCOMPARE(layout->itemAt(1)->widget(), certificateMode);
    QCOMPARE(layout->itemAt(2)->widget(), certificateAction);
}

void TestConnectionDialog::certificateStatusRowsAlignBadgeToRight()
{
    ConnectionDialog dialog;

    verifyRightAlignedCertificateStatus(dialog, QStringLiteral("clientCertificateWidget"));
    verifyRightAlignedCertificateStatus(dialog, QStringLiteral("serverCertificateWidget"));
}

void TestConnectionDialog::advancedSettingsControlsAlignToGrid()
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

QTEST_MAIN(TestConnectionDialog)

#include "test_connectiondialog.moc"
