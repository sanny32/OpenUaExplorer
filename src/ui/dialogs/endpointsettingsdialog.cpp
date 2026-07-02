// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file endpointsettingsdialog.cpp
/// \brief Implements the read-only endpoint settings inspector dialog.
///

#include <QLabel>
#include <QList>
#include <QLocale>
#include <QPushButton>

#include "appcolors.h"
#include "certificatedetailsdialog.h"
#include "formatters/durationformatter.h"
#include "endpointsettingsdialog.h"
#include "opcua/certificateinfo.h"
#include "ui_endpointsettingsdialog.h"

namespace {

/// \brief Returns the short security-policy name (the part after '#'), or "None".
QString securityPolicyName(const QString &policy)
{
    const QString name = policy.section(QLatin1Char('#'), -1);
    return name.isEmpty() ? QStringLiteral("None") : name;
}

/// \brief Returns display text for a certificate validity status.
QString certificateStatusText(CertificateInfo::Status status)
{
    switch (status) {
    case CertificateInfo::Status::NotYetValid:
        return EndpointSettingsDialog::tr("Not yet valid");
    case CertificateInfo::Status::Expired:
        return EndpointSettingsDialog::tr("Expired");
    case CertificateInfo::Status::Valid:
        return EndpointSettingsDialog::tr("Valid");
    case CertificateInfo::Status::Invalid:
        break;
    }
    return EndpointSettingsDialog::tr("Invalid");
}

/// \brief Formats milliseconds with a compact duration hint.
QString formatMilliseconds(int value)
{
    const QLocale locale;
    return EndpointSettingsDialog::tr("%1 ms (%2)")
        .arg(locale.toString(value), formatDuration(value));
}

} // namespace

