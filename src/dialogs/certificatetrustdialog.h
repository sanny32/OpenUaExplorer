// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file certificatetrustdialog.h
/// \brief Declares the server certificate trust decision dialog.
///

#pragma once

#include <QDialog>

namespace Ui {
class CertificateTrustDialog;
}

///
/// \brief Presents an untrusted server certificate and records the trust decision.
///
class CertificateTrustDialog : public QDialog
{
    Q_OBJECT

public:
    ///
    /// \brief Trust choices available for an untrusted server certificate.
    ///
    enum Decision {
        Reject = 0,
        TrustOnce,
        TrustPermanently
    };

    ///
    /// \brief Builds the dialog and wires its reject/trust-once/trust-permanently buttons.
    /// \param parent Parent widget.
    ///
    explicit CertificateTrustDialog(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the dialog and its generated UI.
    ///
    ~CertificateTrustDialog() override;

    ///
    /// \brief Shows the certificate's fingerprint, size, and validation message.
    /// \param certificate DER-encoded certificate.
    /// \param message Validation error description.
    ///
    void setCertificate(const QByteArray &certificate, const QString &message);

    ///
    /// \brief Hides the trust buttons and retitles the dialog for a read-only view.
    /// \param viewOnly Whether the dialog should only display certificate details.
    ///
    void setViewOnly(bool viewOnly);

    ///
    /// \brief Returns the trust decision the user made.
    /// \return Selected decision.
    ///
    Decision decision() const;

private:
    Ui::CertificateTrustDialog *ui;
    Decision _decision = Reject;
};
