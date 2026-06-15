// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file connectiondialog.cpp
/// \brief Implements the OPC UA connection dialog.
///

#include <QAbstractItemView>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontMetrics>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QLineEdit>
#include <QSslCertificate>
#include <QListView>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QScreen>
#include <QSignalBlocker>
#include <QStringList>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QUuid>

#include "appicons.h"
#include "certificatetrustdialog.h"
#include "connectiondialog.h"
#include "opcua/certificateinfo.h"
#include "opcua/opcuaclientservice.h"
#include "opcua/connectionprofilevalidator.h"
#include "opcua/pkimanager.h"
#include "ui_connectiondialog.h"
#include "widgets/coloredpushbutton.h"
#include "widgets/endpointmodel.h"
#include "widgets/themediconlabel.h"

namespace {

///
/// \brief Delegate that renders endpoint security details in one compact row.
///
class EndpointItemDelegate final : public QStyledItemDelegate
{
public:
    explicit EndpointItemDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;
};

///
/// \brief EndpointItemDelegate::EndpointItemDelegate
/// \param parent Delegate owner.
///
EndpointItemDelegate::EndpointItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

///
/// \brief EndpointItemDelegate::paint
/// \param painter Item painter.
/// \param option Item style options.
/// \param index Model index being rendered.
///
void EndpointItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    QStyleOptionViewItem backgroundOption(option);
    initStyleOption(&backgroundOption, index);
    backgroundOption.text.clear();
    backgroundOption.icon = QIcon();

    const QStyle *style = option.widget ? option.widget->style() : QApplication::style();
    style->drawControl(QStyle::CE_ItemViewItem, &backgroundOption, painter, option.widget);

    const bool selected = option.state.testFlag(QStyle::State_Selected);
    const QColor primaryColor = option.palette.color(
        selected ? QPalette::HighlightedText : QPalette::Text);
    const QColor secondaryColor = option.palette.color(
        selected ? QPalette::HighlightedText : QPalette::PlaceholderText);

    const QRect contentRect = option.rect.adjusted(8, 3, -8, -3);
    const int iconSize = 20;
    const QRect iconRect(contentRect.left(),
                         contentRect.center().y() - iconSize / 2,
                         iconSize, iconSize);
    QPixmap iconPixmap = AppIcons::themed(index.data(EndpointModel::IconRole).toString())
                             .pixmap(QSize(iconSize, iconSize));
    if (selected) {
        QPainter iconPainter(&iconPixmap);
        iconPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        iconPainter.fillRect(iconPixmap.rect(), primaryColor);
    }
    painter->drawPixmap(iconRect, iconPixmap);

    const QRect textRect = contentRect.adjusted(iconSize + 8, 0, 0, 0);
    QFont primaryFont = option.font;
    primaryFont.setWeight(QFont::DemiBold);
    const QFontMetrics primaryMetrics(primaryFont);
    const QFontMetrics secondaryMetrics(option.font);
    const QString mode = index.data(EndpointModel::ModeRole).toString();
    const int modeWidth = secondaryMetrics.horizontalAdvance(mode);
    const int textGap = 16;
    const int policyWidth = qMax(0, textRect.width() - modeWidth - textGap);

    painter->save();
    painter->setPen(primaryColor);
    painter->setFont(primaryFont);
    painter->drawText(
        QRect(textRect.left(), textRect.top(), policyWidth, textRect.height()),
        Qt::AlignLeft | Qt::AlignVCenter,
        primaryMetrics.elidedText(index.data(EndpointModel::PolicyRole).toString(),
                                  Qt::ElideRight, policyWidth));

    painter->setPen(secondaryColor);
    painter->setFont(option.font);
    painter->drawText(
        QRect(textRect.right() - modeWidth, textRect.top(), modeWidth, textRect.height()),
        Qt::AlignRight | Qt::AlignVCenter, mode);
    painter->restore();
}

///
/// \brief EndpointItemDelegate::sizeHint
/// \param option Item style options.
/// \param index Model index.
/// \return Preferred endpoint row size.
///
QSize EndpointItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                     const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(qMax(size.height(), 32));
    return size;
}

