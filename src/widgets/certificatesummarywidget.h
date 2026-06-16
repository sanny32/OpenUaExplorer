// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file certificatesummarywidget.h
/// \brief Declares a reusable certificate summary panel.
///

#pragma once

#include <QByteArray>
#include <QString>
#include <QWidget>

namespace Ui {
class CertificateSummaryWidget;
}

///
/// \brief Shows a certificate's subject, issuer, validity and fingerprint.
///
/// Bundles the header, the detail grid (Subject / Issuer / Valid-until + status
/// badge / SHA-256 fingerprint) and the "View..." button so a dialog can treat
/// a certificate panel as a single control. The panel reads a DER-encoded
/// certificate through setCertificate() and elides the fingerprint on resize.
///
class CertificateSummaryWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CertificateSummaryWidget(QWidget *parent = nullptr);
    ~CertificateSummaryWidget() override;

    void setTitle(const QString &title);
    void setHint(const QString &hint);
    void setEmptyText(const QString &text);
    void setCertificate(const QByteArray &der);
    void clear();
    QByteArray certificate() const;

signals:
    ///
    /// \brief Emitted when the user requests the full certificate view.
    ///
    void viewRequested();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void applyTheme();
    void updateContents();
    void fillCertificateFields();
    void elideFingerprint();

    Ui::CertificateSummaryWidget *ui;
    QByteArray _certificate;
    QString _fingerprint;
    QString _hint;
    QString _emptyText;
};
