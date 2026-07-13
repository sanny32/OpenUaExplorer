// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file certificatedetailsdialog.cpp
/// \brief Implements the certificate details dialog.
///

#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QSaveFile>

#include "appcolors.h"
#include "certificatedetailsdialog.h"
#include "messageboxdialog.h"
#include "ui_certificatedetailsdialog.h"

///
/// \brief Builds the dialog and wires its actions.
/// \param parent Parent widget.
///
CertificateDetailsDialog::CertificateDetailsDialog(QWidget *parent)
    : AppBaseDialog(parent)
    , ui(new Ui::CertificateDetailsDialog)
{
    ui->setupUi(this);

    ui->closeButton->setColors(
        { AppColors::accent(), AppColors::accentHover(), AppColors::accentPressed() });

    connect(ui->copyButton, &QPushButton::clicked,
            this, &CertificateDetailsDialog::copyDetails);
    connect(ui->exportButton, &QPushButton::clicked,
            this, &CertificateDetailsDialog::exportCertificate);
    connect(ui->closeButton, &QPushButton::clicked,
            this, &QDialog::accept);
}

///
/// \brief Destroys the dialog and its generated UI.
///
CertificateDetailsDialog::~CertificateDetailsDialog()
{
    delete ui;
}

///
/// \brief Shows a DER certificate in the details grid.
/// \param certificate DER-encoded certificate.
/// \param certificatePath Path of the source certificate file, or empty when unavailable.
///
void CertificateDetailsDialog::setCertificate(const QByteArray &certificate,
                                              const QString &certificatePath)
{
    ui->detailsWidget->setCertificate(certificate, certificatePath);
}

///
/// \brief Copies the displayed details to the clipboard.
///
void CertificateDetailsDialog::copyDetails()
{
    QApplication::clipboard()->setText(ui->detailsWidget->detailsText());
}

///
/// \brief Exports the displayed certificate in DER format.
///
void CertificateDetailsDialog::exportCertificate()
{
    const QByteArray certificate = ui->detailsWidget->certificate();
    if (certificate.isEmpty())
        return;

    const QString fileName = QFileDialog::getSaveFileName(
        this, tr("Export Certificate"), QString(),
        tr("DER Certificate (*.der);;All Files (*)"));
    if (fileName.isEmpty())
        return;

    QSaveFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)
        || file.write(certificate) != certificate.size()
        || !file.commit()) {
        MessageBoxDialog::critical(this, tr("Export Failed"),
                                   tr("Could not write the certificate file."));
    }
}