///
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
    , _endpointModel(new EndpointModel(this))
{
    ui->setupUi(this);

    QStringList endpointHistory = _endpointHistoryStore.history();
    if (endpointHistory.isEmpty())
        endpointHistory.append(ui->discoveryUrlComboBox->currentText());

    ui->discoveryUrlComboBox->clear();
    ui->discoveryUrlComboBox->addItems(endpointHistory);
    _lastEnteredEndpointUrl = endpointHistory.constFirst();
    ui->discoveryUrlComboBox->setEditText(_lastEnteredEndpointUrl);

    const QString certDetailsStyle = QStringLiteral(
        "QLineEdit { background: transparent; border: none; padding: 0; }"
        "QLabel[certCaption=\"true\"] { color: #6b7280; }");
    ui->serverCertValidIcon->setIcon(QStringLiteral("check-circle"), QSize(18, 18));
    ui->clientCertValidIcon->setIcon(QStringLiteral("check-circle"), QSize(18, 18));
    ui->clientCertificateHeaderIcon->setIcon(QStringLiteral("certificate"), QSize(18, 18));
    ui->serverCertificateHeaderIcon->setIcon(QStringLiteral("certificate"), QSize(18, 18));
    const bool darkTheme = AppIcons::isDarkTheme();
    const QString certHeaderStyle = QStringLiteral(
        "QLabel { color: %1; font-weight: 600; }"
        "QLabel:disabled { color: %2; }")
        .arg(darkTheme ? QStringLiteral("#60a5fa") : QStringLiteral("#2563eb"),
             darkTheme ? QStringLiteral("#5b626b") : QStringLiteral("#9aa0a6"));
    ui->clientCertificateHeader->setStyleSheet(certHeaderStyle);
    ui->serverCertificateHeader->setStyleSheet(certHeaderStyle);
    ui->serverCertificateDetails->setStyleSheet(certDetailsStyle);
    ui->clientCertificateDetails->setStyleSheet(certDetailsStyle);
    ui->serverCertFingerprintEdit->installEventFilter(this);
    ui->clientCertFingerprintEdit->installEventFilter(this);
    ui->statusIconLabel->setIcon(QStringLiteral("disconnected"), QSize(16, 16));
    ui->connectButton->setColors({ QColor(0x0a74d1), QColor(0x1682df), QColor(0x075ca7) });
    ui->endpointListWidget->setModel(_endpointModel);
    ui->endpointListWidget->setItemDelegate(new EndpointItemDelegate(ui->endpointListWidget));

    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui->connectButton, &QPushButton::clicked,
            this, &ConnectionDialog::validateAndAccept);
    connect(ui->getEndpointsButton, &QPushButton::clicked,
            this, &ConnectionDialog::discoverEndpoints);
    connect(ui->endpointListWidget->selectionModel(),
            &QItemSelectionModel::currentChanged,
            this, &ConnectionDialog::updateEndpointSelection);
    connect(ui->discoveryUrlComboBox, QOverload<int>::of(&QComboBox::activated),
            this, [this](int index) {
        _endpointModel->clear();
        updateServerCertificate({});
        _lastEnteredEndpointUrl = ui->discoveryUrlComboBox->itemText(index);
    });
    connect(ui->discoveryUrlComboBox->lineEdit(), &QLineEdit::textEdited,
            this, [this](const QString &text) {
        _endpointModel->clear();
        updateServerCertificate({});
        _lastEnteredEndpointUrl = text;
    });
    connect(ui->discoveryUrlComboBox->lineEdit(), &QLineEdit::editingFinished, this, [this]() {
        saveLastEndpointUrl();
        ui->discoveryUrlComboBox->lineEdit()->setCursorPosition(0);
    });
    connect(ui->authenticationComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ConnectionDialog::updateAuthenticationFields);
    connect(ui->clientCertificateViewButton, &QPushButton::clicked,
            this, &ConnectionDialog::chooseClientCertificate);
    connect(ui->serverCertificateViewButton, &QPushButton::clicked,
            this, &ConnectionDialog::viewServerCertificate);
    connect(ui->clientCertViewButton, &QPushButton::clicked,
            this, &ConnectionDialog::viewClientCertificate);
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
    PkiManager pki;
    if (pki.existingClientCertificate(&_clientCertificateFile, &_privateKeyFile)) {
        ui->clientCertificateComboBox->setItemText(
            0, tr("Auto-generated (%1)").arg(QFileInfo(_clientCertificateFile).fileName()));
    }
    updateServerCertificate({});
    updateClientCertificate();
    updatePopupWidth(ui->discoveryUrlComboBox);
    updateAuthenticationFields();
    adjustSize();
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
    const int endpointIndex = ui->endpointListWidget->currentIndex().row();
    if (endpointIndex >= 0 && endpointIndex < _endpointModel->rowCount()) {
        const EndpointInfo endpoint = _endpointModel->endpointAt(endpointIndex);
        result.name = endpoint.endpointUrl;
        result.endpointUrl = endpoint.endpointUrl;
        result.securityPolicy = endpoint.securityPolicy;
        result.securityMode = endpoint.securityModeValue;
    } else {
        result.name = ui->discoveryUrlComboBox->currentText();
        result.endpointUrl = result.name;
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
        _connectAfterDiscovery = false;
        QMessageBox::critical(this, tr("OPC UA Unavailable"),
                              tr("The OPC UA client service is unavailable."));
        return;
    }
    saveLastEndpointUrl();
    const QString url = ui->discoveryUrlComboBox->currentText();
    _endpointModel->clear();
    updateServerCertificate({});
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
    _endpointModel->setEndpoints(endpoints);
    if (endpoints.isEmpty()) {
        _connectAfterDiscovery = false;
        ui->statusLabel->setText(tr("No endpoints discovered."));
        return;
    }
    ui->statusLabel->setText(tr("%1 endpoints discovered.").arg(endpoints.size()));
    ui->endpointListWidget->setCurrentIndex(_endpointModel->index(0, 0));
    updateEndpointSelection();
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
    const int endpointIndex = ui->endpointListWidget->currentIndex().row();
    if (endpointIndex < 0 || endpointIndex >= _endpointModel->rowCount())
        return;
    const EndpointInfo endpoint = _endpointModel->endpointAt(endpointIndex);
    updateServerCertificate(endpoint.serverCertificate);
    ui->securityModeComboBox->setCurrentIndex(qMax(0, endpoint.securityModeValue - 1));
    int policyIndex = ui->securityPolicyComboBox->findText(
        endpoint.securityPolicy.section(QLatin1Char('#'), -1));
    if (policyIndex < 0) {
        ui->securityPolicyComboBox->addItem(
            endpoint.securityPolicy.section(QLatin1Char('#'), -1));
        policyIndex = ui->securityPolicyComboBox->count() - 1;
    }
    ui->securityPolicyComboBox->setCurrentIndex(policyIndex);
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

    // The client certificate stays available regardless of the security mode
    // (it can still be imported, generated or inspected). Only the server
    // certificate and trust settings depend on a secure channel.
    const bool secureChannel = certificate || ui->securityModeComboBox->currentIndex() > 0;
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
            QCoreApplication::applicationName(),
            PkiManager::applicationUri(),
            &_clientCertificateFile, &_privateKeyFile, &error)) {
        QMessageBox::critical(this, tr("Certificate Generation Failed"), error);
        return;
    }
    _privateKeyPassword.clear();
    ui->clientCertificateComboBox->setCurrentIndex(0);
    updateClientCertificate();
}

