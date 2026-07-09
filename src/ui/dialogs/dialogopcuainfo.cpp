// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dialogopcuainfo.cpp
/// \brief Implements the OPC UA information dialog.
///

#include <QApplication>
#include <QClipboard>
#include <QEvent>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QScopedPointer>
#include <QtGlobal>

#include <QOpcUaClient>
#include <QOpcUaProvider>

#include "appcolors.h"
#include "appicons.h"
#include "dialogopcuainfo.h"
#include "ui_dialogopcuainfo.h"

namespace {

constexpr char securityPolicyPrefix[] = "http://opcfoundation.org/UA/SecurityPolicy#";

///
/// \brief Returns the open62541 version detected while configuring Qt OPC UA.
/// \return open62541 version, or an empty string when it was not detected.
///
QString open62541Version()
{
#ifdef OPCUA_OPEN62541_VERSION
    return QStringLiteral(OPCUA_OPEN62541_VERSION);
#else
    return {};
#endif
}

}

///
/// \brief Builds the OPC UA information dialog and fills its runtime fields.
/// \param parent Parent widget.
///
DialogOpcUaInfo::DialogOpcUaInfo(QWidget *parent)
    : AppBaseDialog(parent)
    , ui(new Ui::DialogOpcUaInfo)
{
    ui->setupUi(this);

    setupLayout();
    setupLogo();
    setupContent();
    setupFonts();
    setupLinks();

    ui->closeButton->setColors(
        { AppColors::accent(), AppColors::accentHover(), AppColors::accentPressed() });

    connect(ui->copyButton, &QPushButton::clicked, this, &DialogOpcUaInfo::copyInfo);
    connect(ui->closeButton, &QPushButton::clicked, this, &QDialog::accept);
}

///
/// \brief Destroys the dialog and its generated UI.
///
DialogOpcUaInfo::~DialogOpcUaInfo()
{
    delete ui;
}

///
/// \brief Returns the plain-text diagnostic summary copied by the dialog.
/// \return Current OPC UA information summary.
///
QString DialogOpcUaInfo::copyText() const
{
    return QStringLiteral(
        "OPC UA Info\n\n"
        "Stack\n"
        "OPC UA SDK: %1\n"
        "SDK Version: %2\n"
        "Qt OPC UA Version: %3\n\n"
        "Supported Security\n"
        "Security Policies: %4\n"
        "Message Security Modes: %5\n"
        "User Token Types: %6\n\n"
        "Specification\n"
        "OPC UA Specification: %7\n"
        "Version: %8\n"
        "Profile: %9\n\n"
        "Resources\n"
        "OPC Foundation: https://opcfoundation.org\n"
        "OPC UA Specification: https://reference.opcfoundation.org")
        .arg(rowValue(QStringLiteral("sdkValue")),
             rowValue(QStringLiteral("sdkVersionValue")),
             rowValue(QStringLiteral("qtOpcUaVersionValue")),
             rowValue(QStringLiteral("securityPoliciesValue")),
             rowValue(QStringLiteral("securityModesValue")),
             rowValue(QStringLiteral("userTokenTypesValue")),
             rowValue(QStringLiteral("specificationValue")),
             rowValue(QStringLiteral("specificationVersionValue")),
             rowValue(QStringLiteral("profileValue")));
}

///
/// \brief Returns security policies reported by the first available backend.
/// \return Supported security policy display names.
///
QStringList DialogOpcUaInfo::supportedSecurityPolicies() const
{
    QOpcUaProvider provider;
    const QStringList backends = provider.availableBackends();
    if (backends.isEmpty())
        return {};

    QScopedPointer<QOpcUaClient> client(provider.createClient(backends.first()));
    if (!client)
        return {};

    QStringList policies;
    for (const QString &policy : client->supportedSecurityPolicies())
        policies.append(securityPolicyName(policy));
    policies.removeDuplicates();
    policies.sort(Qt::CaseInsensitive);
    return policies;
}

///
/// \brief Refreshes the logo when the application palette changes.
/// \param event Change event being handled.
///
void DialogOpcUaInfo::changeEvent(QEvent *event)
{
    AppBaseDialog::changeEvent(event);

    if (event->type() == QEvent::PaletteChange
        || event->type() == QEvent::ApplicationPaletteChange) {
        setupLogo();
    }
}

///
/// \brief Copies the displayed OPC UA information to the clipboard.
///
void DialogOpcUaInfo::copyInfo()
{
    QApplication::clipboard()->setText(copyText());
}

