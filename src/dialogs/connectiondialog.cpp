// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file connectiondialog.cpp
/// \brief Implements the OPC UA connection dialog.
///

#include <QAbstractItemView>
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontMetrics>
#include <QInputDialog>
#include <QLineEdit>
#include <QSslCertificate>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QStringList>
#include <QStyle>
#include <QUuid>

#include "appicons.h"
#include "certificatetrustdialog.h"
#include "connectiondialog.h"
#include "opcua/opcuaclientservice.h"
#include "opcua/connectionprofilevalidator.h"
#include "opcua/pkimanager.h"
#include "ui_connectiondialog.h"
#include "widgets/certificatesummarywidget.h"
#include "widgets/coloredpushbutton.h"
#include "widgets/endpointdiscoverywidget.h"

namespace {

///
/// \brief Expands a combo box popup to fit its longest item.
/// \param comboBox Combo box whose popup width should be updated.
///
void updatePopupWidth(QComboBox *comboBox)
{
    int contentWidth = comboBox->width();
    const QFontMetrics metrics(comboBox->view()->font());
    for (int index = 0; index < comboBox->count(); ++index)
        contentWidth = qMax(contentWidth, metrics.horizontalAdvance(comboBox->itemText(index)));

    const int popupPadding = comboBox->style()->pixelMetric(QStyle::PM_ScrollBarExtent)
        + comboBox->style()->pixelMetric(QStyle::PM_ComboBoxFrameWidth) * 2
        + 32;
    contentWidth += popupPadding;

    if (const QScreen *screen = comboBox->screen())
        contentWidth = qMin(contentWidth, screen->availableGeometry().width() - 40);

    comboBox->view()->setMinimumWidth(contentWidth);
}

}

///
/// \brief ConnectionDialog::ConnectionDialog
/// \param parent
///
ConnectionDialog::ConnectionDialog(QWidget *parent)
    : AppBaseDialog(parent)
    , ui(new Ui::ConnectionDialog)
{
    ui->setupUi(this);

    setupEndpointHistory();
    setupCertificatePanels();
    setupControls();
    setupConnections();
    updateAuthenticationFields();
}

///
/// \brief ConnectionDialog::setupEndpointHistory
///
/// Populates the endpoint URL combo box from the persisted history.
///
void ConnectionDialog::setupEndpointHistory()
{
    QStringList endpointHistory = _endpointHistoryStore.history();
    if (endpointHistory.isEmpty())
        endpointHistory.append(ui->discoveryUrlComboBox->currentText());

    ui->discoveryUrlComboBox->clear();
    ui->discoveryUrlComboBox->addItems(endpointHistory);
    _lastEnteredEndpointUrl = endpointHistory.constFirst();
    ui->discoveryUrlComboBox->setEditText(_lastEnteredEndpointUrl);
    updatePopupWidth(ui->discoveryUrlComboBox);
}

///
/// \brief ConnectionDialog::setupCertificatePanels
///
/// Configures the server and client certificate summary panels and seeds the
/// client certificate selection from any existing auto-generated certificate.
///
void ConnectionDialog::setupCertificatePanels()
{
    ui->serverCertificateWidget->setTitle(tr("Server Certificate"));
    ui->serverCertificateWidget->setHint(
        tr("Select an endpoint that provides a server certificate."));
    ui->serverCertificateWidget->clear();

    ui->clientCertificateWidget->setTitle(tr("Client Certificate"));
    ui->clientCertificateWidget->setEmptyText(tr("No client certificate"));

    ui->clientCertificateComboBox->clear();
    ui->clientCertificateComboBox->addItem(tr("Auto-generate"));
    ui->clientCertificateComboBox->addItem(tr("Imported certificate"));
    ui->clientCertificateComboBox->setSizePolicy(
        QSizePolicy::Expanding, ui->clientCertificateComboBox->sizePolicy().verticalPolicy());
    ui->clientCertificateLayout->setStretch(1, 1);
    ui->clientCertificateLayout->setStretch(2, 0);
    ui->trustListManageButton->setText(tr("Generate..."));

    PkiManager pki;
    if (pki.existingClientCertificate(&_clientCertificateFile, &_privateKeyFile)) {
        ui->clientCertificateComboBox->setItemText(
            0, tr("Auto-generated (%1)").arg(QFileInfo(_clientCertificateFile).fileName()));
    }
    updateClientCertificateAction();
    updateClientCertificate();
}

