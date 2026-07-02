// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file certificatedetailswidget.cpp
/// \brief Implements the reusable certificate details widget.
///

#include <QColor>
#include <QCryptographicHash>
#include <QDir>
#include <QLabel>
#include <QRegularExpression>
#include <QSslCertificate>
#include <QSslCertificateExtension>
#include <QVariant>

#include "appcolors.h"
#include "certificatedetailswidget.h"
#include "formatters/datetimeformatter.h"
#include "opcua/certificateinfo.h"
#include "ui_certificatedetailswidget.h"

namespace {

///
/// \brief Returns display text for an empty value.
/// \return Placeholder text.
///
QString unavailable()
{
    return QObject::tr("Unavailable");
}

///
/// \brief Formats a certificate date for the details widget.
/// \param value Date to format.
/// \return Local date text, or an unavailable placeholder.
///
QString formatDate(const QDateTime &value)
{
    const QString text = formatDateTime(value);
    return text.isEmpty() ? unavailable() : text;
}

///
/// \brief Builds a compact distinguished name from certificate attributes.
/// \param certificate Certificate to inspect.
/// \param subject True for subject attributes, false for issuer attributes.
/// \return Distinguished name text.
///
QString distinguishedName(const QSslCertificate &certificate, bool subject)
{
    const QList<QByteArray> attributes = subject
        ? certificate.subjectInfoAttributes()
        : certificate.issuerInfoAttributes();

    QStringList parts;
    for (const QByteArray &attribute : attributes) {
        const QStringList values = subject
            ? certificate.subjectInfo(attribute)
            : certificate.issuerInfo(attribute);
        for (const QString &value : values) {
            if (!value.isEmpty()) {
                parts.append(QStringLiteral("%1=%2")
                                 .arg(QString::fromLatin1(attribute), value));
            }
        }
    }
    return parts.isEmpty() ? unavailable() : parts.join(QStringLiteral(", "));
}

///
/// \brief Renders the subject alternative names as GeneralName tag/value pairs.
/// \param certificate Certificate to inspect.
/// \return Display text such as "[[6, urn:...], [2, host]]", or an unavailable placeholder.
///
QString subjectAlternativeNameText(const QSslCertificate &certificate)
{
    // RFC 5280 GeneralName ASN.1 tag numbers, listed in the order OPC UA tooling shows them.
    static const QList<QPair<QString, int>> generalNames = {
        {QStringLiteral("URI"), 6},
        {QStringLiteral("DNS"), 2},
        {QStringLiteral("IP"), 7},
        {QStringLiteral("Email"), 1},
    };

    for (const QSslCertificateExtension &extension : certificate.extensions()) {
        if (extension.oid() != QStringLiteral("2.5.29.17"))
            continue;

        const QVariantMap entries = extension.value().toMap();
        QStringList parts;
        for (const auto &generalName : generalNames) {
            const QStringList values = entries.value(generalName.first).toStringList();
            for (const QString &value : values) {
                if (!value.isEmpty()) {
                    parts.append(QStringLiteral("[%1, %2]")
                                     .arg(QString::number(generalName.second), value));
                }
            }
        }
        if (!parts.isEmpty())
            return QStringLiteral("[%1]").arg(parts.join(QStringLiteral(", ")));
    }
    return unavailable();
}

///
/// \brief Extracts the OPC UA application URI from subject alternative names.
/// \param certificate Certificate to inspect.
/// \return Application URI, or an unavailable placeholder.
///
QString applicationUri(const QSslCertificate &certificate)
{
    const QString alternativeNames = subjectAlternativeNameText(certificate);
    const QRegularExpression uriPattern(QStringLiteral(R"((urn:[^\],\)\s]+))"));
    const QRegularExpressionMatch match = uriPattern.match(alternativeNames);
    return match.hasMatch() ? match.captured(1) : unavailable();
}

///
/// \brief Extracts the signature algorithm from Qt's certificate text dump.
/// \param certificate Certificate to inspect.
/// \return Signature algorithm text.
///
QString signatureAlgorithm(const QSslCertificate &certificate)
{
    const QRegularExpression pattern(
        QStringLiteral(R"(Signature Algorithm:\s*([^\r\n]+))"));
    const QRegularExpressionMatch match = pattern.match(certificate.toText());
    return match.hasMatch() ? match.captured(1).trimmed() : unavailable();
}

///
/// \brief Builds the copy-to-clipboard text from the shown fields.
/// \param rows Label/value rows.
/// \return Plain text details.
///
QString buildDetailsText(const QList<QPair<QString, QString>> &rows)
{
    QStringList lines;
    for (const auto &row : rows)
        lines.append(QStringLiteral("%1 %2").arg(row.first, row.second));
    return lines.join(QLatin1Char('\n'));
}

}

