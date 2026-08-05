// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file connectiondialog.cpp
/// \brief Implements the OPC UA connection dialog.
///

#include <algorithm>

#include <QAction>
#include <QComboBox>
#include <QCursor>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QGridLayout>
#include <QLabel>
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
#include "certificatesdialog.h"
#include "connectiondialog.h"
#include "findserversdialog.h"
#include "messageboxdialog.h"
#include "opcua/certificateinfo.h"
#include "opcua/opcuabackend.h"
#include "opcua/recentconnectionstore.h"
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
    , _secretStore(new SecretStore(this))
{
    ui->setupUi(this);

    connect(_secretStore, &SecretStore::readFinished,
            this, &ConnectionDialog::applyStoredPassword);

    setupEndpointHistory();
    setupCertificatePanels();
    setupControls();
    setupConnections();
    applySessionDefaults();
    updateAuthenticationFields();
    restoreConnectionFor(_lastEnteredEndpointUrl);
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
    QStringList defaultEndpoints;
    for (int index = 0; index < ui->discoveryUrlComboBox->count(); ++index)
        defaultEndpoints.append(ui->discoveryUrlComboBox->itemText(index));

    _endpointHistoryStore.seedIfUninitialized(defaultEndpoints);
    const QStringList endpointHistory = _endpointHistoryStore.history();

    ui->discoveryUrlComboBox->setActionEntry(tr("Find Servers..."));
    ui->discoveryUrlComboBox->setHistory(endpointHistory);
    _lastEnteredEndpointUrl = endpointHistory.isEmpty() ? QString() : endpointHistory.constFirst();
    ui->discoveryUrlComboBox->setEditText(_lastEnteredEndpointUrl);
}

///
/// \brief Restores how this server was connected the last time it was used.
///
/// The recent-connection history keeps the authentication method, the user name and the
/// endpoint that was picked, and the password lives in the credential store when the user
/// asked to remember it. Reopening the dialog for a known server therefore starts where the
/// last connection left off instead of at the defaults.
///
/// \param endpointUrl Endpoint URL the dialog currently shows.
///
void ConnectionDialog::restoreConnectionFor(const QString &endpointUrl)
{
    // Reaching the same server again is not a change of server: whatever the user has typed
    // by now stays, and only a different URL starts the panel over.
    const QString wanted = endpointUrl.trimmed();
    if (wanted == _restoredEndpointUrl)
        return;

    _restoredEndpointUrl = wanted;
    _restoredProfileId.clear();
    ui->rememberCheckBox->setChecked(false);
    ui->passwordEdit->clear();
    if (wanted.isEmpty())
        return;

    const QList<ConnectionProfile> recent = RecentConnectionStore().connections();
    const auto match = std::find_if(
        recent.cbegin(), recent.cend(), [&wanted](const ConnectionProfile &profile) {
            return profile.endpointUrl == wanted;
        });
    if (match == recent.cend())
        return;

    // Connecting the same server again continues its profile instead of starting a new one,
    // so the stored password keeps being found and sessions keep a stable identity.
    _presetId = match->id;
    _restoredProfileId = match->id;

    const int authValue = static_cast<int>(match->authentication);
    const int authIndex = ui->authenticationComboBox->findData(authValue);
    ui->authenticationComboBox->setCurrentIndex(
        authIndex >= 0 ? authIndex
                       : qBound(0, authValue, ui->authenticationComboBox->count() - 1));
    ui->usernameEdit->setText(match->username);

    if (match->authentication == ConnectionProfile::Authentication::Certificate) {
        ui->certificateEdit->setText(match->clientCertificateFile);
        ui->privateKeyEdit->setText(match->privateKeyFile);
    }

    // Discovery re-selects the endpoint that was connected last time.
    _pendingSecurityPolicy = match->securityPolicy;
    _pendingSecurityMode = match->securityMode;

    updateAuthenticationFields();
    if (!_restoredProfileId.isEmpty())
        _secretStore->read(_restoredProfileId, SecretStore::Secret::Password);
}

