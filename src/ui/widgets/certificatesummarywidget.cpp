// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file certificatesummarywidget.cpp
/// \brief Implements the reusable certificate summary panel.
///

#include <QColor>
#include <QEvent>
#include <QFontMetrics>

#include "appicons.h"
#include "certificatesummarywidget.h"
#include "opcua/certificateinfo.h"
#include "ui_certificatesummarywidget.h"

///
/// \brief Builds the panel, applies the theme, and shows the empty state.
/// \param parent Owning widget.
///
CertificateSummaryWidget::CertificateSummaryWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CertificateSummaryWidget)
{
    ui->setupUi(this);

    ui->headerIcon->setIcon(QStringLiteral("certificate"), QSize(18, 18));
    ui->validIcon->setIcon(QStringLiteral("check-circle"), QSize(18, 18));
    ui->detailsWidget->setStyleSheet(QStringLiteral(
        "QLabel[certCaption=\"true\"] { color: #6b7280; }"));
    ui->serialNumberEdit->installEventFilter(this);

    applyTheme();
    updateContents();
}

///
/// \brief Destroys the widget and its generated UI.
///
CertificateSummaryWidget::~CertificateSummaryWidget()
{
    delete ui;
}

///
/// \brief Sets the header caption, hiding the header when empty.
/// \param title Header caption shown next to the certificate icon.
///
void CertificateSummaryWidget::setTitle(const QString &title)
{
    const bool show = !title.isEmpty();
    ui->headerLabel->setText(title);
    ui->headerIcon->setVisible(show);
    ui->headerLabel->setVisible(show);
    ui->headerLine->setVisible(show);
}

///
/// \brief Sets the message shown instead of the details when no certificate is set.
/// \param hint Message shown instead of the details when no certificate is set.
///
/// Setting a non-empty hint switches the empty state to "hint mode": the detail
/// grid is hidden and the dimmed header plus the hint are shown (server panel).
///
void CertificateSummaryWidget::setHint(const QString &hint)
{
    _hint = hint;
    ui->hintLabel->setText(hint);
    updateContents();
}

///
/// \brief Sets the placeholder shown in the subject field when no certificate is set.
/// \param text Placeholder shown in the subject field when no certificate is set.
///
/// Used when no hint is configured: the detail grid stays visible and shows this
/// placeholder instead (client panel).
///
void CertificateSummaryWidget::setEmptyText(const QString &text)
{
    _emptyText = text;
    updateContents();
}

///
/// \brief Shows a DER certificate, or clears the panel when empty.
/// \param der DER-encoded certificate, or an empty array to clear the panel.
///
void CertificateSummaryWidget::setCertificate(const QByteArray &der)
{
    _certificate = der;
    updateContents();
}

///
/// \brief Clears the displayed certificate.
///
void CertificateSummaryWidget::clear()
{
    setCertificate({});
}

///
/// \brief Returns the certificate currently shown.
/// \return DER-encoded certificate currently shown, or an empty array.
///
QByteArray CertificateSummaryWidget::certificate() const
{
    return _certificate;
}

///
/// \brief Applies the theme-dependent header colour.
///
/// Applies the theme-dependent header colour. Read-only theme access only; the
/// panel never connects to theme signals so it stays safe under test harnesses.
///
void CertificateSummaryWidget::applyTheme()
{
    const bool darkTheme = AppIcons::isDarkTheme();
    ui->headerLabel->setStyleSheet(QStringLiteral(
        "QLabel { color: %1; font-weight: 600; }"
        "QLabel:disabled { color: %2; }")
        .arg(darkTheme ? QStringLiteral("#60a5fa") : QStringLiteral("#2563eb"),
             darkTheme ? QStringLiteral("#5b626b") : QStringLiteral("#9aa0a6")));
}

