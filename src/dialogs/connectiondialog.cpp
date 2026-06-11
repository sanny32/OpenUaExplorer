// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file connectiondialog.cpp
/// \brief Implements the OPC UA connection dialog.
///

#include <QCoreApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSysInfo>
#include <QUuid>

#include "connectiondialog.h"
#include "opcua/opcuaclientservice.h"
#include "opcua/pkimanager.h"
#include "ui_connectiondialog.h"
#include "widgets/coloredpushbutton.h"

///
/// \brief ConnectionDialog::ConnectionDialog
/// \param parent
///
ConnectionDialog::ConnectionDialog(QWidget *parent)
    : AppBaseDialog(parent)
    , ui(new Ui::ConnectionDialog)
{
    ui->setupUi(this);

    ui->clientCertificateHintIcon->setIcon(QStringLiteral("info"), QSize(24, 24));
    ui->serverCertificateIconLabel->setIcon(QStringLiteral("lock"), QSize(48, 48));
    ui->statusIconLabel->setIcon(QStringLiteral("disconnected"), QSize(16, 16));
    ui->connectButton->setColors({ QColor(0x0a74d1), QColor(0x1682df), QColor(0x075ca7) });

    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui->connectButton, &QPushButton::clicked,
            this, &ConnectionDialog::validateAndAccept);
    connect(ui->browseServersButton, &QPushButton::clicked,
            this, &ConnectionDialog::discoverEndpoints);
    connect(ui->endpointComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ConnectionDialog::updateEndpointSelection);
    connect(ui->authenticationComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ConnectionDialog::updateAuthenticationFields);
    connect(ui->clientCertificateViewButton, &QPushButton::clicked,
            this, &ConnectionDialog::chooseClientCertificate);
    connect(ui->trustListManageButton, &QPushButton::clicked,
            this, &ConnectionDialog::generateClientCertificate);
    connect(ui->showPasswordCheckBox, &QCheckBox::toggled, ui->passwordEdit, [this](bool checked) {
        ui->passwordEdit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
    });
    ui->clientCertificateComboBox->clear();
    ui->clientCertificateComboBox->addItem(tr("Auto-generate"));
    ui->clientCertificateComboBox->addItem(tr("Imported certificate"));
    ui->trustListManageButton->setText(tr("Generate..."));
    ui->clientCertificateViewButton->setText(tr("Import..."));
    updateAuthenticationFields();
}

///
/// \brief ConnectionDialog::~ConnectionDialog
///
ConnectionDialog::~ConnectionDialog()
{
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
    ui->backendComboBox->clear();
    if (_service) {
        ui->backendComboBox->addItems(_service->availableBackends());
        const int open62541 = ui->backendComboBox->findText(
            QStringLiteral("open62541"), Qt::MatchContains);
        if (open62541 >= 0)
            ui->backendComboBox->setCurrentIndex(open62541);
        connect(_service, &OpcUaClientService::endpointsDiscovered,
                this, &ConnectionDialog::handleEndpoints);
    }
    if (ui->backendComboBox->count() == 0)
        ui->backendComboBox->addItem(QStringLiteral("open62541"));
}