///
/// \brief Builds the widget and prepares its empty state.
/// \param parent Owning widget.
///
CertificateDetailsWidget::CertificateDetailsWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CertificateDetailsWidget)
{
    ui->setupUi(this);

    ui->statusIcon->setIcon(QStringLiteral("shield-trusted"), QSize(22, 22));

    const QString labelStyle =
        QStringLiteral("color: %1;").arg(AppColors::fieldLabel().name());
    int labelColumnWidth = 0;
    for (QLabel *label : findChildren<QLabel *>()) {
        if (label->objectName().endsWith(QStringLiteral("Label"))) {
            label->setStyleSheet(labelStyle);
            labelColumnWidth = qMax(labelColumnWidth, label->sizeHint().width());
        }
    }
    ui->summaryLayout->setColumnMinimumWidth(0, labelColumnWidth);
    ui->technicalLayout->setColumnMinimumWidth(0, labelColumnWidth);

    clear();
}

///
/// \brief Destroys the widget and its generated UI.
///
CertificateDetailsWidget::~CertificateDetailsWidget()
{
    delete ui;
}

///
/// \brief Shows a DER certificate in the details grid.
/// \param certificate DER-encoded certificate.
/// \param certificatePath Path of the source certificate file, or empty when unavailable.
///
void CertificateDetailsWidget::setCertificate(const QByteArray &certificate,
                                              const QString &certificatePath)
{
    clear();
    _certificate = certificate;

    const QString displayedCertificatePath = QDir::toNativeSeparators(certificatePath);
    ui->certificatePathLabel->setVisible(!displayedCertificatePath.isEmpty());
    ui->certificatePathValue->setVisible(!displayedCertificatePath.isEmpty());
    ui->certificatePathValue->setText(displayedCertificatePath);

    const CertificateInfo info = CertificateInfo::fromDer(certificate);
    const QList<QSslCertificate> chain = QSslCertificate::fromData(certificate, QSsl::Der);
    if (chain.isEmpty()) {
        ui->nameValue->setText(unavailable());
        ui->serialNumberValue->setText(unavailable());
        setSummaryStatus(tr("Invalid"), false);
        QList<QPair<QString, QString>> rows = {
            {tr("Status:"), ui->statusValue->text()},
            {tr("Name:"), ui->nameValue->text()},
            {tr("Serial Number:"), ui->serialNumberValue->text()},
        };
        if (!displayedCertificatePath.isEmpty())
            rows.insert(2, {tr("Certificate Path:"), ui->certificatePathValue->text()});
        _detailsText = buildDetailsText(rows);
        return;
    }

    const QSslCertificate parsed = chain.constFirst();
    const QString signedBy = parsed.isSelfSigned() ? tr("Self Signed") : info.issuer;
    const QString serialNumber =
        QString::fromLatin1(parsed.serialNumber()).remove(QLatin1Char(':')).toLower();
    const QString thumbprint = QStringLiteral("[%1] 0x%2")
        .arg(parsed.digest(QCryptographicHash::Sha1).size())
        .arg(QString::fromLatin1(parsed.digest(QCryptographicHash::Sha1).toHex()));
    const QString keySize = info.keyBits > 0
        ? QString::number(info.keyBits)
        : unavailable();

    ui->nameValue->setText(info.subject.isEmpty() ? unavailable() : info.subject);
    ui->signedByValue->setText(signedBy.isEmpty() ? unavailable() : signedBy);
    ui->validFromValue->setText(formatDate(parsed.effectiveDate()));
    ui->validToValue->setText(formatDate(parsed.expiryDate()));
    ui->applicationUriValue->setText(applicationUri(parsed));
    ui->keySizeValue->setText(keySize);
    ui->serialNumberValue->setText(serialNumber.isEmpty() ? unavailable() : serialNumber);
    ui->signatureAlgorithmValue->setText(signatureAlgorithm(parsed));
    ui->issuerValue->setText(distinguishedName(parsed, false));
    ui->subjectValue->setText(distinguishedName(parsed, true));
    ui->subjectAlternativeNameValue->setText(subjectAlternativeNameText(parsed));
    ui->thumbprintValue->setText(thumbprint);

    bool valid = false;
    QString statusText;
    if (info.status == CertificateInfo::Status::Valid) {
        statusText = tr("Valid");
        valid = true;
    } else if (info.status == CertificateInfo::Status::Expired) {
        statusText = tr("Expired");
    } else if (info.status == CertificateInfo::Status::NotYetValid) {
        statusText = tr("Not yet valid");
    } else {
        statusText = tr("Invalid");
    }
    setSummaryStatus(statusText, valid);

    QList<QPair<QString, QString>> rows = {
        {tr("Status:"), ui->statusValue->text()},
        {tr("Name:"), ui->nameValue->text()},
        {tr("Signed By:"), ui->signedByValue->text()},
        {tr("Valid From:"), ui->validFromValue->text()},
        {tr("Valid To:"), ui->validToValue->text()},
        {tr("Application URI:"), ui->applicationUriValue->text()},
        {tr("Key Size:"), ui->keySizeValue->text()},
        {tr("Serial Number:"), ui->serialNumberValue->text()},
        {tr("Signature Algorithm:"), ui->signatureAlgorithmValue->text()},
        {tr("Issuer:"), ui->issuerValue->text()},
        {tr("Subject:"), ui->subjectValue->text()},
        {tr("Subject Alternative Name:"), ui->subjectAlternativeNameValue->text()},
        {tr("Thumbprint:"), ui->thumbprintValue->text()},
    };
    if (!displayedCertificatePath.isEmpty())
        rows.insert(7, {tr("Certificate Path:"), ui->certificatePathValue->text()});
    _detailsText = buildDetailsText(rows);
}

