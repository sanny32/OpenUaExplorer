// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file certificatetrustdialog.cpp
/// \brief Implements the server certificate trust decision dialog.
///

#include <QPushButton>

#include "appcolors.h"
#include "certificatetrustdialog.h"
#include "opcua/certificateinfo.h"
#include "ui_certificatetrustdialog.h"

///
/// \brief Builds the dialog and wires its reject/trust-once/trust-permanently buttons.
/// \param parent Parent widget.
///
CertificateTrustDialog::CertificateTrustDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::CertificateTrustDialog)
{
    ui->setupUi(this);

    ui->messageIcon->setIcon(QStringLiteral("alert-circle"), QSize(18, 18));

    ui->headerTitle->setStyleSheet(QStringLiteral("font-size: 19px; font-weight: 700; color: %1;")
        .arg(AppColors::titleText().name()));
    ui->headerSubtitle->setStyleSheet(
        QStringLiteral("color: %1;").arg(AppColors::subtitleText().name()));

    ui->messageFrame->setStyleSheet(QStringLiteral(
        "#messageFrame { background-color: %1; border: 1px solid %2; border-radius: 8px; }")
        .arg(AppColors::errorBadgeBackground().name(), AppColors::errorBadgeBorder().name()));
    ui->messageLabel->setStyleSheet(QStringLiteral(
        "color: %1; font-weight: 600; background: transparent;")
        .arg(AppColors::errorBadgeText().name()));

    applyHeader(true);

    connect(ui->rejectButton, &QPushButton::clicked, this, [this]() {
        _decision = Reject;
        reject();
    });
    connect(ui->trustOnceButton, &QPushButton::clicked, this, [this]() {
        _decision = TrustOnce;
        accept();
    });
    connect(ui->trustPermanentlyButton, &QPushButton::clicked, this, [this]() {
        _decision = TrustPermanently;
        accept();
    });
}

///
/// \brief Destroys the dialog and its generated UI.
///
CertificateTrustDialog::~CertificateTrustDialog()
{
    delete ui;
}

///
/// \brief Shows the certificate's parsed details and validation message.
/// \param certificate DER-encoded certificate.
/// \param message Validation error description.
/// \note Permanent trust is offered only inside the validity window: a certificate that is
///       expired or not yet valid keeps failing validation after it is added to the trust
///       list, so the prompt would just come back on the next connection.
///
void CertificateTrustDialog::setCertificate(const QByteArray &certificate,
                                             const QString &message)
{
    ui->messageLabel->setText(message);
    ui->detailsWidget->setCertificate(certificate);

    const CertificateInfo info = CertificateInfo::fromDer(certificate);
    const bool withinValidity = info.status == CertificateInfo::Status::Valid;
    ui->trustPermanentlyButton->setEnabled(withinValidity);
    ui->trustPermanentlyButton->setToolTip(withinValidity
        ? QString()
        : tr("This certificate is outside its validity period. Trusting it permanently would "
             "not help: it would still fail validation on the next connection."));
}

///
/// \brief Hides the trust buttons and retitles the dialog for a read-only view.
/// \param viewOnly Whether the dialog should only display certificate details.
///
void CertificateTrustDialog::setViewOnly(bool viewOnly)
{
    ui->trustOnceButton->setVisible(!viewOnly);
    ui->trustPermanentlyButton->setVisible(!viewOnly);
    ui->rejectButton->setText(viewOnly ? tr("Close") : tr("Reject"));
    setWindowTitle(viewOnly ? tr("Server Certificate") : tr("Untrusted Server Certificate"));
    applyHeader(!viewOnly);
}

///
/// \brief Configures the header banner for an untrusted prompt or a plain view.
/// \param untrusted True for the warning banner, false for a neutral header.
///
void CertificateTrustDialog::applyHeader(bool untrusted)
{
    ui->headerIcon->setIcon(untrusted ? QStringLiteral("shield-warning")
                                       : QStringLiteral("shield-trusted"),
                            QSize(52, 52));
    ui->headerTitle->setText(untrusted ? tr("Untrusted Server Certificate")
                                       : tr("Server Certificate"));
    ui->headerSubtitle->setText(untrusted
        ? tr("The server presented a certificate that this client does not yet "
             "trust. Review the certificate details below before deciding whether "
             "to trust it.")
        : tr("Details of the server certificate are shown below."));
    ui->messageFrame->setVisible(untrusted);

    const QColor bannerBg = untrusted ? AppColors::noticeWarningBackground()
                                      : AppColors::noticeNeutralBackground();
    const QColor bannerBorder = untrusted ? AppColors::noticeWarningBorder()
                                          : AppColors::noticeNeutralBorder();
    const QColor iconTint = untrusted ? AppColors::noticeWarningIconTint()
                                      : AppColors::noticeNeutralIconTint();
    ui->headerFrame->setStyleSheet(QStringLiteral(
        "#headerFrame { background-color: %1; border: 1px solid %2; border-radius: 12px; }")
        .arg(bannerBg.name(), bannerBorder.name()));
    ui->headerIcon->setStyleSheet(QStringLiteral(
        "background-color: %1; border-radius: 38px; padding: 12px;")
        .arg(AppColors::toCss(iconTint)));
}

///
/// \brief Returns the trust decision the user made.
/// \return Selected decision.
///
CertificateTrustDialog::Decision CertificateTrustDialog::decision() const
{
    return _decision;
}
