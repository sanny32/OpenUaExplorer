// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file editfavoritedialog.cpp
/// \brief Implements the favourite edit dialog.
///

#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>

#include "appcolors.h"
#include "editfavoritedialog.h"
#include "ui_editfavoritedialog.h"

namespace {

/// \brief Standard OPC UA security policy URI prefix.
constexpr char securityPolicyPrefix[] = "http://opcfoundation.org/UA/SecurityPolicy#";

/// \brief Combo-box item roles carrying the policy URI and message security mode.
constexpr int policyRole = Qt::UserRole;
constexpr int modeRole = Qt::UserRole + 1;

///
/// \brief One offered security policy/mode pairing.
///
struct SecurityOption {
    /// \brief Policy short name (the part after '#').
    const char *policy;
    /// \brief Message security mode value (1 None, 2 Sign, 3 Sign & Encrypt).
    int mode;
};

/// \brief Standard policy/mode pairings offered for a favourite.
constexpr SecurityOption securityOptions[] = {
    {"None", 1},
    {"Basic128Rsa15", 2}, {"Basic128Rsa15", 3},
    {"Basic256", 2}, {"Basic256", 3},
    {"Basic256Sha256", 2}, {"Basic256Sha256", 3},
    {"Aes128_Sha256_RsaOaep", 2}, {"Aes128_Sha256_RsaOaep", 3},
    {"Aes256_Sha256_RsaPss", 2}, {"Aes256_Sha256_RsaPss", 3},
};

///
/// \brief Formats a policy name and mode as a "policy / mode" label.
/// \param policyName Policy short name.
/// \param mode Message security mode value.
/// \return Human-readable label.
///
QString securityDisplay(const QString &policyName, int mode)
{
    if (mode <= 1)
        return QStringLiteral("None");
    const QString modeName = mode == 2 ? QStringLiteral("Sign")
                                       : QStringLiteral("SignAndEncrypt");
    return QStringLiteral("%1 / %2").arg(policyName, modeName);
}

}

///
/// \brief Builds the dialog from its generated UI and themed styling.
/// \param parent Parent widget.
///
EditFavoriteDialog::EditFavoriteDialog(QWidget *parent)
    : AppBaseDialog(parent)
    , ui(new Ui::EditFavoriteDialog)
{
    ui->setupUi(this);
    ui->starIcon->setIcon(QStringLiteral("star"), QSize(20, 20));
    ui->saveButton->setColors(
        { AppColors::accent(), AppColors::accentHover(), AppColors::accentPressed() });
    applyStyling();
    populateSecurityOptions();

    connect(ui->serverUrlEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        ui->saveButton->setEnabled(!text.trimmed().isEmpty());
    });
    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui->saveButton, &QPushButton::clicked, this, &QDialog::accept);
}

///
/// \brief Destroys the dialog and its generated UI.
///
EditFavoriteDialog::~EditFavoriteDialog()
{
    delete ui;
}

///
/// \brief Applies the theme-aware label colours that cannot be expressed in the .ui file.
///
void EditFavoriteDialog::applyStyling()
{
    ui->titleLabel->setStyleSheet(QStringLiteral("font-size: 16px; font-weight: bold; color: %1;")
                                      .arg(AppColors::titleText().name()));
    ui->subtitleLabel->setStyleSheet(QStringLiteral("color: %1;")
                                         .arg(AppColors::subtitleText().name()));
    const QString labelStyle = QStringLiteral("font-weight: bold; color: %1;")
                                   .arg(AppColors::titleText().name());
    ui->serverUrlLabel->setStyleSheet(labelStyle);
    ui->securityLabel->setStyleSheet(labelStyle);
    ui->hintLabel->setStyleSheet(QStringLiteral("color: %1;").arg(AppColors::hint().name()));
}

///
/// \brief Fills the security combo with the standard policy/mode pairings.
///
void EditFavoriteDialog::populateSecurityOptions()
{
    for (const SecurityOption &option : securityOptions) {
        const QString policyName = QString::fromLatin1(option.policy);
        ui->securityComboBox->addItem(securityDisplay(policyName, option.mode));
        const int index = ui->securityComboBox->count() - 1;
        ui->securityComboBox->setItemData(
            index, QString::fromLatin1(securityPolicyPrefix) + policyName, policyRole);
        ui->securityComboBox->setItemData(index, option.mode, modeRole);
    }
}

///
/// \brief Selects the combo entry matching a policy and mode, adding it when non-standard.
/// \param policy Stored security policy URI (or short name).
/// \param mode Message security mode value.
///
void EditFavoriteDialog::selectSecurity(const QString &policy, int mode)
{
    const QString policyName = policy.section(QLatin1Char('#'), -1);
    for (int i = 0; i < ui->securityComboBox->count(); ++i) {
        const QString itemPolicy = ui->securityComboBox->itemData(i, policyRole).toString();
        if (itemPolicy.section(QLatin1Char('#'), -1) == policyName
            && ui->securityComboBox->itemData(i, modeRole).toInt() == mode) {
            ui->securityComboBox->setCurrentIndex(i);
            return;
        }
    }

    ui->securityComboBox->insertItem(0, securityDisplay(policyName, mode));
    ui->securityComboBox->setItemData(0, policy, policyRole);
    ui->securityComboBox->setItemData(0, mode, modeRole);
    ui->securityComboBox->setCurrentIndex(0);
}

///
/// \brief Pre-fills the dialog from a favourite, keeping its other settings to write back.
/// \param profile Favourite being edited.
///
void EditFavoriteDialog::setProfile(const ConnectionProfile &profile)
{
    _profile = profile;
    ui->serverUrlEdit->setText(profile.endpointUrl);
    ui->serverUrlEdit->setCursorPosition(0);
    selectSecurity(profile.securityPolicy, profile.securityMode);
}

///
/// \brief Returns the favourite with the edited URL and security policy/mode applied.
/// \return Updated profile.
///
ConnectionProfile EditFavoriteDialog::profile() const
{
    ConnectionProfile result = _profile;
    const QString url = ui->serverUrlEdit->text().trimmed();
    // Keep the display name mirroring the URL when it was never given a distinct name.
    if (result.name.isEmpty() || result.name == _profile.endpointUrl)
        result.name = url;
    result.endpointUrl = url;
    result.securityPolicy = ui->securityComboBox->currentData(policyRole).toString();
    result.securityMode = ui->securityComboBox->currentData(modeRole).toInt();
    return result;
}
