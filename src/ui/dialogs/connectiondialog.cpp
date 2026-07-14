// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file connectiondialog.cpp
/// \brief Implements the OPC UA connection dialog.
///

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QLineEdit>
#include <QSslCertificate>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QStringList>
#include <QUuid>

#include "appcolors.h"
#include "appicons.h"
#include "appsettings.h"
#include "certificatedetailsdialog.h"
#include "connectiondialog.h"
#include "messageboxdialog.h"
#include "opcua/certificateinfo.h"
#include "opcua/opcuabackend.h"
#include "opcua/connectionprofilevalidator.h"
#include "opcua/pkimanager.h"
#include "ui_connectiondialog.h"
#include "widgets/certificatesummarywidget.h"
#include "widgets/coloredpushbutton.h"
#include "widgets/endpointdiscoverywidget.h"
#include "widgets/historycombobox.h"

///
/// \brief Builds the dialog and initialises its history, certificate panels, and controls.
/// \param parent Parent widget.
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
    applySessionDefaults();
    updateAuthenticationFields();
}

///
/// \brief Seeds the Advanced Settings controls from the application-wide session defaults.
///
void ConnectionDialog::applySessionDefaults()
{
    const AppSettings::SessionDefaults defaults = AppSettings().sessionDefaults();
    ui->sessionTimeoutSpinBox->setValue(defaults.sessionTimeoutMs);
    ui->endpointTimeoutSpinBox->setValue(defaults.endpointTimeoutMs);
    ui->connectTimeoutSpinBox->setValue(defaults.connectTimeoutMs);
    ui->requestTimeoutSpinBox->setValue(defaults.requestTimeoutMs);
    ui->secureChannelLifetimeSpinBox->setValue(defaults.secureChannelLifetimeMs);
    ui->maxMessageSizeSpinBox->setValue(defaults.maxMessageSizeBytes);
}

///
/// \brief Persists the Advanced Settings controls as the application-wide session defaults.
///
void ConnectionDialog::saveSessionDefaults()
{
    AppSettings::SessionDefaults defaults;
    defaults.sessionTimeoutMs = ui->sessionTimeoutSpinBox->value();
    defaults.endpointTimeoutMs = ui->endpointTimeoutSpinBox->value();
    defaults.connectTimeoutMs = ui->connectTimeoutSpinBox->value();
    defaults.requestTimeoutMs = ui->requestTimeoutSpinBox->value();
    defaults.secureChannelLifetimeMs = ui->secureChannelLifetimeSpinBox->value();
    defaults.maxMessageSizeBytes = ui->maxMessageSizeSpinBox->value();
    AppSettings().setSessionDefaults(defaults);
}

///
/// \brief Populates the endpoint URL combo box from the persisted history.
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
}

///
/// \brief Configures the server and client certificate panels, seeding any existing
///        auto-generated client certificate.
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
        selectGeneratedClientCertificate();
    }
    updateClientCertificateAction();
    updateClientCertificate();
}

///
/// \brief Applies icons, colours, and the password visibility toggle.
///
void ConnectionDialog::setupControls()
{
    ui->statusIconLabel->setIcon(QStringLiteral("disconnected"), QSize(16, 16));
    ui->connectButton->setColors(
        { AppColors::accent(), AppColors::accentHover(), AppColors::accentPressed() });
    ui->getEndpointsButton->setIcon(QStringLiteral("search"));

    ui->certificateLayout->setAlignment(ui->certificateBrowseButton, Qt::AlignLeft);
    ui->certificateLayout->setAlignment(ui->privateKeyBrowseButton, Qt::AlignLeft);
    ui->certificateBrowseButton->setFixedHeight(ui->certificateEdit->sizeHint().height());
    ui->privateKeyBrowseButton->setFixedHeight(ui->privateKeyEdit->sizeHint().height());

    for (QLineEdit *edit : { ui->passwordEdit, ui->privateKeyPasswordEdit }) {
        QAction *toggle = edit->addAction(
            AppIcons::themed(QStringLiteral("eye")), QLineEdit::TrailingPosition);
        toggle->setCheckable(true);
        auto refreshToggle = [edit, toggle, this] {
            const bool shown = toggle->isChecked();
            edit->setEchoMode(shown ? QLineEdit::Normal : QLineEdit::Password);
            toggle->setIcon(AppIcons::themed(
                shown ? QStringLiteral("eye-off") : QStringLiteral("eye")));
            toggle->setToolTip(shown ? tr("Hide password") : tr("Show password"));
        };
        refreshToggle();
        connect(toggle, &QAction::toggled, this, refreshToggle);
    }
}

