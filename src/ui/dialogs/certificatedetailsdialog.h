// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file certificatedetailsdialog.h
/// \brief Declares the certificate details dialog.
///

#pragma once

#include "dialogs/appbasedialog.h"

#include <QByteArray>
#include <QString>

namespace Ui {
class CertificateDetailsDialog;
}

///
/// \brief Shows parsed certificate summary and technical fields.
///
class CertificateDetailsDialog : public AppBaseDialog
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the dialog and wires its actions.
    /// \param parent Parent widget.
    ///
    explicit CertificateDetailsDialog(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the dialog and its generated UI.
    ///
    ~CertificateDetailsDialog() override;

    ///
    /// \brief Shows a DER certificate in the details grid.
    /// \param certificate DER-encoded certificate.
    ///
    void setCertificate(const QByteArray &certificate);

private slots:
    void copyDetails();
    void exportCertificate();

private:
    void clearDetails();
    void setSummaryStatus(const QString &text, bool valid);

    Ui::CertificateDetailsDialog *ui;
    QByteArray _certificate;
    QString _detailsText;
};