///
/// \brief ConnectionDialog::viewServerCertificate
///
void ConnectionDialog::viewServerCertificate()
{
    if (_serverCertificate.isEmpty())
        return;

    CertificateTrustDialog dialog(this);
    dialog.setViewOnly(true);
    dialog.setCertificate(
        _serverCertificate,
        tr("Certificate advertised by the selected OPC UA endpoint."));
    dialog.exec();
}

///
/// \brief ConnectionDialog::validateAndAccept
///
void ConnectionDialog::validateAndAccept()
{
    const int endpointIndex = ui->endpointListWidget->currentIndex().row();
    if (_endpointModel->rowCount() == 0
        || endpointIndex < 0 || endpointIndex >= _endpointModel->rowCount()) {
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
/// \brief ConnectionDialog::updateServerCertificate
/// \param certificate DER-encoded certificate advertised by the selected endpoint.
///
void ConnectionDialog::updateServerCertificate(const QByteArray &certificate)
{
    _serverCertificate = certificate;
    const bool available = !_serverCertificate.isEmpty();
    ui->serverCertificateViewButton->setEnabled(available);
    ui->serverCertificateDetails->setVisible(available);
    ui->serverCertificateHintLabel->setVisible(!available);
    ui->serverCertificateHeader->setEnabled(available);
    ui->serverCertificateHeaderIcon->setEnabled(available);

    if (!available) {
        ui->serverCertificateHintLabel->setText(
            tr("Select an endpoint that provides a server certificate."));
        return;
    }

    fillCertificateFields(_serverCertificate, ui->serverCertSubjectEdit,
                          ui->serverCertIssuerEdit, ui->serverCertValidEdit,
                          ui->serverCertFingerprintEdit, ui->serverCertValidIcon,
                          ui->serverCertValidBadge, &_serverCertFingerprint);
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
    _clientCertificate = chain.isEmpty() ? QByteArray() : chain.constFirst().toDer();

    const bool available = !_clientCertificate.isEmpty();
    ui->clientCertViewButton->setEnabled(available);
    if (!available) {
        ui->clientCertSubjectEdit->setText(tr("No client certificate"));
        ui->clientCertIssuerEdit->clear();
        ui->clientCertValidEdit->clear();
        ui->clientCertFingerprintEdit->clear();
        ui->clientCertValidIcon->setVisible(false);
        ui->clientCertValidBadge->clear();
        return;
    }

    fillCertificateFields(_clientCertificate, ui->clientCertSubjectEdit,
                          ui->clientCertIssuerEdit, ui->clientCertValidEdit,
                          ui->clientCertFingerprintEdit, ui->clientCertValidIcon,
                          ui->clientCertValidBadge, &_clientCertFingerprint);
}

///
/// \brief ConnectionDialog::fillCertificateFields
/// \param der DER-encoded certificate to describe.
/// \param subjectEdit Subject field.
/// \param issuerEdit Issuer field.
/// \param validEdit Expiry field.
/// \param fingerprintLabel SHA-256 fingerprint label.
/// \param validIcon Validity badge icon.
/// \param validBadge Validity badge text.
/// \param fingerprintStore Receives the full fingerprint for elision.
///
void ConnectionDialog::fillCertificateFields(
    const QByteArray &der, QLabel *subjectEdit, QLabel *issuerEdit,
    QLabel *validEdit, QLabel *fingerprintLabel, ThemedIconLabel *validIcon,
    QLabel *validBadge, QString *fingerprintStore)
{
    const CertificateInfo info = CertificateInfo::fromDer(der);
    *fingerprintStore = info.fingerprint;
    fingerprintLabel->setToolTip(tr("SHA-256: %1").arg(*fingerprintStore));
    elideFingerprint(fingerprintLabel, *fingerprintStore);

    if (!info.readable) {
        subjectEdit->setText(tr("Unable to read certificate"));
        issuerEdit->clear();
        validEdit->setText(tr("%1 bytes").arg(der.size()));
        validIcon->setVisible(false);
        validBadge->clear();
        return;
    }

    subjectEdit->setText(info.subject);
    QString issuer = info.issuer;
    if (info.selfSigned && !issuer.isEmpty())
        issuer = tr("%1 (self-signed)").arg(issuer);
    issuerEdit->setText(issuer);
    validEdit->setText(
        info.expiryDate.toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));

    QString badgeText;
    QColor color;
    bool valid = false;
    if (info.status == CertificateInfo::Status::NotYetValid) {
        badgeText = tr("Not yet valid");
        color = QColor(0xc0, 0x7d, 0x00);
    } else if (info.status == CertificateInfo::Status::Expired) {
        badgeText = tr("Expired");
        color = QColor(0xd1, 0x34, 0x38);
    } else if (info.status == CertificateInfo::Status::Valid) {
        badgeText = tr("Valid");
        color = QColor(0x2e, 0x9e, 0x44);
        valid = true;
    } else {
        badgeText = tr("Invalid");
        color = QColor(0xd1, 0x34, 0x38);
    }

    validIcon->setVisible(valid);
    validBadge->setText(badgeText);
    validBadge->setStyleSheet(
        QStringLiteral("color: %1; font-weight: 600;").arg(color.name()));
}

///
/// \brief ConnectionDialog::viewClientCertificate
///
void ConnectionDialog::viewClientCertificate()
{
    if (_clientCertificate.isEmpty())
        return;

    CertificateTrustDialog dialog(this);
    dialog.setViewOnly(true);
    dialog.setCertificate(_clientCertificate,
                          tr("Certificate presented by this application to the server."));
    dialog.exec();
}

///
/// \brief ConnectionDialog::elideFingerprint
/// \param label Fingerprint label to update.
/// \param fingerprint Full SHA-256 fingerprint (colon-separated bytes).
///
/// Shows the fingerprint truncated on a byte boundary with an ellipsis so it is
/// never clipped in the middle of a hex pair.
///
void ConnectionDialog::elideFingerprint(QLabel *label, const QString &fingerprint)
{
    if (fingerprint.isEmpty()) {
        label->clear();
        return;
    }

    const QFontMetrics metrics(label->font());
    const int available = label->contentsRect().width();
    if (available <= 0 || metrics.horizontalAdvance(fingerprint) <= available) {
        label->setText(fingerprint);
        return;
    }

    const QStringList bytes = fingerprint.split(QLatin1Char(':'));
    const QString ellipsis = QStringLiteral("…");
    QString shown;
    for (const QString &part : bytes) {
        const QString candidate = shown.isEmpty()
            ? part : shown + QLatin1Char(':') + part;
        if (!shown.isEmpty()
            && metrics.horizontalAdvance(candidate + ellipsis) > available) {
            break;
        }
        shown = candidate;
    }
    label->setText(shown.isEmpty() ? ellipsis : shown + ellipsis);
}

///
/// \brief ConnectionDialog::eventFilter
/// \param watched Watched object.
/// \param event Event being delivered.
/// \return True if the event was handled.
///
bool ConnectionDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::Resize) {
        if (watched == ui->serverCertFingerprintEdit)
            elideFingerprint(ui->serverCertFingerprintEdit, _serverCertFingerprint);
        else if (watched == ui->clientCertFingerprintEdit)
            elideFingerprint(ui->clientCertFingerprintEdit, _clientCertFingerprint);
    }
    return AppBaseDialog::eventFilter(watched, event);
}