///
/// \brief Wires the dialog widgets to their handlers.
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
    connect(ui->discoveryUrlComboBox, &HistoryComboBox::itemRemoved,
            this, &ConnectionDialog::forgetEndpointUrl);
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
    connect(ui->certificateBrowseButton, &QPushButton::clicked,
            this, &ConnectionDialog::browseCertificateAuthCertificate);
    connect(ui->privateKeyBrowseButton, &QPushButton::clicked,
            this, &ConnectionDialog::browseCertificateAuthPrivateKey);
    connect(ui->certificateEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        _clientCertificateFile = text;
        updateClientCertificate();
    });
    connect(ui->privateKeyEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        _privateKeyFile = text;
    });
    connect(ui->clientCertificateComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ConnectionDialog::updateClientCertificateAction);
    connect(ui->clientCertificateViewButton, &QPushButton::clicked,
            this, &ConnectionDialog::handleClientCertificateAction);
    connect(ui->clientCertificateWidget, &CertificateSummaryWidget::viewDetailsRequested,
            this, &ConnectionDialog::viewClientCertificateDetails);
    connect(ui->serverCertificateWidget, &CertificateSummaryWidget::viewDetailsRequested,
            this, &ConnectionDialog::viewServerCertificateDetails);
    connect(ui->trustListManageButton, &QPushButton::clicked,
            this, &ConnectionDialog::generateClientCertificate);
}

///
/// \brief Returns the selected authentication mode.
/// \return Selected authentication mode value.
///
int ConnectionDialog::currentAuthentication() const
{
    return ui->authenticationComboBox->currentData().isValid()
        ? ui->authenticationComboBox->currentData().toInt()
        : ui->authenticationComboBox->currentIndex();
}

///
/// \brief Saves the last endpoint URL and destroys the dialog.
///
ConnectionDialog::~ConnectionDialog()
{
    saveLastEndpointUrl();
    delete ui;
}

///
/// \brief Sets the backend used for discovery and subscribes to its results.
/// \param backend OPC UA backend.
///
void ConnectionDialog::setBackend(OpcUaBackend *backend)
{
    if (_service)
        disconnect(_service, nullptr, this, nullptr);
    _service = backend;
    if (_service) {
        connect(_service, &OpcUaBackend::endpointsDiscovered,
                this, &ConnectionDialog::handleEndpoints);
    }
}

///
/// \brief Builds a connection profile from the dialog's current selections.
/// \return Connection settings selected by the user.
///
ConnectionProfile ConnectionDialog::profile() const
{
    ConnectionProfile result;
    result.id = _presetId.isEmpty()
        ? QUuid::createUuid().toString(QUuid::WithoutBraces)
        : _presetId;
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
    result.maxMessageSizeBytes = ui->maxMessageSizeSpinBox->value();
    return result;
}

///
/// \brief Pre-fills the dialog from a saved profile so a favourite opens ready to connect.
/// \param profile Profile whose id, endpoint, authentication, and timeouts are applied.
///
void ConnectionDialog::setProfile(const ConnectionProfile &profile)
{
    if (profile.endpointUrl.isEmpty())
        return;

    _presetId = profile.id;
    resetDiscovery();
    _lastEnteredEndpointUrl = profile.endpointUrl;
    {
        const QSignalBlocker blocker(ui->discoveryUrlComboBox);
        ui->discoveryUrlComboBox->setEditText(profile.endpointUrl);
        ui->discoveryUrlComboBox->lineEdit()->setCursorPosition(0);
    }

    const int authIndex =
        ui->authenticationComboBox->findData(static_cast<int>(profile.authentication));
    if (authIndex >= 0)
        ui->authenticationComboBox->setCurrentIndex(authIndex);
    ui->usernameEdit->setText(profile.username);

    if (profile.sessionTimeoutMs > 0)
        ui->sessionTimeoutSpinBox->setValue(profile.sessionTimeoutMs);
    if (profile.connectTimeoutMs > 0)
        ui->connectTimeoutSpinBox->setValue(profile.connectTimeoutMs);
    if (profile.secureChannelLifetimeMs > 0)
        ui->secureChannelLifetimeSpinBox->setValue(profile.secureChannelLifetimeMs);
    if (profile.endpointTimeoutMs > 0)
        ui->endpointTimeoutSpinBox->setValue(profile.endpointTimeoutMs);
    if (profile.requestTimeoutMs > 0)
        ui->requestTimeoutSpinBox->setValue(profile.requestTimeoutMs);
    if (profile.maxMessageSizeBytes > 0)
        ui->maxMessageSizeSpinBox->setValue(profile.maxMessageSizeBytes);

    updateAuthenticationFields();
}

///
/// \brief Returns the entered username password.
/// \return Username password.
///
QString ConnectionDialog::password() const
{
    return ui->passwordEdit->text();
}