///
/// \brief ConnectionDialog::setupControls
///
/// Applies icons, colours and the password visibility toggle.
///
void ConnectionDialog::setupControls()
{
    ui->statusIconLabel->setIcon(QStringLiteral("disconnected"), QSize(16, 16));
    ui->connectButton->setColors({ QColor(0x0a74d1), QColor(0x1682df), QColor(0x075ca7) });
    ui->getEndpointsButton->setIcon(QStringLiteral("search.svg"));

    const bool darkTheme = AppIcons::isDarkTheme();
    const QString hintStyle = QStringLiteral("color: %1;")
        .arg(darkTheme ? QStringLiteral("#7c828b") : QStringLiteral("#9aa0a6"));
    ui->usernameHintLabel->setStyleSheet(hintStyle);
    ui->passwordHintLabel->setStyleSheet(hintStyle);

    QAction *passwordToggle = ui->passwordEdit->addAction(
        AppIcons::themed(QStringLiteral("eye.svg")), QLineEdit::TrailingPosition);
    passwordToggle->setCheckable(true);
    auto refreshPasswordToggle = [this, passwordToggle] {
        const bool shown = passwordToggle->isChecked();
        ui->passwordEdit->setEchoMode(shown ? QLineEdit::Normal : QLineEdit::Password);
        passwordToggle->setIcon(AppIcons::themed(
            shown ? QStringLiteral("eye-off.svg") : QStringLiteral("eye.svg")));
        passwordToggle->setToolTip(shown ? tr("Hide password") : tr("Show password"));
    };
    refreshPasswordToggle();
    connect(passwordToggle, &QAction::toggled, this, refreshPasswordToggle);
}

///
/// \brief ConnectionDialog::setupConnections
///
/// Wires the dialog widgets to their handlers.
///
void ConnectionDialog::setupConnections()
{
    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui->connectButton, &QPushButton::clicked,
            this, &ConnectionDialog::validateAndAccept);
    connect(ui->getEndpointsButton, &QPushButton::clicked,
            this, &ConnectionDialog::discoverEndpoints);
    connect(ui->endpointsWidget, &EndpointDiscoveryWidget::currentEndpointChanged,
            this, &ConnectionDialog::updateEndpointSelection);
    connect(ui->discoveryUrlComboBox, QOverload<int>::of(&QComboBox::activated),
            this, [this](int index) {
        resetDiscovery();
        _lastEnteredEndpointUrl = ui->discoveryUrlComboBox->itemText(index);
    });
    connect(ui->discoveryUrlComboBox->lineEdit(), &QLineEdit::textEdited,
            this, [this](const QString &text) {
        resetDiscovery();
        _lastEnteredEndpointUrl = text;
    });
    connect(ui->discoveryUrlComboBox->lineEdit(), &QLineEdit::editingFinished, this, [this]() {
        saveLastEndpointUrl();
        ui->discoveryUrlComboBox->lineEdit()->setCursorPosition(0);
    });
    connect(ui->authenticationComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ConnectionDialog::updateAuthenticationFields);
    connect(ui->clientCertificateComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ConnectionDialog::updateClientCertificateAction);
    connect(ui->clientCertificateViewButton, &QPushButton::clicked,
            this, &ConnectionDialog::handleClientCertificateAction);
    connect(ui->clientCertificateWidget, &CertificateSummaryWidget::viewRequested,
            this, &ConnectionDialog::viewClientCertificate);
    connect(ui->serverCertificateWidget, &CertificateSummaryWidget::viewRequested,
            this, &ConnectionDialog::viewServerCertificate);
    connect(ui->trustListManageButton, &QPushButton::clicked,
            this, &ConnectionDialog::generateClientCertificate);
}

///
/// \brief ConnectionDialog::currentAuthentication
/// \return Selected authentication mode value.
///
int ConnectionDialog::currentAuthentication() const
{
    return ui->authenticationComboBox->currentData().isValid()
        ? ui->authenticationComboBox->currentData().toInt()
        : ui->authenticationComboBox->currentIndex();
}

