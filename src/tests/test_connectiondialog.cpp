// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_connectiondialog.cpp
/// \brief UI tests for ConnectionDialog: discovery, certificate selection, and layout.
///

#include <algorithm>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QCursor>
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
#include <QStackedWidget>
#include <QStyleFactory>
#include <QTableView>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QUuid>

#include "appsettings.h"
#include "dialogs/connectiondialog.h"
#include "models/endpointmodel.h"
#include "opcua/endpointhistorystore.h"
#include "opcua/opcuabackend.h"
#include "opcua/pkimanager.h"
#include "opcua/recentconnectionstore.h"
#include "opcua/secretstore.h"
#include "settingsstore.h"
#include "widgets/certificatesummarywidget.h"
#include "widgets/dialogbuttonbox.h"
#include "widgets/endpointdiscoverywidget.h"
#include "widgets/historycombobox.h"

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
    void designerEndpointsSeedEmptyHistory();
    void clearedEndpointHistoryStaysEmpty();
    void usernamePasswordDefaultsAreEmpty();
    void rememberOptionFollowsUsernameAuthentication();
    void anonymousAuthenticationHidesTheCredentialFields();
    void reopeningRestoresTheAuthenticationOfTheLastConnection();
    void reopeningFillsInTheRememberedPassword();
    void discoveryPopulatesEndpointModelAndAuthentication();
    void endpointsWidgetSelectsAPolicyAndModePair();
    void closingDuringDiscoveryRestoresCursor();
    void clientCertificateActionFollowsSelection();
    void clientCertificateSelectorFillsRow();
    void certificateStatusRowsAlignBadgeToRight();
    void advancedSettingsControlsAlignToGrid();
    void advancedSettingsSeedFromStoredDefaults();
    void serverTrustStateFollowsTrustList();
    void trustIsRefusedForACertificateOutsideItsValidity();
    void endpointHoverDoesNotUseSelectionBackgroundInFusion();

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

///
/// \brief Generates a client certificate and returns its DER bytes.
/// \return DER-encoded certificate, or an empty array when generation is unavailable.
///
QByteArray generateCertificate()
{
    PkiManager pki;
    QString certificateFile;
    QString privateKeyFile;
    QString error;
    if (!pki.generateClientCertificate(PkiManager::clientCertificateCommonName(),
                                       PkiManager::applicationUri(),
                                       &certificateFile, &privateKeyFile, &error)) {
        return {};
    }
    QFile file(certificateFile);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

///
/// \brief Answers the next modal question dialog, waiting for it to appear.
/// \param answer Standard button to click once the dialog is up.
///
void answerNextQuestion(DialogButtonBox::StandardButton answer)
{
    QTimer::singleShot(0, qApp, [answer]() {
        auto *modal = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!modal) {
            answerNextQuestion(answer);
            return;
        }
        auto *buttons = modal->findChild<DialogButtonBox *>();
        QVERIFY(buttons);
        QPushButton *button = buttons->button(answer);
        QVERIFY(button);
        QTest::mouseClick(button, Qt::LeftButton);
    });
}

///
/// \brief Reports whether the trust list holds a certificate.
/// \param certificate DER-encoded certificate.
/// \return True when the trusted store holds this certificate.
///
bool isInTrustList(const QByteArray &certificate)
{
    PkiManager pki;
    const QString wanted = PkiManager::fingerprint(certificate);
    const QList<QByteArray> trusted = pki.certificates(PkiManager::Category::Trusted);
    return std::any_of(trusted.cbegin(), trusted.cend(), [&wanted](const QByteArray &stored) {
        return PkiManager::fingerprint(stored) == wanted;
    });
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
    while (QGuiApplication::overrideCursor())
        QGuiApplication::restoreOverrideCursor();
    SettingsStore settings;
    settings.clear();
}

///
/// \brief Verifies that every endpoint declared in the UI seeds an empty history.
///
void TestConnectionDialog::designerEndpointsSeedEmptyHistory()
{
    ConnectionDialog dialog;
    auto *endpoints = dialog.findChild<HistoryComboBox *>(
        QStringLiteral("discoveryUrlComboBox"));
    QVERIFY(endpoints);

    QCOMPARE(endpoints->count(), 4);
    QCOMPARE(endpoints->itemText(0),
             QStringLiteral("opc.tcp://uademo.prosysopc.com:53530/OPCUA/SimulationServer"));
    QCOMPARE(endpoints->itemText(1),
             QStringLiteral("opc.tcp://opcua.demo-this.com:51210/UA/SampleServer"));
    QCOMPARE(endpoints->itemData(2, Qt::AccessibleDescriptionRole).toString(),
             QStringLiteral("separator"));
    QVERIFY(endpoints->isActionEntry(3));
}