///
/// \brief Reflects the current certificate (or empty state) in the panel.
///
/// Reflects the current certificate (or empty state) in the header, hint label
/// and detail grid.
///
void CertificateSummaryWidget::updateContents()
{
    const bool available = !_certificate.isEmpty();
    const bool hintMode = !_hint.isEmpty();
    const bool headerActive = available || !hintMode;
    ui->headerIcon->setEnabled(headerActive);
    ui->headerLabel->setEnabled(headerActive);

    if (!available) {
        _serialNumber.clear();
        ui->hintLabel->setVisible(hintMode);
        ui->detailsWidget->setVisible(!hintMode);
        if (!hintMode) {
            ui->subjectEdit->setText(_emptyText);
            ui->issuerEdit->clear();
            ui->validEdit->clear();
            ui->serialNumberEdit->clear();
            ui->validIcon->setVisible(false);
            ui->validBadge->clear();
        }
        return;
    }

    ui->hintLabel->setVisible(false);
    ui->detailsWidget->setVisible(true);
    fillCertificateFields();
}

///
/// \brief Describes the current certificate in the detail grid.
///
/// Describes the current certificate in the detail grid.
///
void CertificateSummaryWidget::fillCertificateFields()
{
    const CertificateInfo info = CertificateInfo::fromDer(_certificate);
    _serialNumber = info.serialNumber;
    ui->serialNumberEdit->setToolTip(
        _serialNumber.isEmpty() ? QString() : tr("Serial number: %1").arg(_serialNumber));
    elideSerialNumber();

    if (!info.readable) {
        ui->subjectEdit->setText(tr("Unable to read certificate"));
        ui->issuerEdit->clear();
        ui->validEdit->setText(tr("%1 bytes").arg(_certificate.size()));
        ui->validIcon->setVisible(false);
        ui->validBadge->clear();
        return;
    }

    ui->subjectEdit->setText(info.subject);
    QString issuer = info.issuer;
    if (info.selfSigned && !issuer.isEmpty())
        issuer = tr("%1 (self-signed)").arg(issuer);
    ui->issuerEdit->setText(issuer);
    ui->validEdit->setText(
        info.expiryDate.toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));

    QString badgeText;
    QColor color;
    bool valid = false;
    if (info.status == CertificateInfo::Status::NotYetValid) {
        badgeText = tr("Not yet valid");
        color = QColor(0xc0, 0x7d, 0x00);
    } else if (info.status == CertificateInfo::Status::Expired) {
        badgeText = tr("Expired");
        color = QColor(0xd1, 0x34, 0x38);
    } else if (info.status == CertificateInfo::Status::Valid) {
        badgeText = tr("Valid");
        color = QColor(0x2e, 0x9e, 0x44);
        valid = true;
    } else {
        badgeText = tr("Invalid");
        color = QColor(0xd1, 0x34, 0x38);
    }

    ui->validIcon->setVisible(valid);
    ui->validBadge->setText(badgeText);
    ui->validBadge->setStyleSheet(
        QStringLiteral("color: %1; font-weight: 600;").arg(color.name()));
}

///
/// \brief Truncates the serial number on a byte boundary to fit the label.
///
void CertificateSummaryWidget::elideSerialNumber()
{
    QLabel *label = ui->serialNumberEdit;
    if (_serialNumber.isEmpty()) {
        label->setText(tr("Unavailable"));
        return;
    }

    const QFontMetrics metrics(label->font());
    const int available = label->contentsRect().width();
    if (available <= 0 || metrics.horizontalAdvance(_serialNumber) <= available) {
        label->setText(_serialNumber);
        return;
    }

    const QStringList bytes = _serialNumber.split(QLatin1Char(':'));
    if (bytes.size() == 1) {
        label->setText(metrics.elidedText(_serialNumber, Qt::ElideRight, available));
        return;
    }

    const QString ellipsis = QStringLiteral("...");
    QString shown;
    for (const QString &part : bytes) {
        const QString candidate = shown.isEmpty()
            ? part : shown + QLatin1Char(':') + part;
        if (!shown.isEmpty()
            && metrics.horizontalAdvance(candidate + ellipsis) > available) {
            break;
        }
        shown = candidate;
    }
    label->setText(shown.isEmpty() ? ellipsis : shown + ellipsis);
}

///
/// \brief Re-elides the serial number when its label is resized.
/// \param watched Watched object.
/// \param event Event being delivered.
/// \return True if the event was handled.
///
bool CertificateSummaryWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->serialNumberEdit && event->type() == QEvent::Resize)
        elideSerialNumber();
    return QWidget::eventFilter(watched, event);
}