///
/// \brief ConnectionDialog::~ConnectionDialog
///
ConnectionDialog::~ConnectionDialog()
{
    saveLastEndpointUrl();
    delete ui;
}

///
/// \brief ConnectionDialog::setClientService
/// \param service OPC UA client service.
///
void ConnectionDialog::setClientService(OpcUaClientService *service)
{
    if (_service)
        disconnect(_service, nullptr, this, nullptr);
    _service = service;
    if (_service) {
        connect(_service, &OpcUaClientService::endpointsDiscovered,
                this, &ConnectionDialog::handleEndpoints);
    }
}

///
/// \brief ConnectionDialog::profile
/// \return Connection settings selected by the user.
///
ConnectionProfile ConnectionDialog::profile() const
{
    ConnectionProfile result;
    result.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (ui->endpointsWidget->hasSelection()) {
        const EndpointInfo endpoint = ui->endpointsWidget->currentEndpoint();
        result.name = endpoint.endpointUrl;
        result.endpointUrl = endpoint.endpointUrl;
        result.securityPolicy = endpoint.securityPolicy;
        result.securityMode = endpoint.securityModeValue;
    } else {
        result.name = ui->discoveryUrlComboBox->currentText();
        result.endpointUrl = result.name;
        result.securityPolicy = QStringLiteral("None");
        result.securityMode = 1;
    }
    result.authentication =
        static_cast<ConnectionProfile::Authentication>(currentAuthentication());
    result.username = ui->usernameEdit->text();
    result.clientCertificateFile = _clientCertificateFile;
    result.privateKeyFile = _privateKeyFile;
    result.sessionTimeoutMs = ui->sessionTimeoutSpinBox->value();
    result.connectTimeoutMs = ui->connectTimeoutSpinBox->value();
    result.secureChannelLifetimeMs = ui->secureChannelLifetimeSpinBox->value();
    result.endpointTimeoutMs = ui->endpointTimeoutSpinBox->value();
    result.requestTimeoutMs = ui->requestTimeoutSpinBox->value();
    result.saveProfile = ui->saveFavoriteCheckBox->isChecked();
    return result;
}

///
/// \brief ConnectionDialog::password
/// \return Username password.
///
QString ConnectionDialog::password() const
{
    return ui->passwordEdit->text();
}

///
/// \brief ConnectionDialog::privateKeyPassword
/// \return Imported private key password.
///
QString ConnectionDialog::privateKeyPassword() const
{
    return _privateKeyPassword;
}

///
/// \brief ConnectionDialog::resetDiscovery
///
/// Clears the discovered endpoints, the server certificate and the status
/// indicator so no stale discovery result is shown for a new endpoint URL.
///
void ConnectionDialog::resetDiscovery()
{
    ui->endpointsWidget->clear();
    ui->serverCertificateWidget->clear();
    ui->statusIconLabel->setIcon(QStringLiteral("disconnected"), QSize(16, 16));
    ui->statusLabel->setText(tr("Disconnected"));
}

///
/// \brief ConnectionDialog::discoverEndpoints
///
void ConnectionDialog::discoverEndpoints()
{
    if (!_service) {
        _connectAfterDiscovery = false;
        QMessageBox::critical(this, tr("OPC UA Unavailable"),
                              tr("The OPC UA client service is unavailable."));
        return;
    }
    saveLastEndpointUrl();
    const QString url = ui->discoveryUrlComboBox->currentText();
    resetDiscovery();
    ui->statusLabel->setText(tr("Discovering endpoints..."));
    ui->getEndpointsButton->setEnabled(false);
    ui->connectButton->setEnabled(false);
    _service->discoverEndpoints(url);
}

///
/// \brief ConnectionDialog::handleEndpoints
/// \param endpoints Discovered endpoints.
/// \param error Discovery error.
///
void ConnectionDialog::handleEndpoints(QList<EndpointInfo> endpoints, const QString &error)
{
    ui->getEndpointsButton->setEnabled(true);
    ui->connectButton->setEnabled(true);
    if (!error.isEmpty()) {
        _connectAfterDiscovery = false;
        ui->statusLabel->setText(error);
        return;
    }
    ui->endpointsWidget->setEndpoints(endpoints);
    if (endpoints.isEmpty()) {
        _connectAfterDiscovery = false;
        ui->statusLabel->setText(tr("No endpoints discovered."));
        return;
    }
    ui->statusLabel->setText(tr("%1 endpoints discovered.").arg(endpoints.size()));
    ui->statusIconLabel->setIcon(QStringLiteral("check-circle"), QSize(16, 16));
    if (_connectAfterDiscovery) {
        _connectAfterDiscovery = false;
        validateAndAccept();
    }
}

