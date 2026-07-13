// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_connectiondialog.cpp
/// \brief UI tests for ConnectionDialog: discovery, certificate selection, and layout.
///

#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QSettings>
#include <QSizePolicy>
#include <QSpinBox>
#include <QStandardPaths>
#include <QSslCertificate>
#include <QStyleFactory>
#include <QTableView>
#include <QTemporaryDir>
#include <QTest>

#include "appsettings.h"
#include "dialogs/connectiondialog.h"
#include "models/endpointmodel.h"
#include "opcua/opcuabackend.h"
#include "opcua/pkimanager.h"
#include "widgets/certificatesummarywidget.h"
#include "widgets/endpointdiscoverywidget.h"

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
    void browse(const QString &) override {}
    void browseReferences(const QString &) override {}
    void readNode(const QString &) override {}
    void readValues(const QStringList &) override {}
    void writeValue(const QString &, const QVariant &, int) override {}

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
    void usernamePasswordDefaultsAreEmpty();
    void discoveryPopulatesEndpointModelAndAuthentication();
    void clientCertificateActionFollowsSelection();
    void clientCertificateSelectorFillsRow();
    void certificateStatusRowsAlignBadgeToRight();
    void advancedSettingsControlsAlignToGrid();
    void advancedSettingsSeedFromStoredDefaults();
    void endpointHoverUsesSelectionBackgroundInFusion();

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

///
/// \brief Reports whether a named Qt style can be created.
/// \param styleName Style name to look up.
/// \return True when Qt lists the style.
///
bool hasStyle(const QString &styleName)
{
    for (const QString &key : QStyleFactory::keys()) {
        if (key.compare(styleName, Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
}

///
/// \brief Builds a minimal endpoint row for dialog UI tests.
/// \param url Endpoint URL to show.
/// \return Endpoint row data.
///
EndpointInfo makeDialogEndpoint(const QString &url)
{
    EndpointInfo endpoint;
    endpoint.endpointUrl = url;
    endpoint.securityPolicy = QStringLiteral("None");
    endpoint.securityMode = QStringLiteral("None");
    endpoint.securityModeValue = 1;
    endpoint.supportsAnonymous = true;
    return endpoint;
}

///
/// \brief Samples a cell background away from its text.
/// \param view Table view to render.
/// \param row Model row to sample.
/// \param column Model column to sample.
/// \return Rendered pixel colour.
///
QColor cellBackgroundColor(QTableView *view, int row, int column)
{
    const QModelIndex index = view->model()->index(row, column);
    const QRect rect = view->visualRect(index);
    QImage image(view->viewport()->size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    view->viewport()->render(&painter);
    painter.end();

    return image.pixelColor(rect.right() - 4, rect.center().y());
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

void TestConnectionDialog::usernamePasswordDefaultsAreEmpty()
{
    ConnectionDialog dialog;
    auto *username = dialog.findChild<QLineEdit *>(QStringLiteral("usernameEdit"));
    auto *password = dialog.findChild<QLineEdit *>(QStringLiteral("passwordEdit"));
    QVERIFY(username);
    QVERIFY(password);

    QVERIFY(username->text().isEmpty());
    QVERIFY(password->text().isEmpty());
}

void TestConnectionDialog::discoveryPopulatesEndpointModelAndAuthentication()
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
    auto *certificateEdit = dialog.findChild<QLineEdit *>(QStringLiteral("certificateEdit"));
    auto *privateKeyEdit = dialog.findChild<QLineEdit *>(QStringLiteral("privateKeyEdit"));
    auto *certificateWidget = dialog.findChild<CertificateSummaryWidget *>(
        QStringLiteral("clientCertificateWidget"));
    QVERIFY(certificateMode);
    QVERIFY(certificateAction);
    QVERIFY(certificateEdit);
    QVERIFY(privateKeyEdit);
    QVERIFY(certificateWidget);

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
    QCOMPARE(certificateMode->currentIndex(), 0);
    QCOMPARE(certificateMode->itemText(0),
             QStringLiteral("Auto-generated (%1)")
                 .arg(QFileInfo(profile.clientCertificateFile).fileName()));
    QVERIFY(certificateEdit->text().isEmpty());
    QVERIFY(privateKeyEdit->text().isEmpty());

    QFile generatedCertificate(profile.clientCertificateFile);
    QVERIFY(generatedCertificate.open(QIODevice::ReadOnly));
    const QList<QSslCertificate> chain =
        QSslCertificate::fromData(generatedCertificate.readAll(), QSsl::Der);
    generatedCertificate.close();
    QVERIFY(!chain.isEmpty());
    QCOMPARE(certificateWidget->certificate(), chain.constFirst().toDer());

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

void TestConnectionDialog::advancedSettingsSeedFromStoredDefaults()
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

void TestConnectionDialog::endpointHoverUsesSelectionBackgroundInFusion()
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

    QCOMPARE(cellBackgroundColor(endpointView, 1, EndpointModel::PolicyColumn),
             cellBackgroundColor(endpointView, 0, EndpointModel::PolicyColumn));
}

QTEST_MAIN(TestConnectionDialog)

#include "test_connectiondialog.moc"