///
/// \brief Builds the dialog from its generated UI and themed styling.
/// \param parent Parent widget.
///
EndpointSettingsDialog::EndpointSettingsDialog(QWidget *parent)
    : AppBaseDialog(parent)
    , ui(new Ui::EndpointSettingsDialog)
{
    ui->setupUi(this);
    ui->closeButton->setColors(
        { AppColors::accent(), AppColors::accentHover(), AppColors::accentPressed() });
    ui->certificateStatusIcon->setIcon(QStringLiteral("shield-trusted"), QSize(18, 18));
    updateServerCertificateFields();
    applyStyling();

    connect(ui->closeButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(ui->viewCertificateButton, &QPushButton::clicked,
            this, &EndpointSettingsDialog::viewServerCertificateDetails);
}

///
/// \brief Destroys the dialog and its generated UI.
///
EndpointSettingsDialog::~EndpointSettingsDialog()
{
    delete ui;
}

///
/// \brief Applies the theme-aware label colours that cannot be expressed in the .ui file.
///
void EndpointSettingsDialog::applyStyling()
{
    ui->certificateLayout->setColumnMinimumWidth(
        0, ui->securityModeCaption->sizeHint().width());

    const QString captionStyle = QStringLiteral("color: %1;").arg(AppColors::fieldLabel().name());
    const QList<QLabel *> captions = {
        ui->serverUrlCaption, ui->securityPolicyCaption, ui->securityModeCaption,
        ui->authenticationCaption, ui->sessionTimeoutCaption,
        ui->connectTimeoutCaption, ui->requestTimeoutCaption,
        ui->secureChannelLifetimeCaption, ui->maxMessageSizeCaption,
        ui->certificateSubjectCaption, ui->certificateIssuerCaption,
        ui->certificateValidUntilCaption, ui->certificateStatusCaption,
        ui->certificateSerialNumberCaption
    };
    for (QLabel *caption : captions)
        caption->setStyleSheet(captionStyle);
}

///
/// \brief Fills the inspector with the values of the active connection profile.
/// \param profile Profile describing the live connection.
///
void EndpointSettingsDialog::setProfile(const ConnectionProfile &profile)
{
    const QLocale locale;
    ui->serverUrlValue->setText(profile.endpointUrl);
    ui->securityPolicyValue->setText(securityPolicyName(profile.securityPolicy));

    QString mode;
    switch (profile.securityMode) {
    case 2:  mode = tr("Sign"); break;
    case 3:  mode = tr("Sign & Encrypt"); break;
    default: mode = tr("None"); break;
    }
    ui->securityModeValue->setText(mode);

    QString authentication;
    switch (profile.authentication) {
    case ConnectionProfile::Authentication::Username:
        authentication = profile.username.isEmpty()
            ? tr("Username / Password")
            : tr("Username / Password (%1)").arg(profile.username);
        break;
    case ConnectionProfile::Authentication::Certificate:
        authentication = tr("Certificate");
        break;
    case ConnectionProfile::Authentication::Anonymous:
        authentication = tr("Anonymous");
        break;
    }
    ui->authenticationValue->setText(authentication);

    ui->sessionTimeoutValue->setText(formatMilliseconds(profile.sessionTimeoutMs));
    ui->connectTimeoutValue->setText(formatMilliseconds(profile.connectTimeoutMs));
    ui->requestTimeoutValue->setText(formatMilliseconds(profile.requestTimeoutMs));
    ui->secureChannelLifetimeValue->setText(formatMilliseconds(profile.secureChannelLifetimeMs));

    QString messageSize = tr("%1 bytes").arg(locale.toString(profile.maxMessageSizeBytes));
    if (profile.maxMessageSizeBytes >= 1024 * 1024) {
        const double mib = profile.maxMessageSizeBytes / (1024.0 * 1024.0);
        messageSize = tr("%1 bytes (%2 MiB)")
            .arg(locale.toString(profile.maxMessageSizeBytes), locale.toString(mib, 'f', 1));
    } else if (profile.maxMessageSizeBytes >= 1024) {
        const double kib = profile.maxMessageSizeBytes / 1024.0;
        messageSize = tr("%1 bytes (%2 KiB)")
            .arg(locale.toString(profile.maxMessageSizeBytes), locale.toString(kib, 'f', 1));
    }
    ui->maxMessageSizeValue->setText(messageSize);
}

///
/// \brief Shows the endpoint's server certificate, enabling its details link.
/// \param der DER-encoded server certificate, or an empty array when none is in use.
///
void EndpointSettingsDialog::setServerCertificate(const QByteArray &der)
{
    _serverCertificate = der;
    updateServerCertificateFields();
}

///
/// \brief Updates the certificate status row text, icon, and colour.
/// \param status Certificate validity status.
///
void EndpointSettingsDialog::setCertificateStatus(CertificateInfo::Status status)
{
    const bool valid = status == CertificateInfo::Status::Valid;
    ui->certificateStatusIcon->setVisible(valid);
    ui->certificateStatusValue->setText(certificateStatusText(status));
    ui->certificateStatusValue->setStyleSheet(
        QStringLiteral("color: %1; font-weight: 600;")
            .arg(valid ? AppColors::statusSuccess().name() : AppColors::statusError().name()));
}

///
/// \brief Updates the server certificate rows from the current DER payload.
///
void EndpointSettingsDialog::updateServerCertificateFields()
{
    ui->viewCertificateButton->setEnabled(!_serverCertificate.isEmpty());

    if (_serverCertificate.isEmpty()) {
        ui->certificateSubjectValue->setText(
            tr("The active endpoint does not use a server certificate."));
        ui->certificateIssuerValue->clear();
        ui->certificateValidUntilValue->clear();
        ui->certificateStatusIcon->setVisible(false);
        ui->certificateStatusValue->clear();
        ui->certificateStatusValue->setStyleSheet(QString());
        ui->certificateSerialNumberValue->clear();
        return;
    }

    const CertificateInfo info = CertificateInfo::fromDer(_serverCertificate);
    if (!info.readable) {
        ui->certificateSubjectValue->setText(tr("Unable to read certificate"));
        ui->certificateIssuerValue->clear();
        ui->certificateValidUntilValue->setText(tr("%1 bytes").arg(_serverCertificate.size()));
        setCertificateStatus(info.status);
        ui->certificateSerialNumberValue->setText(tr("Unavailable"));
        return;
    }

    ui->certificateSubjectValue->setText(info.subject);
    ui->certificateIssuerValue->setText(
        info.selfSigned && !info.issuer.isEmpty()
            ? tr("%1 (self-signed)").arg(info.issuer)
            : info.issuer);
    ui->certificateValidUntilValue->setText(
        info.expiryDate.toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
    setCertificateStatus(info.status);
    ui->certificateSerialNumberValue->setText(
        info.serialNumber.isEmpty() ? tr("Unavailable") : info.serialNumber);
}

///
/// \brief Opens a read-only details dialog for the endpoint's server certificate.
///
void EndpointSettingsDialog::viewServerCertificateDetails()
{
    if (_serverCertificate.isEmpty())
        return;

    CertificateDetailsDialog dialog(this);
    dialog.setCertificate(_serverCertificate);
    dialog.exec();
}