///
/// \brief Returns the password for the imported private key.
/// \return Imported private key password.
///
QString ConnectionDialog::privateKeyPassword() const
{
    return ui->privateKeyPasswordEdit->text();
}

///
/// \brief Clears stale discovery results (endpoints, server certificate, status) for a new URL.
///
void ConnectionDialog::resetDiscovery()
{
    ui->endpointsWidget->clear();
    ui->serverCertificateWidget->clear();
    ui->statusIconLabel->setIcon(QStringLiteral("disconnected"), QSize(16, 16));
    ui->statusLabel->setText(tr("Disconnected"));
}

///
/// \brief Starts endpoint discovery for the entered URL.
///
void ConnectionDialog::discoverEndpoints()
{
    if (!_service) {
        _connectAfterDiscovery = false;
        MessageBoxDialog::critical(this, tr("OPC UA Unavailable"),
                                   tr("The OPC UA backend is unavailable."));
        return;
    }
    saveLastEndpointUrl();
    const QString url = ui->discoveryUrlComboBox->currentText();
    const ConnectionProfile settings = profile();
    resetDiscovery();
    ui->statusLabel->setText(tr("Discovering endpoints..."));
    ui->getEndpointsButton->setEnabled(false);
    ui->connectButton->setEnabled(false);
    _service->discoverEndpoints(url, settings.backend, settings.endpointTimeoutMs);
}

///
/// \brief Shows the discovery result and, when queued, continues to connect.
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
/// \brief Shows the selected endpoint's certificate and rebuilds the authentication choices.
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
/// \brief Enables the username and certificate fields appropriate to the chosen auth and security.
///
void ConnectionDialog::updateAuthenticationFields()
{
    const int authentication = currentAuthentication();
    const bool username = authentication
        == static_cast<int>(ConnectionProfile::Authentication::Username);
    const bool certificate = authentication
        == static_cast<int>(ConnectionProfile::Authentication::Certificate);
    ui->authStack->setCurrentWidget(certificate ? ui->certificatePanel : ui->usernamePanel);

    ui->usernameEdit->setEnabled(username);
    ui->passwordEdit->setEnabled(username);

    const bool showHintText = !certificate && !username;
    const QString hintStyle = showHintText
        ? QStringLiteral("color: %1;").arg(AppColors::hint().name())
        : QStringLiteral("color: transparent;");
    ui->usernameHintLabel->setStyleSheet(hintStyle);
    ui->passwordHintLabel->setStyleSheet(hintStyle);

    const int labelColumn = ui->authenticationLabel->sizeHint().width();
    const int hintColumn = qMax(ui->usernameHintLabel->sizeHint().width(),
                                ui->passwordHintLabel->sizeHint().width());
    ui->authenticationLayout->setColumnMinimumWidth(0, labelColumn);
    ui->authenticationLayout->setColumnMinimumWidth(2, hintColumn);
    ui->usernameLayout->setColumnMinimumWidth(0, labelColumn);
    ui->usernameLayout->setColumnMinimumWidth(2, hintColumn);
    ui->certificateLayout->setColumnMinimumWidth(0, labelColumn);
    ui->certificateLayout->setColumnMinimumWidth(2, hintColumn);

    // The client certificate stays available regardless of the security mode
    // (it can still be imported, generated or inspected). Only the server
    // certificate and trust settings depend on a secure channel.
    const bool secureChannel = certificate || _selectedSecurityModeValue > 1;
    ui->serverCertificateGroupBox->setEnabled(secureChannel);
    ui->trustListLabel->setEnabled(secureChannel);
    ui->trustListComboBox->setEnabled(secureChannel);
    ui->trustListManageButton->setEnabled(secureChannel);
    updateTrustServerCertificate(secureChannel);
}

///
/// \brief Offers automatic trust only for a server certificate inside its validity period.
/// \param secureChannel Whether the selected endpoint secures the channel at all.
/// \note Trusting an expired or not yet valid certificate would not help: it keeps failing
///       validation on every connection, so the prompt comes back regardless.
///
void ConnectionDialog::updateTrustServerCertificate(bool secureChannel)
{
    const QByteArray certificate = ui->endpointsWidget->hasSelection()
        ? ui->endpointsWidget->currentEndpoint().serverCertificate
        : QByteArray();
    const bool withinValidity = !certificate.isEmpty()
        && CertificateInfo::fromDer(certificate).status == CertificateInfo::Status::Valid;
    const bool trustable = secureChannel && withinValidity;

    if (!trustable)
        ui->trustServerCertificateCheckBox->setChecked(false);
    ui->trustServerCertificateCheckBox->setEnabled(trustable);
    ui->trustServerCertificateCheckBox->setToolTip(
        secureChannel && !withinValidity
            ? tr("The server certificate is outside its validity period. Trusting it would not "
                 "help: it would still fail validation on every connection.")
            : QString());
}