///
/// \brief Verifies that removing every initial endpoint leaves later dialogs empty.
///
void TestConnectionDialog::clearedEndpointHistoryStaysEmpty()
{
    ConnectionDialog initialDialog;
    auto *initialEndpoints = initialDialog.findChild<HistoryComboBox *>(
        QStringLiteral("discoveryUrlComboBox"));
    QVERIFY(initialEndpoints);

    EndpointHistoryStore store;
    for (int index = 0; index < initialEndpoints->count(); ++index) {
        if (!initialEndpoints->isActionEntry(index))
            store.remove(initialEndpoints->itemText(index));
    }

    ConnectionDialog reopenedDialog;
    auto *reopenedEndpoints = reopenedDialog.findChild<HistoryComboBox *>(
        QStringLiteral("discoveryUrlComboBox"));
    QVERIFY(reopenedEndpoints);
    QCOMPARE(reopenedEndpoints->count(), 1);
    QVERIFY(reopenedEndpoints->isActionEntry(0));
    QVERIFY(reopenedEndpoints->currentText().isEmpty());
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

///
/// \brief Remembering a password is offered for username authentication and nothing else.
///
void TestConnectionDialog::rememberOptionFollowsUsernameAuthentication()
{
    ConnectionDialog dialog;
    auto *remember = dialog.findChild<QCheckBox *>(QStringLiteral("rememberCheckBox"));
    auto *authentication = dialog.findChild<QComboBox *>(
        QStringLiteral("authenticationComboBox"));
    QVERIFY(remember);
    QVERIFY(authentication);

    // The designer items are ordered like the authentication enum.
    authentication->setCurrentIndex(
        static_cast<int>(ConnectionProfile::Authentication::Anonymous));
    QVERIFY(!remember->isEnabled());
    QVERIFY(!dialog.rememberCredentials());

    authentication->setCurrentIndex(
        static_cast<int>(ConnectionProfile::Authentication::Username));
    QVERIFY(remember->isEnabled());
    QVERIFY(!dialog.rememberCredentials());

    remember->setChecked(true);
    QVERIFY(dialog.rememberCredentials());

    // A mode without a password to store must not report a leftover tick as an answer.
    authentication->setCurrentIndex(
        static_cast<int>(ConnectionProfile::Authentication::Certificate));
    QVERIFY(!dialog.rememberCredentials());
}

///
/// \brief Anonymous shows its notice instead of credential fields, without growing the stack.
///
void TestConnectionDialog::anonymousAuthenticationHidesTheCredentialFields()
{
    ConnectionDialog dialog;
    auto *stack = dialog.findChild<QStackedWidget *>(QStringLiteral("authStack"));
    auto *anonymousPanel = dialog.findChild<QWidget *>(QStringLiteral("anonymousPanel"));
    auto *usernamePanel = dialog.findChild<QWidget *>(QStringLiteral("usernamePanel"));
    auto *certificatePanel = dialog.findChild<QWidget *>(QStringLiteral("certificatePanel"));
    auto *authentication = dialog.findChild<QComboBox *>(
        QStringLiteral("authenticationComboBox"));
    QVERIFY(stack);
    QVERIFY(anonymousPanel);
    QVERIFY(usernamePanel);
    QVERIFY(certificatePanel);
    QVERIFY(authentication);

    // The designer items are ordered like the authentication enum.
    authentication->setCurrentIndex(
        static_cast<int>(ConnectionProfile::Authentication::Anonymous));
    QCOMPARE(stack->currentWidget(), anonymousPanel);

    authentication->setCurrentIndex(
        static_cast<int>(ConnectionProfile::Authentication::Username));
    QCOMPARE(stack->currentWidget(), usernamePanel);

    authentication->setCurrentIndex(
        static_cast<int>(ConnectionProfile::Authentication::Certificate));
    QCOMPARE(stack->currentWidget(), certificatePanel);

    authentication->setCurrentIndex(
        static_cast<int>(ConnectionProfile::Authentication::Anonymous));

    // The notice fills the space the credential fields leave behind, and must fit in it: a
    // taller page would make the stack reserve extra height on the other pages as well.
    auto *notice = dialog.findChild<QWidget *>(QStringLiteral("anonymousNotice"));
    QVERIFY(notice);
    QVERIFY(!notice->isHidden());
    QVERIFY(anonymousPanel->sizeHint().height() <= usernamePanel->sizeHint().height());
    QVERIFY(anonymousPanel->sizeHint().height() <= certificatePanel->sizeHint().height());

    // It reads as part of the form: left edge where the labels start, right edge where the
    // combo box above it ends.
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    auto *label = dialog.findChild<QLabel *>(QStringLiteral("authenticationLabel"));
    QVERIFY(label);
    const QRect noticeRect(notice->mapTo(&dialog, QPoint(0, 0)), notice->size());
    const QRect labelRect(label->mapTo(&dialog, QPoint(0, 0)), label->size());
    const QRect comboRect(authentication->mapTo(&dialog, QPoint(0, 0)), authentication->size());
    QCOMPARE(noticeRect.left(), labelRect.left());
    QCOMPARE(noticeRect.right(), comboRect.right());

    // Hiding a shown dialog only once it is destroyed sends the focus-out through slots of a
    // half-destroyed object, so close it while it is still whole.
    dialog.hide();
}

///
/// \brief Reopening the dialog for a known server brings back how it was connected.
///
void TestConnectionDialog::reopeningRestoresTheAuthenticationOfTheLastConnection()
{
    const QString endpoint = QStringLiteral("opc.tcp://restored:4840");
    EndpointHistoryStore().save(endpoint);

    ConnectionProfile profile;
    profile.id = QStringLiteral("restored-profile");
    profile.endpointUrl = endpoint;
    profile.authentication = ConnectionProfile::Authentication::Username;
    profile.username = QStringLiteral("operator");
    RecentConnectionStore().record(profile);

    ConnectionDialog dialog;
    auto *authentication = dialog.findChild<QComboBox *>(
        QStringLiteral("authenticationComboBox"));
    auto *username = dialog.findChild<QLineEdit *>(QStringLiteral("usernameEdit"));
    QVERIFY(authentication);
    QVERIFY(username);

    QCOMPARE(authentication->currentIndex(),
             static_cast<int>(ConnectionProfile::Authentication::Username));
    QCOMPARE(username->text(), QStringLiteral("operator"));
    // The connection continues the profile it had, so its stored password keeps being found.
    QCOMPARE(dialog.profile().id, profile.id);
}

///
/// \brief A password kept for the server is filled in, with the remember option ticked.
///
void TestConnectionDialog::reopeningFillsInTheRememberedPassword()
{
    const QString endpoint = QStringLiteral("opc.tcp://remembered:4840");
    // Unique per run so the credential store of the machine keeps no test leftovers around.
    const QString profileId = QStringLiteral("ouaexp-dialog-test-")
        + QUuid::createUuid().toString(QUuid::WithoutBraces);

    SecretStore secrets;
    QSignalSpy writeSpy(&secrets, &SecretStore::writeFinished);
    secrets.write(profileId, SecretStore::Secret::Password, QStringLiteral("kept-secret"));
    if (!writeSpy.wait(5000) || !writeSpy.takeFirst().at(2).toString().isEmpty())
        QSKIP("No usable keychain backend.");

    EndpointHistoryStore().save(endpoint);
    ConnectionProfile profile;
    profile.id = profileId;
    profile.endpointUrl = endpoint;
    profile.authentication = ConnectionProfile::Authentication::Username;
    profile.username = QStringLiteral("operator");
    RecentConnectionStore().record(profile);

    {
        ConnectionDialog dialog;
        auto *password = dialog.findChild<QLineEdit *>(QStringLiteral("passwordEdit"));
        auto *remember = dialog.findChild<QCheckBox *>(QStringLiteral("rememberCheckBox"));
        QVERIFY(password);
        QVERIFY(remember);

        QTRY_COMPARE(password->text(), QStringLiteral("kept-secret"));
        QVERIFY(remember->isChecked());
        QCOMPARE(dialog.password(), QStringLiteral("kept-secret"));
    }

    QSignalSpy removeSpy(&secrets, &SecretStore::writeFinished);
    secrets.remove(profileId, SecretStore::Secret::Password);
    removeSpy.wait(5000);
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
void TestConnectionDialog::endpointsWidgetSelectsAPolicyAndModePair()
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

void TestConnectionDialog::closingDuringDiscoveryRestoresCursor()
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

void TestConnectionDialog::serverTrustStateFollowsTrustList()
{
    const QByteArray certificate = generateCertificate();
    if (certificate.isEmpty())
        QSKIP("Certificate generation is unavailable.");

    DialogFakeBackend backend;
    ConnectionDialog dialog;
    dialog.setBackend(&backend);

    auto *trustSection = dialog.findChild<QWidget *>(QStringLiteral("serverTrustSection"));
    auto *trustStatus = dialog.findChild<QLabel *>(QStringLiteral("serverTrustStatusLabel"));
    auto *trustButton = dialog.findChild<QPushButton *>(QStringLiteral("serverTrustButton"));
    QVERIFY(trustSection);
    QVERIFY(trustStatus);
    QVERIFY(trustButton);

    // Without an endpoint there is no certificate to reason about, so the section stays away.
    QVERIFY(!trustSection->isVisibleTo(&dialog));

    EndpointInfo endpoint = makeDialogEndpoint(QStringLiteral("opc.tcp://localhost:4840"));
    endpoint.securityMode = QStringLiteral("Sign & Encrypt");
    endpoint.securityModeValue = 3;
    endpoint.serverCertificate = certificate;
    emit backend.endpointsDiscovered({endpoint}, {});

    QVERIFY(trustSection->isVisibleTo(&dialog));
    QVERIFY(!isInTrustList(certificate));
    QCOMPARE(trustStatus->text(), QStringLiteral("Not in trust list"));
    QCOMPARE(trustButton->text(), QStringLiteral("Trust"));

    // Declining the confirmation leaves the trust list untouched.
    answerNextQuestion(DialogButtonBox::No);
    QTest::mouseClick(trustButton, Qt::LeftButton);
    QVERIFY(!isInTrustList(certificate));
    QCOMPARE(trustButton->text(), QStringLiteral("Trust"));

    answerNextQuestion(DialogButtonBox::Yes);
    QTest::mouseClick(trustButton, Qt::LeftButton);
    QVERIFY(isInTrustList(certificate));
    QCOMPARE(trustStatus->text(), QStringLiteral("In trust list"));
    QCOMPARE(trustButton->text(), QStringLiteral("Remove from trust list"));

    answerNextQuestion(DialogButtonBox::No);
    QTest::mouseClick(trustButton, Qt::LeftButton);
    QVERIFY(isInTrustList(certificate));
    QCOMPARE(trustButton->text(), QStringLiteral("Remove from trust list"));

    answerNextQuestion(DialogButtonBox::Yes);
    QTest::mouseClick(trustButton, Qt::LeftButton);
    QVERIFY(!isInTrustList(certificate));
    QCOMPARE(trustStatus->text(), QStringLiteral("Not in trust list"));
    QCOMPARE(trustButton->text(), QStringLiteral("Trust"));
}

void TestConnectionDialog::trustIsRefusedForACertificateOutsideItsValidity()
{
    DialogFakeBackend backend;
    ConnectionDialog dialog;
    dialog.setBackend(&backend);

    auto *trustButton = dialog.findChild<QPushButton *>(QStringLiteral("serverTrustButton"));
    QVERIFY(trustButton);

    // A certificate that fails the validity check keeps failing it on every connection, so
    // trusting it would change nothing.
    EndpointInfo endpoint = makeDialogEndpoint(QStringLiteral("opc.tcp://localhost:4840"));
    endpoint.securityMode = QStringLiteral("Sign & Encrypt");
    endpoint.securityModeValue = 3;
    endpoint.serverCertificate = QByteArrayLiteral("not-a-certificate");
    emit backend.endpointsDiscovered({endpoint}, {});

    QCOMPARE(trustButton->text(), QStringLiteral("Trust"));
    QVERIFY(!trustButton->isEnabled());
    QVERIFY(!trustButton->toolTip().isEmpty());
}

void TestConnectionDialog::endpointHoverDoesNotUseSelectionBackgroundInFusion()
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

QTEST_MAIN(TestConnectionDialog)

#include "test_connectiondialog.moc"
