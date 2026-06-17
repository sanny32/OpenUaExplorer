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
/// \brief Shows a certificate's subject, issuer, validity, and serial number.
///
/// The panel reads a DER-encoded certificate through setCertificate() and
/// elides the serial number on resize.
///
class CertificateSummaryWidget : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the panel, applies the theme, and shows the empty state.
    /// \param parent Owning widget.
    ///
    explicit CertificateSummaryWidget(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the widget and its generated UI.
    ///
    ~CertificateSummaryWidget() override;

    ///
    /// \brief Sets the header caption, hiding the header when empty.
    /// \param title Header caption shown next to the certificate icon.
    ///
    void setTitle(const QString &title);

    ///
    /// \brief Sets the message shown instead of the details when no certificate is set.
    /// \param hint Message shown instead of the details when no certificate is set.
    ///
    void setHint(const QString &hint);

    ///
    /// \brief Sets the placeholder shown in the subject field when no certificate is set.
    /// \param text Placeholder shown in the subject field when no certificate is set.
    ///
    void setEmptyText(const QString &text);

    ///
    /// \brief Shows a DER certificate, or clears the panel when empty.
    /// \param der DER-encoded certificate, or an empty array to clear the panel.
    ///
    void setCertificate(const QByteArray &der);

    ///
    /// \brief Clears the displayed certificate.
    ///
    void clear();

    ///
    /// \brief Returns the certificate currently shown.
    /// \return DER-encoded certificate currently shown, or an empty array.
    ///
    QByteArray certificate() const;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void applyTheme();
    void updateContents();
    void fillCertificateFields();
    void elideSerialNumber();

    Ui::CertificateSummaryWidget *ui;
    QByteArray _certificate;
    QString _serialNumber;
    QString _hint;
    QString _emptyText;
};