///
/// \brief ConnectionDialog::updateEndpointSelection
///
void ConnectionDialog::updateEndpointSelection()
{
    if (!ui->endpointsWidget->hasSelection())
        return;
    const EndpointInfo endpoint = ui->endpointsWidget->currentEndpoint();
    ui->serverCertificateWidget->setCertificate(endpoint.serverCertificate);
    _selectedSecurityModeValue = endpoint.securityModeValue;
    const int previousAuthentication = ui->authenticationComboBox->currentIndex();
    ui->authenticationComboBox->clear();
    if (endpoint.supportsAnonymous)
        ui->authenticationComboBox->addItem(tr("Anonymous"),
                                            static_cast<int>(ConnectionProfile::Authentication::Anonymous));
    if (endpoint.supportsUsername)
        ui->authenticationComboBox->addItem(tr("Username/Password"),
                                            static_cast<int>(ConnectionProfile::Authentication::Username));
    if (endpoint.supportsCertificate)
        ui->authenticationComboBox->addItem(tr("Certificate"),
                                            static_cast<int>(ConnectionProfile::Authentication::Certificate));
    ui->authenticationComboBox->setCurrentIndex(
        qBound(0, previousAuthentication, ui->authenticationComboBox->count() - 1));
    updateAuthenticationFields();
}

///
/// \brief ConnectionDialog::updateAuthenticationFields
///
void ConnectionDialog::updateAuthenticationFields()
{
    const int authentication = currentAuthentication();
    const bool username = authentication
        == static_cast<int>(ConnectionProfile::Authentication::Username);
    const bool certificate = authentication
        == static_cast<int>(ConnectionProfile::Authentication::Certificate);
    ui->usernameEdit->setEnabled(username);
    ui->passwordEdit->setEnabled(username);
    ui->usernameHintLabel->setVisible(!username);
    ui->passwordHintLabel->setVisible(!username);

    // The client certificate stays available regardless of the security mode
    // (it can still be imported, generated or inspected). Only the server
    // certificate and trust settings depend on a secure channel.
    const bool secureChannel = certificate || _selectedSecurityModeValue > 1;
    ui->serverCertificateGroupBox->setEnabled(secureChannel);
    ui->trustServerCertificateCheckBox->setEnabled(secureChannel);
    ui->trustListLabel->setEnabled(secureChannel);
    ui->trustListComboBox->setEnabled(secureChannel);
    ui->trustListManageButton->setEnabled(secureChannel);
}

///
/// \brief ConnectionDialog::chooseClientCertificate
///
void ConnectionDialog::chooseClientCertificate()
{
    const QString certificate = QFileDialog::getOpenFileName(
        this, tr("Select Client Certificate"), QString(),
        tr("Certificates (*.der *.pem *.crt);;All Files (*)"));
    if (certificate.isEmpty())
        return;
    const QString key = QFileDialog::getOpenFileName(
        this, tr("Select Private Key"), QString(),
        tr("Private Keys (*.pem *.key);;All Files (*)"));
    if (key.isEmpty())
        return;
    _clientCertificateFile = certificate;
    _privateKeyFile = key;
    bool ok = false;
    _privateKeyPassword = QInputDialog::getText(
        this, tr("Private Key Password"),
        tr("Password (leave empty for an unencrypted key):"),
        QLineEdit::Password, QString(), &ok);
    if (!ok)
        _privateKeyPassword.clear();
    ui->clientCertificateComboBox->setCurrentIndex(1);
    ui->clientCertificateComboBox->setItemText(1, QFileInfo(certificate).fileName());
    updateClientCertificate();
}

