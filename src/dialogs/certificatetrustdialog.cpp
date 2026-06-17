// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file certificatetrustdialog.cpp
/// \brief Implements the server certificate trust decision dialog.
///

#include <QCryptographicHash>
#include <QPushButton>

#include "certificatetrustdialog.h"
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
/// \brief Shows the certificate's fingerprint, size, and validation message.
/// \param certificate DER-encoded certificate.
/// \param message Validation error description.
///
void CertificateTrustDialog::setCertificate(const QByteArray &certificate,
                                             const QString &message)
{
    ui->messageLabel->setText(message);
    const QByteArray fingerprint =
        QCryptographicHash::hash(certificate, QCryptographicHash::Sha256).toHex(':').toUpper();
    ui->fingerprintEdit->setText(QString::fromLatin1(fingerprint));
    ui->sizeLabel->setText(tr("%1 bytes").arg(certificate.size()));
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
}

///
/// \brief Returns the trust decision the user made.
/// \return Selected decision.
///
CertificateTrustDialog::Decision CertificateTrustDialog::decision() const
{
    return _decision;
}