///
/// \brief Prompts for a client certificate, private key, and optional key password.
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
    ui->privateKeyPasswordEdit->clear();
    ui->clientCertificateComboBox->setCurrentIndex(1);
    ui->clientCertificateComboBox->setItemText(1, QFileInfo(certificate).fileName());
    updateClientCertificate();
}

///
/// \brief Browses for the certificate used by certificate authentication.
///
void ConnectionDialog::browseCertificateAuthCertificate()
{
    const QString certificate = QFileDialog::getOpenFileName(
        this, tr("Select Client Certificate"), _clientCertificateFile,
        tr("Certificates (*.der *.pem *.crt);;All Files (*)"));
    if (certificate.isEmpty())
        return;
    ui->certificateEdit->setText(certificate);
}

///
/// \brief Browses for the private key used by certificate authentication.
///
void ConnectionDialog::browseCertificateAuthPrivateKey()
{
    const QString key = QFileDialog::getOpenFileName(
        this, tr("Select Private Key"), _privateKeyFile,
        tr("Private Keys (*.pem *.key);;All Files (*)"));
    if (key.isEmpty())
        return;
    ui->privateKeyEdit->setText(key);
    ui->privateKeyPasswordEdit->clear();
}

///
/// \brief Generates a self-signed client certificate and selects it.
///
void ConnectionDialog::generateClientCertificate()
{
    PkiManager pki;
    QString error;
    if (!pki.generateClientCertificate(
            PkiManager::clientCertificateCommonName(),
            PkiManager::applicationUri(),
            &_clientCertificateFile, &_privateKeyFile, &error)) {
        MessageBoxDialog::critical(this, tr("Certificate Generation Failed"), error);
        return;
    }
    ui->privateKeyPasswordEdit->clear();
    selectGeneratedClientCertificate();
}

///
/// \brief Generates or imports a client certificate depending on the selected mode.
///
void ConnectionDialog::handleClientCertificateAction()
{
    if (ui->clientCertificateComboBox->currentIndex() == 0)
        generateClientCertificate();
    else
        chooseClientCertificate();
}

///
/// \brief Opens the configured client certificate details.
///
void ConnectionDialog::viewClientCertificateDetails()
{
    showCertificateDetails(ui->clientCertificateWidget->certificate(), _clientCertificateFile);
}

///
/// \brief Opens the selected endpoint's server certificate details.
///
void ConnectionDialog::viewServerCertificateDetails()
{
    showCertificateDetails(ui->serverCertificateWidget->certificate());
}

///
/// \brief Validates the profile (discovering or generating a certificate first) before accepting.
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
        MessageBoxDialog::warning(this, tr("Missing Username"),
                                  tr("Enter a username for this endpoint."),
                                  DialogButtonBox::Ok);
        return;
    }
    saveSessionDefaults();
    accept();
}

///
/// \brief Persists the last entered endpoint URL and refreshes the history combo box.
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
}

///
/// \brief Forgets an endpoint URL the user removed from the history combo box.
/// \param endpointUrl URL that was removed.
///
void ConnectionDialog::forgetEndpointUrl(const QString &endpointUrl)
{
    _endpointHistoryStore.remove(endpointUrl);
    _lastEnteredEndpointUrl = ui->discoveryUrlComboBox->currentText();
}

///
/// \brief Selects the generated client certificate and mirrors its paths into the dialog.
///
void ConnectionDialog::selectGeneratedClientCertificate()
{
    ui->clientCertificateComboBox->setItemText(
        0, tr("Auto-generated (%1)").arg(QFileInfo(_clientCertificateFile).fileName()));
    ui->clientCertificateComboBox->setCurrentIndex(0);
    updateClientCertificateAction();
    updateClientCertificate();
}

///
/// \brief Reads the selected client certificate file and shows its details.
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
/// \brief Sets the certificate action button label to Generate or Import for the current mode.
///
void ConnectionDialog::updateClientCertificateAction()
{
    ui->clientCertificateViewButton->setText(
        ui->clientCertificateComboBox->currentIndex() == 0 ? tr("Generate...") : tr("Import..."));
}

///
/// \brief Opens a read-only certificate details dialog.
/// \param certificate DER-encoded certificate to show.
/// \param certificatePath Path of the source certificate file, or empty when unavailable.
///
void ConnectionDialog::showCertificateDetails(const QByteArray &certificate,
                                              const QString &certificatePath)
{
    if (certificate.isEmpty())
        return;

    CertificateDetailsDialog dialog(this);
    dialog.setCertificate(certificate, certificatePath);
    dialog.exec();
}