///
/// \brief Fills in the password the credential store held for the restored server.
///
/// The read is asynchronous, so anything the user typed in the meantime wins, and a result
/// for a server that is no longer shown is dropped.
///
/// \param profileId Profile the secret belongs to.
/// \param secret Which secret was read.
/// \param value Stored password, empty when nothing was kept.
/// \param error Read error, if any.
///
void ConnectionDialog::applyStoredPassword(const QString &profileId, SecretStore::Secret secret,
                                           const QString &value, const QString &error)
{
    if (secret != SecretStore::Secret::Password || profileId != _restoredProfileId)
        return;
    if (!error.isEmpty() || value.isEmpty() || !ui->passwordEdit->text().isEmpty())
        return;

    ui->passwordEdit->setText(value);
    ui->rememberCheckBox->setChecked(true);
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
    setupServerTrustSection();

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
        restoreConnectionFor(_lastEnteredEndpointUrl);
    });
    connect(ui->discoveryUrlComboBox, &HistoryComboBox::itemRemoved,
            this, &ConnectionDialog::forgetEndpointUrl);
    connect(ui->discoveryUrlComboBox, &HistoryComboBox::actionTriggered,
            this, &ConnectionDialog::findServers);
    connect(ui->discoveryUrlComboBox->lineEdit(), &QLineEdit::textEdited,
            this, [this](const QString &text) {
        resetDiscovery();
        _lastEnteredEndpointUrl = text;
    });
    connect(ui->discoveryUrlComboBox->lineEdit(), &QLineEdit::editingFinished, this, [this]() {
        saveLastEndpointUrl();
        ui->discoveryUrlComboBox->lineEdit()->setCursorPosition(0);
        restoreConnectionFor(_lastEnteredEndpointUrl);
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
    connect(ui->serverTrustButton, &QPushButton::clicked,
            this, &ConnectionDialog::toggleServerCertificateTrust);
    connect(ui->trustListManageButton, &QPushButton::clicked,
            this, &ConnectionDialog::manageTrustList);
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
    endEndpointDiscovery();
    saveLastEndpointUrl();
    delete ui;
}

///
/// \brief Sets the backend used for discovery and subscribes to its results.
/// \param backend OPC UA backend.
///
void ConnectionDialog::setBackend(OpcUaBackend *backend)
{
    if (_service) {
        endEndpointDiscovery();
        disconnect(_service, nullptr, this, nullptr);
    }
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

    // The caller knows better than the history what this dialog is for, so a restore that is
    // still on its way must not write over it.
    _presetId = profile.id;
    _restoredEndpointUrl = profile.endpointUrl;
    _restoredProfileId.clear();
    resetDiscovery();
    _lastEnteredEndpointUrl = profile.endpointUrl;
    {
        const QSignalBlocker blocker(ui->discoveryUrlComboBox);
        ui->discoveryUrlComboBox->setEditText(profile.endpointUrl);
        ui->discoveryUrlComboBox->lineEdit()->setCursorPosition(0);
    }

    // Until an endpoint is discovered the combo box holds the designer items, which carry no
    // user data; there the authentication mode is the item position.
    const int authValue = static_cast<int>(profile.authentication);
    const int authIndex = ui->authenticationComboBox->findData(authValue);
    ui->authenticationComboBox->setCurrentIndex(
        authIndex >= 0
            ? authIndex
            : qBound(0, authValue, ui->authenticationComboBox->count() - 1));
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
/// \brief Reports whether the entered password should be kept in the credential store.
/// \return True when the user asked to be remembered.
///
bool ConnectionDialog::rememberCredentials() const
{
    return ui->rememberCheckBox->isEnabled() && ui->rememberCheckBox->isChecked();
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
    _pendingSecurityPolicy.clear();
    _pendingSecurityMode = -1;
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
    beginEndpointDiscovery();
    _service->discoverEndpoints(url, settings.backend, settings.endpointTimeoutMs);
}

///
/// \brief Browses a discovery server and adopts the endpoint the user picks there.
///
/// The child dialog runs GetEndpoints on the same backend, so this dialog stops
/// listening for discovery results while it is open; otherwise every branch the user
/// expands would overwrite the endpoint table here.
///
void ConnectionDialog::findServers()
{
    if (!_service) {
        MessageBoxDialog::critical(this, tr("OPC UA Unavailable"),
                                   tr("The OPC UA backend is unavailable."));
        return;
    }

    const ConnectionProfile settings = profile();
    disconnect(_service, &OpcUaBackend::endpointsDiscovered,
               this, &ConnectionDialog::handleEndpoints);

    FindServersDialog dialog(this);
    dialog.setBackend(_service);
    dialog.setRequestDefaults(settings.backend, settings.endpointTimeoutMs);
    const int result = dialog.exec();

    connect(_service, &OpcUaBackend::endpointsDiscovered,
            this, &ConnectionDialog::handleEndpoints);

    if (result != QDialog::Accepted)
        return;

    const EndpointInfo endpoint = dialog.selectedEndpoint();
    {
        const QSignalBlocker blocker(ui->discoveryUrlComboBox);
        ui->discoveryUrlComboBox->setEditText(endpoint.endpointUrl);
    }
    _lastEnteredEndpointUrl = endpoint.endpointUrl;
    discoverEndpoints();

    // discoverEndpoints() resets the pending selection, so record it afterwards.
    _pendingSecurityPolicy = endpoint.securityPolicy;
    _pendingSecurityMode = endpoint.securityModeValue;
}

///
/// \brief Shows the discovery result and, when queued, continues to connect.
/// \param endpoints Discovered endpoints.
/// \param error Discovery error.
///
void ConnectionDialog::handleEndpoints(QList<EndpointInfo> endpoints, const QString &error)
{
    endEndpointDiscovery();
    ui->getEndpointsButton->setEnabled(true);
    ui->connectButton->setEnabled(true);
    if (!error.isEmpty()) {
        _connectAfterDiscovery = false;
        _pendingSecurityPolicy.clear();
        _pendingSecurityMode = -1;
        ui->statusLabel->setText(error);
        return;
    }
    ui->endpointsWidget->setEndpoints(endpoints);
    if (_pendingSecurityMode >= 0) {
        ui->endpointsWidget->selectEndpoint(_pendingSecurityPolicy, _pendingSecurityMode);
        _pendingSecurityPolicy.clear();
        _pendingSecurityMode = -1;
    }
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
/// \brief Shows the application wait cursor during endpoint discovery.
///
void ConnectionDialog::beginEndpointDiscovery()
{
    if (_endpointDiscoveryCursorActive)
        return;
    QGuiApplication::setOverrideCursor(Qt::WaitCursor);
    _endpointDiscoveryCursorActive = true;
}

///
/// \brief Removes the wait cursor installed for endpoint discovery.
///
void ConnectionDialog::endEndpointDiscovery()
{
    if (!_endpointDiscoveryCursorActive)
        return;
    QGuiApplication::restoreOverrideCursor();
    _endpointDiscoveryCursorActive = false;
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
    // The list is rebuilt from the tokens this endpoint advertises, so the choice is carried
    // over by what it means, not by where it sat: the offered methods differ per endpoint.
    const int previousAuthentication = currentAuthentication();
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
    const int carriedOver = ui->authenticationComboBox->findData(previousAuthentication);
    ui->authenticationComboBox->setCurrentIndex(qMax(0, carriedOver));
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
    // Only a user password can be stored: the backend refuses encrypted private keys, so a
    // key password would be useless on the next connection anyway.
    ui->rememberCheckBox->setEnabled(username);

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
    _secureChannel = secureChannel;
    updateServerTrustState();
}

///
/// \brief Styles the trust section and aligns it with the certificate card's caption column.
///
/// The section lives outside CertificateSummaryWidget, so its caption only lines up with the
/// Subject/Issuer/Valid rows when it borrows the widest caption's width.
///
void ConnectionDialog::setupServerTrustSection()
{
    ui->serverTrustIcon->setIcon(QStringLiteral("check-circle"), QSize(18, 18));
    ui->serverTrustCaption->setStyleSheet(
        QStringLiteral("color: %1;").arg(AppColors::caption().name()));

    int captionColumn = 0;
    const QList<QLabel *> captions = ui->serverCertificateWidget->findChildren<QLabel *>();
    for (const QLabel *caption : captions) {
        if (caption->property("certCaption").toBool())
            captionColumn = qMax(captionColumn, caption->sizeHint().width());
    }
    ui->serverTrustLayout->setColumnMinimumWidth(0, captionColumn);

    updateServerTrustState();
}

///
/// \brief Returns the server certificate of the selected endpoint.
/// \return DER-encoded server certificate, or an empty array when nothing is selected.
///
QByteArray ConnectionDialog::selectedServerCertificate() const
{
    return ui->endpointsWidget->hasSelection()
        ? ui->endpointsWidget->currentEndpoint().serverCertificate
        : QByteArray();
}

///
/// \brief Reports whether a certificate is stored in the trust list.
/// \param certificate DER-encoded certificate.
/// \return True when the trust list holds this certificate.
///
bool ConnectionDialog::isTrusted(const QByteArray &certificate) const
{
    if (certificate.isEmpty())
        return false;

    PkiManager pki;
    const QString wanted = PkiManager::fingerprint(certificate);
    const QList<QByteArray> trusted = pki.certificates(PkiManager::Category::Trusted);
    return std::any_of(trusted.cbegin(), trusted.cend(), [&wanted](const QByteArray &stored) {
        return PkiManager::fingerprint(stored) == wanted;
    });
}

///
/// \brief Shows whether the selected server certificate is in the trust list, and offers the
///        matching action.
/// \note The whole section hides without a certificate to reason about: an unsecured channel
///       never validates one, so neither the state nor the policy applies.
///
void ConnectionDialog::updateServerTrustState()
{
    const QByteArray certificate = selectedServerCertificate();
    const bool applicable = _secureChannel && !certificate.isEmpty();
    const bool trusted = isTrusted(certificate);

    ui->serverTrustLine->setVisible(applicable);
    ui->serverTrustSection->setVisible(applicable);
    if (!applicable)
        return;

    ui->serverTrustIcon->setVisible(trusted);
    ui->serverTrustStatusLabel->setText(trusted ? tr("In trust list") : tr("Not in trust list"));
    ui->serverTrustStatusLabel->setStyleSheet(
        QStringLiteral("color: %1; font-weight: 600;")
            .arg(trusted ? AppColors::statusSuccess().name() : AppColors::statusWarning().name()));
    ui->serverTrustButton->setText(trusted ? tr("Remove from trust list") : tr("Trust"));

    // A certificate outside its validity period keeps failing validation, so trusting it would
    // change nothing. Removing one stays available, as the trust list still needs cleaning up.
    const bool withinValidity =
        CertificateInfo::fromDer(certificate).status == CertificateInfo::Status::Valid;
    ui->serverTrustButton->setEnabled(trusted || withinValidity);
    ui->serverTrustButton->setToolTip(
        trusted || withinValidity
            ? QString()
            : tr("The server certificate is outside its validity period. Trusting it would not "
                 "help: it would still fail validation on every connection."));
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
/// \brief Adds the selected server certificate to the trust list, or removes it from there.
///
/// Both directions change what the client accepts on every future connection, so each is
/// confirmed first. Removal withdraws trust the user granted earlier, so it warns rather
/// than merely asks.
///
void ConnectionDialog::toggleServerCertificateTrust()
{
    const QByteArray certificate = selectedServerCertificate();
    if (certificate.isEmpty())
        return;

    const bool trusted = isTrusted(certificate);
    const QString subject = CertificateInfo::fromDer(certificate).subject;
    const DialogButtonBox::StandardButton answer = trusted
        ? MessageBoxDialog::warning(
              this, tr("Remove from Trust List"),
              tr("Remove the certificate of \"%1\" from the trust list?").arg(subject),
              DialogButtonBox::Yes | DialogButtonBox::No, DialogButtonBox::No)
        : MessageBoxDialog::question(
              this, tr("Trust Server Certificate"),
              tr("Add the certificate of \"%1\" to the trust list?").arg(subject),
              DialogButtonBox::Yes | DialogButtonBox::No, DialogButtonBox::No);
    if (answer != DialogButtonBox::Yes)
        return;

    PkiManager pki;
    QString error;
    const bool changed = trusted ? pki.removeCertificate(certificate, &error)
                                 : pki.trustServerCertificate(certificate, &error);
    if (!changed) {
        MessageBoxDialog::critical(this, tr("Trust List Failed"), error);
        return;
    }
    updateServerTrustState();
}

///
/// \brief Opens the trust store manager and picks up any change it made to the trust list.
///
void ConnectionDialog::manageTrustList()
{
    CertificatesDialog dialog(this);
    dialog.exec();
    updateServerTrustState();
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
    ui->discoveryUrlComboBox->setHistory(endpointHistory);
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
    resetDiscovery();
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
