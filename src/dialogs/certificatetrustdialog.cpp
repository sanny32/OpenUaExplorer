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
/// \brief CertificateTrustDialog::CertificateTrustDialog
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
/// \brief CertificateTrustDialog::~CertificateTrustDialog
///
CertificateTrustDialog::~CertificateTrustDialog()
{
    delete ui;
}

///
/// \brief CertificateTrustDialog::setCertificate
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
/// \brief CertificateTrustDialog::decision
/// \return Selected decision.
///
CertificateTrustDialog::Decision CertificateTrustDialog::decision() const
{
    return _decision;
}