///
/// \brief ConnectionDialog::generateClientCertificate
///
void ConnectionDialog::generateClientCertificate()
{
    PkiManager pki;
    QString error;
    if (!pki.generateClientCertificate(
            PkiManager::clientCertificateCommonName(),
            PkiManager::applicationUri(),
            &_clientCertificateFile, &_privateKeyFile, &error)) {
        QMessageBox::critical(this, tr("Certificate Generation Failed"), error);
        return;
    }
    _privateKeyPassword.clear();
    ui->clientCertificateComboBox->setCurrentIndex(0);
    updateClientCertificateAction();
    updateClientCertificate();
}

///
/// \brief ConnectionDialog::handleClientCertificateAction
///
void ConnectionDialog::handleClientCertificateAction()
{
    if (ui->clientCertificateComboBox->currentIndex() == 0)
        generateClientCertificate();
    else
        chooseClientCertificate();
}

///
/// \brief ConnectionDialog::viewServerCertificate
///
void ConnectionDialog::viewServerCertificate()
{
    const QByteArray certificate = ui->serverCertificateWidget->certificate();
    if (certificate.isEmpty())
        return;

    CertificateTrustDialog dialog(this);
    dialog.setViewOnly(true);
    dialog.setCertificate(
        certificate,
        tr("Certificate advertised by the selected OPC UA endpoint."));
    dialog.exec();
}

///
/// \brief ConnectionDialog::validateAndAccept
///
void ConnectionDialog::validateAndAccept()
{
    if (!ui->endpointsWidget->hasSelection()) {
        _connectAfterDiscovery = true;
        discoverEndpoints();
        return;
    }
    const ConnectionProfile selectedProfile = profile();
    if (ConnectionProfileValidator::validate(selectedProfile)
        == ConnectionProfileValidator::Error::MissingClientCertificate) {
        generateClientCertificate();
        if (_clientCertificateFile.isEmpty() || _privateKeyFile.isEmpty())
            return;
    }
    if (ConnectionProfileValidator::validate(profile())
        == ConnectionProfileValidator::Error::MissingUsername) {
        QMessageBox::warning(this, tr("Missing Username"),
                             tr("Enter a username for this endpoint."));
        return;
    }
    accept();
}

///
/// \brief ConnectionDialog::saveLastEndpointUrl
///
void ConnectionDialog::saveLastEndpointUrl()
{
    const QString endpointUrl = _lastEnteredEndpointUrl.trimmed();
    if (endpointUrl.isEmpty())
        return;

    _endpointHistoryStore.save(endpointUrl);
    const QStringList endpointHistory = _endpointHistoryStore.history();

    const QSignalBlocker blocker(ui->discoveryUrlComboBox);
    ui->discoveryUrlComboBox->clear();
    ui->discoveryUrlComboBox->addItems(endpointHistory);
    ui->discoveryUrlComboBox->setEditText(endpointUrl);
    updatePopupWidth(ui->discoveryUrlComboBox);
}

///
/// \brief ConnectionDialog::updateClientCertificate
///
/// Reads the currently selected client certificate file and shows its details.
///
void ConnectionDialog::updateClientCertificate()
{
    QByteArray data;
    if (!_clientCertificateFile.isEmpty()) {
        QFile file(_clientCertificateFile);
        if (file.open(QIODevice::ReadOnly))
            data = file.readAll();
    }

    QList<QSslCertificate> chain = QSslCertificate::fromData(data, QSsl::Der);
    if (chain.isEmpty())
        chain = QSslCertificate::fromData(data, QSsl::Pem);
    ui->clientCertificateWidget->setCertificate(
        chain.isEmpty() ? QByteArray() : chain.constFirst().toDer());
}

///
/// \brief ConnectionDialog::updateClientCertificateAction
///
void ConnectionDialog::updateClientCertificateAction()
{
    ui->clientCertificateViewButton->setText(
        ui->clientCertificateComboBox->currentIndex() == 0 ? tr("Generate...") : tr("Import..."));
}

///
/// \brief ConnectionDialog::viewClientCertificate
///
void ConnectionDialog::viewClientCertificate()
{
    const QByteArray certificate = ui->clientCertificateWidget->certificate();
    if (certificate.isEmpty())
        return;

    CertificateTrustDialog dialog(this);
    dialog.setViewOnly(true);
    dialog.setCertificate(certificate,
                          tr("Certificate presented by this application to the server."));
    dialog.exec();
}