///
/// \brief Populates the static and runtime labels.
///
void DialogOpcUaInfo::setupContent()
{
    QStringList policies = supportedSecurityPolicies();
    if (policies.isEmpty())
        policies = fallbackSecurityPolicies();

    setRowValue(QStringLiteral("sdkValue"), QStringLiteral("open62541"));
    const QString sdkVersion = open62541Version();
    setRowValue(QStringLiteral("sdkVersionValue"),
                sdkVersion.isEmpty() ? tr("Not available") : sdkVersion);
    setRowValue(QStringLiteral("qtOpcUaVersionValue"),
                tr("%1 (built against %2)").arg(qVersion(), QStringLiteral(QT_VERSION_STR)));
    setRowValue(QStringLiteral("securityPoliciesValue"), displayList(policies));
    setRowValue(QStringLiteral("securityModesValue"),
                tr("None, Sign, SignAndEncrypt"));
    setRowValue(QStringLiteral("userTokenTypesValue"),
                tr("Anonymous, Username, X509 Certificate"));
    setRowValue(QStringLiteral("specificationValue"), tr("Part 1-14"));
    setRowValue(QStringLiteral("specificationVersionValue"), tr("1.05.01"));
    setRowValue(QStringLiteral("profileValue"), tr("Client"));
}

///
/// \brief Applies emphasis and muted roles to dialog labels.
///
void DialogOpcUaInfo::setupFonts()
{
    QFont titleFont = ui->titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 4);
    titleFont.setBold(true);
    ui->titleLabel->setFont(titleFont);

    const QList<QLabel *> sectionLabels = {
        ui->stackTitleLabel,
        ui->securityTitleLabel,
        ui->specificationTitleLabel,
        ui->resourcesTitleLabel,
    };
    for (QLabel *label : sectionLabels) {
        QFont font = label->font();
        font.setBold(true);
        label->setFont(font);
    }
}

///
/// \brief Aligns table value columns across independent sections.
///
void DialogOpcUaInfo::setupLayout()
{
    constexpr int captionColumnWidth = 170;

    const QList<QGridLayout *> grids = {
        ui->stackGrid,
        ui->securityGrid,
        ui->specificationGrid,
        ui->resourcesGrid,
    };
    for (QGridLayout *grid : grids) {
        grid->setColumnMinimumWidth(0, captionColumnWidth);
        grid->setColumnStretch(0, 0);
        grid->setColumnStretch(1, 1);
        for (int index = 0; index < grid->count(); ++index) {
            QLayoutItem *item = grid->itemAt(index);
            if (item)
                item->setAlignment(Qt::AlignTop);
        }
    }
}

///
/// \brief Loads the official OPC UA logo from application resources.
///
void DialogOpcUaInfo::setupLogo()
{
    const QString logoResource = AppIcons::isDarkTheme()
        ? QStringLiteral(":/res/opcua-logo-dark.png")
        : QStringLiteral(":/res/opcua-logo-light.png");
    ui->logoLabel->setPixmap(QPixmap(logoResource));
}

///
/// \brief Configures external resource links.
///
void DialogOpcUaInfo::setupLinks()
{
    ui->foundationLinkLabel->setText(
        QStringLiteral("<a href=\"https://opcfoundation.org\">https://opcfoundation.org</a>"));
    ui->specificationLinkLabel->setText(
        QStringLiteral("<a href=\"https://reference.opcfoundation.org\">"
                       "https://reference.opcfoundation.org</a>"));

    const QList<QLabel *> linkLabels = { ui->foundationLinkLabel, ui->specificationLinkLabel };
    for (QLabel *label : linkLabels) {
        label->setTextFormat(Qt::RichText);
        label->setTextInteractionFlags(Qt::TextBrowserInteraction);
        label->setOpenExternalLinks(true);
    }
}

///
/// \brief Sets a value label identified by object name.
/// \param objectName Label object name.
/// \param value Value text.
///
void DialogOpcUaInfo::setRowValue(const QString &objectName, const QString &value)
{
    auto *label = findChild<QLabel *>(objectName);
    if (label)
        label->setText(value);
}

///
/// \brief Returns a value label's text by object name.
/// \param objectName Label object name.
/// \return Label text, or an empty string when the label is absent.
///
QString DialogOpcUaInfo::rowValue(const QString &objectName) const
{
    const auto *label = findChild<QLabel *>(objectName);
    return label ? label->text() : QString();
}

///
/// \brief Returns known OPC UA security policies used when no backend can report them.
/// \return Fallback policy names.
///
QStringList DialogOpcUaInfo::fallbackSecurityPolicies()
{
    return {
        QStringLiteral("None"),
        QStringLiteral("Basic128Rsa15"),
        QStringLiteral("Basic256"),
        QStringLiteral("Basic256Sha256"),
        QStringLiteral("Aes128_Sha256_RsaOaep"),
        QStringLiteral("Aes256_Sha256_RsaPss"),
    };
}

///
/// \brief Converts a policy URI to a compact display name.
/// \param policyUri OPC UA security policy URI or name.
/// \return Display name.
///
QString DialogOpcUaInfo::securityPolicyName(const QString &policyUri)
{
    if (policyUri.startsWith(QLatin1String(securityPolicyPrefix)))
        return policyUri.mid(static_cast<int>(qstrlen(securityPolicyPrefix)));
    return policyUri;
}

///
/// \brief Formats a string list for a table cell.
/// \param values Values to join.
/// \return Comma-separated values, or Not available.
///
QString DialogOpcUaInfo::displayList(const QStringList &values)
{
    if (values.isEmpty())
        return tr("Not available");
    return values.join(QStringLiteral(", "));
}
