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
    enum Decision {
        Reject = 0,
        TrustOnce,
        TrustPermanently
    };

    explicit CertificateTrustDialog(QWidget *parent = nullptr);
    ~CertificateTrustDialog() override;

    void setCertificate(const QByteArray &certificate, const QString &message);
    void setViewOnly(bool viewOnly);
    Decision decision() const;

private:
    Ui::CertificateTrustDialog *ui;
    Decision _decision = Reject;
};
