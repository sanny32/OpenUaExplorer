// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file certificatedetailswidget.h
/// \brief Declares the reusable certificate details widget.
///

#pragma once

#include <QByteArray>
#include <QString>
#include <QWidget>

namespace Ui {
class CertificateDetailsWidget;
}

///
/// \brief Shows parsed certificate summary and technical fields.
///
class CertificateDetailsWidget : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the widget and prepares its empty state.
    /// \param parent Owning widget.
    ///
    explicit CertificateDetailsWidget(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the widget and its generated UI.
    ///
    ~CertificateDetailsWidget() override;

    ///
    /// \brief Shows a DER certificate in the details grid.
    /// \param certificate DER-encoded certificate.
    /// \param certificatePath Path of the source certificate file, or empty when unavailable.
    ///
    void setCertificate(const QByteArray &certificate,
                        const QString &certificatePath = QString());

    ///
    /// \brief Clears the displayed certificate and fields.
    ///
    void clear();

    ///
    /// \brief Returns the certificate currently shown.
    /// \return DER-encoded certificate currently shown, or an empty array.
    ///
    QByteArray certificate() const;

    ///
    /// \brief Returns the copyable text for the currently shown fields.
    /// \return Plain text certificate details.
    ///
    QString detailsText() const;

private:
    void setSummaryStatus(const QString &text, bool valid);

    Ui::CertificateDetailsWidget *ui;
    QByteArray _certificate;
    QString _detailsText;
};