///
/// \brief ConnectionDialog::profile
/// \return Connection settings selected by the user.
///
ConnectionProfile ConnectionDialog::profile() const
{
    ConnectionProfile result;
    result.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    result.name = ui->endpointComboBox->currentText();
    result.backend = ui->backendComboBox->currentText();
    const int endpointIndex = ui->endpointComboBox->currentData().toInt();
    if (endpointIndex >= 0 && endpointIndex < _endpoints.size()) {
        const EndpointInfo &endpoint = _endpoints.at(endpointIndex);
        result.endpointUrl = endpoint.endpointUrl;
        result.securityPolicy = endpoint.securityPolicy;
        result.securityMode = endpoint.securityModeValue;
    } else {
        result.endpointUrl = ui->endpointComboBox->currentText();
        result.securityPolicy = ui->securityPolicyComboBox->currentText();
        result.securityMode = ui->securityModeComboBox->currentIndex() + 1;
    }
    result.authentication = static_cast<ConnectionProfile::Authentication>(
        ui->authenticationComboBox->currentData().isValid()
            ? ui->authenticationComboBox->currentData().toInt()
            : ui->authenticationComboBox->currentIndex());
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
/// \brief ConnectionDialog::discoverEndpoints
///
void ConnectionDialog::discoverEndpoints()
{
    if (!_service) {
        QMessageBox::critical(this, tr("OPC UA Unavailable"),
                              tr("The OPC UA client service is unavailable."));
        return;
    }
    const QString url = ui->endpointComboBox->currentText();
    ui->statusLabel->setText(tr("Discovering endpoints..."));
    ui->browseServersButton->setEnabled(false);
    ui->connectButton->setEnabled(false);
    _service->discoverEndpoints(url, ui->backendComboBox->currentText());
}

///
/// \brief ConnectionDialog::handleEndpoints
/// \param endpoints Discovered endpoints.
/// \param error Discovery error.
///
void ConnectionDialog::handleEndpoints(QList<EndpointInfo> endpoints, const QString &error)
{
    ui->browseServersButton->setEnabled(true);
    ui->connectButton->setEnabled(true);
    if (!error.isEmpty()) {
        ui->statusLabel->setText(error);
        return;
    }
    _endpoints = endpoints;
    ui->endpointComboBox->clear();
    for (int index = 0; index < _endpoints.size(); ++index) {
        const EndpointInfo &endpoint = _endpoints.at(index);
        const QString policy = endpoint.securityPolicy.section(QLatin1Char('#'), -1);
        ui->endpointComboBox->addItem(
            QStringLiteral("%1 [%2 / %3]")
                .arg(endpoint.endpointUrl, policy, endpoint.securityMode),
            index);
    }
    ui->statusLabel->setText(tr("%1 endpoints discovered.").arg(_endpoints.size()));
    updateEndpointSelection();
}

///
/// \brief ConnectionDialog::updateEndpointSelection
///
void ConnectionDialog::updateEndpointSelection()
{
    const int endpointIndex = ui->endpointComboBox->currentData().toInt();
    if (endpointIndex < 0 || endpointIndex >= _endpoints.size())
        return;
    const EndpointInfo &endpoint = _endpoints.at(endpointIndex);
    ui->securityModeComboBox->setCurrentIndex(qMax(0, endpoint.securityModeValue - 1));
    int policyIndex = ui->securityPolicyComboBox->findText(
        endpoint.securityPolicy.section(QLatin1Char('#'), -1));
    if (policyIndex < 0) {
        ui->securityPolicyComboBox->addItem(
            endpoint.securityPolicy.section(QLatin1Char('#'), -1));
        policyIndex = ui->securityPolicyComboBox->count() - 1;
    }
    ui->securityPolicyComboBox->setCurrentIndex(policyIndex);
    ui->endpointDescriptionEdit->setPlainText(
        QStringLiteral("%1\n%2 - %3")
            .arg(endpoint.endpointUrl, endpoint.securityPolicy, endpoint.securityMode));

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
    const int authentication = ui->authenticationComboBox->currentData().isValid()
        ? ui->authenticationComboBox->currentData().toInt()
        : ui->authenticationComboBox->currentIndex();
    const bool username = authentication
        == static_cast<int>(ConnectionProfile::Authentication::Username);
    const bool certificate = authentication
        == static_cast<int>(ConnectionProfile::Authentication::Certificate);
    ui->usernameEdit->setEnabled(username);
    ui->passwordEdit->setEnabled(username);
    ui->showPasswordCheckBox->setEnabled(username);
    ui->certificatesGroupBox->setEnabled(certificate
                                         || ui->securityModeComboBox->currentIndex() > 0);
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
}

///
/// \brief ConnectionDialog::generateClientCertificate
///
void ConnectionDialog::generateClientCertificate()
{
    PkiManager pki;
    QString error;
    if (!pki.generateClientCertificate(
            QCoreApplication::applicationName(),
            QStringLiteral("urn:%1:OpenUaExplorer").arg(QSysInfo::machineHostName()),
            &_clientCertificateFile, &_privateKeyFile, &error)) {
        QMessageBox::critical(this, tr("Certificate Generation Failed"), error);
        return;
    }
    _privateKeyPassword.clear();
    ui->clientCertificateComboBox->setCurrentIndex(0);
    ui->serverCertificateStatusLabel->setText(tr("Client certificate generated"));
}

///
/// \brief ConnectionDialog::validateAndAccept
///
void ConnectionDialog::validateAndAccept()
{
    const int endpointIndex = ui->endpointComboBox->currentData().toInt();
    if (_endpoints.isEmpty() || endpointIndex < 0 || endpointIndex >= _endpoints.size()) {
        discoverEndpoints();
        return;
    }
    const ConnectionProfile selectedProfile = profile();
    const bool needsCertificate = selectedProfile.securityMode > 1
        || selectedProfile.authentication == ConnectionProfile::Authentication::Certificate;
    if (needsCertificate && (_clientCertificateFile.isEmpty() || _privateKeyFile.isEmpty())) {
        generateClientCertificate();
        if (_clientCertificateFile.isEmpty() || _privateKeyFile.isEmpty())
            return;
    }
    if (selectedProfile.authentication == ConnectionProfile::Authentication::Username
        && ui->usernameEdit->text().isEmpty()) {
        QMessageBox::warning(this, tr("Missing Username"),
                             tr("Enter a username for this endpoint."));
        return;
    }
    accept();
}