///
/// \brief Clears the displayed certificate and fields.
///
void CertificateDetailsWidget::clear()
{
    _certificate.clear();
    setSummaryStatus(unavailable(), false);
    ui->nameValue->setText(unavailable());
    ui->signedByValue->setText(unavailable());
    ui->validFromValue->setText(unavailable());
    ui->validToValue->setText(unavailable());
    ui->applicationUriValue->setText(unavailable());
    ui->keySizeValue->setText(unavailable());
    ui->certificatePathLabel->setVisible(false);
    ui->certificatePathValue->setVisible(false);
    ui->certificatePathValue->clear();
    ui->serialNumberValue->setText(unavailable());
    ui->signatureAlgorithmValue->setText(unavailable());
    ui->issuerValue->setText(unavailable());
    ui->subjectValue->setText(unavailable());
    ui->subjectAlternativeNameValue->setText(unavailable());
    ui->thumbprintValue->setText(unavailable());
    _detailsText.clear();
}

///
/// \brief Returns the certificate currently shown.
/// \return DER-encoded certificate currently shown, or an empty array.
///
QByteArray CertificateDetailsWidget::certificate() const
{
    return _certificate;
}

///
/// \brief Returns the copyable text for the currently shown fields.
/// \return Plain text certificate details.
///
QString CertificateDetailsWidget::detailsText() const
{
    return _detailsText;
}

///
/// \brief Updates the summary status text and colour.
/// \param text Status text.
/// \param valid Whether the status should use the valid colour.
///
void CertificateDetailsWidget::setSummaryStatus(const QString &text, bool valid)
{
    ui->statusValue->setText(text);
    ui->statusIcon->setVisible(valid);
    ui->statusValue->setStyleSheet(
        QStringLiteral("color: %1; font-weight: 600;")
            .arg(valid ? AppColors::statusSuccess().name() : AppColors::statusError().name()));
}
