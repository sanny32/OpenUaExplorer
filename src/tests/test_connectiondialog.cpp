// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSizePolicy>
#include <QTableView>
#include <QTest>

#include "dialogs/connectiondialog.h"
#include "opcua/opcuabackend.h"
#include "opcua/opcuaclientservice.h"

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

class TestConnectionDialog : public QObject
{
    Q_OBJECT

private slots:
    void discoveryPopulatesEndpointModelAndAuthentication();
    void certificateStatusRowsAlignBadgeToRight();
};

namespace {

void verifyRightAlignedCertificateStatus(ConnectionDialog &dialog, const QString &layoutName,
                                         const QString &dateLabelName,
                                         const QString &iconLabelName,
                                         const QString &badgeLabelName)
{
    auto *layout = dialog.findChild<QHBoxLayout *>(layoutName);
    auto *dateLabel = dialog.findChild<QLabel *>(dateLabelName);
    auto *iconLabel = dialog.findChild<QLabel *>(iconLabelName);
    auto *badgeLabel = dialog.findChild<QLabel *>(badgeLabelName);

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

void TestConnectionDialog::certificateStatusRowsAlignBadgeToRight()
{
    ConnectionDialog dialog;

    verifyRightAlignedCertificateStatus(dialog, QStringLiteral("clientCertValidLayout"),
                                        QStringLiteral("clientCertValidEdit"),
                                        QStringLiteral("clientCertValidIcon"),
                                        QStringLiteral("clientCertValidBadge"));
    verifyRightAlignedCertificateStatus(dialog, QStringLiteral("serverCertValidLayout"),
                                        QStringLiteral("serverCertValidEdit"),
                                        QStringLiteral("serverCertValidIcon"),
                                        QStringLiteral("serverCertValidBadge"));
}

QTEST_MAIN(TestConnectionDialog)

#include "test_connectiondialog.moc"
