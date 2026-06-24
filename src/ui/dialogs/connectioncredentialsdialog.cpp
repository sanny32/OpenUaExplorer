// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file connectioncredentialsdialog.cpp
/// \brief Implements the favourite connection-credentials dialog.
///

#include <QAction>
#include <QFileDialog>
#include <QLineEdit>
#include <QPushButton>

#include "appcolors.h"
#include "appicons.h"
#include "connectioncredentialsdialog.h"
#include "ui_connectioncredentialsdialog.h"

///
/// \brief Builds the dialog from its generated UI and themed styling.
/// \param parent Parent widget.
///
ConnectionCredentialsDialog::ConnectionCredentialsDialog(QWidget *parent)
    : AppBaseDialog(parent)
    , ui(new Ui::ConnectionCredentialsDialog)
{
    ui->setupUi(this);
    ui->connectButton->setColors(
        { AppColors::accent(), AppColors::accentHover(), AppColors::accentPressed() });
    applyStyling();
    setupPasswordToggle();

    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui->connectButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(ui->certificateBrowseButton, &QPushButton::clicked,
            this, &ConnectionCredentialsDialog::browseCertificate);
    connect(ui->privateKeyBrowseButton, &QPushButton::clicked,
            this, &ConnectionCredentialsDialog::browsePrivateKey);
}

///
/// \brief Destroys the dialog and its generated UI.
///
ConnectionCredentialsDialog::~ConnectionCredentialsDialog()
{
    delete ui;
}

///
/// \brief Applies the theme-aware label colours that cannot be expressed in the .ui file.
///
void ConnectionCredentialsDialog::applyStyling()
{
    ui->titleLabel->setStyleSheet(QStringLiteral("font-size: 16px; font-weight: bold; color: %1;")
                                      .arg(AppColors::titleText().name()));
    ui->subtitleLabel->setStyleSheet(QStringLiteral("color: %1;")
                                         .arg(AppColors::subtitleText().name()));
    const QString labelStyle = QStringLiteral("font-weight: bold; color: %1;")
                                   .arg(AppColors::titleText().name());
    ui->usernameLabel->setStyleSheet(labelStyle);
    ui->passwordLabel->setStyleSheet(labelStyle);
    ui->certificateLabel->setStyleSheet(labelStyle);
    ui->privateKeyLabel->setStyleSheet(labelStyle);
    ui->privateKeyPasswordLabel->setStyleSheet(labelStyle);
}

///
/// \brief Adds a show/hide toggle to each password field.
///
void ConnectionCredentialsDialog::setupPasswordToggle()
{
    for (QLineEdit *edit : { ui->passwordEdit, ui->privateKeyPasswordEdit }) {
        QAction *toggle = edit->addAction(AppIcons::themed(QStringLiteral("eye")),
                                          QLineEdit::TrailingPosition);
        toggle->setCheckable(true);
        connect(toggle, &QAction::toggled, this, [edit, toggle](bool shown) {
            edit->setEchoMode(shown ? QLineEdit::Normal : QLineEdit::Password);
            toggle->setIcon(AppIcons::themed(
                shown ? QStringLiteral("eye-off") : QStringLiteral("eye")));
        });
    }
}

///
/// \brief Pre-fills the dialog from a favourite and shows the matching credential panel.
/// \param profile Favourite being connected.
///
void ConnectionCredentialsDialog::setProfile(const ConnectionProfile &profile)
{
    _profile = profile;

    const bool certificate =
        profile.authentication == ConnectionProfile::Authentication::Certificate;
    ui->authStack->setCurrentWidget(certificate ? ui->certificatePage : ui->usernamePage);
    ui->headerIcon->setIcon(certificate ? QStringLiteral("certificate")
                                         : QStringLiteral("lock"),
                            QSize(20, 20));

    if (certificate) {
        ui->certificateEdit->setText(profile.clientCertificateFile);
        ui->certificateEdit->setCursorPosition(0);
        ui->privateKeyEdit->setText(profile.privateKeyFile);
        ui->privateKeyEdit->setCursorPosition(0);
    } else {
        ui->usernameEdit->setText(profile.username);
        ui->passwordEdit->setFocus();
    }
}

///
/// \brief Returns the favourite with the entered username and certificate paths applied.
/// \return Updated profile.
///
ConnectionProfile ConnectionCredentialsDialog::profile() const
{
    ConnectionProfile result = _profile;
    if (_profile.authentication == ConnectionProfile::Authentication::Certificate) {
        result.clientCertificateFile = ui->certificateEdit->text().trimmed();
        result.privateKeyFile = ui->privateKeyEdit->text().trimmed();
    } else {
        result.username = ui->usernameEdit->text();
    }
    return result;
}

///
/// \brief Returns the entered username password.
/// \return Username password.
///
QString ConnectionCredentialsDialog::password() const
{
    return ui->passwordEdit->text();
}

///
/// \brief Returns the entered private-key password.
/// \return Private-key password.
///
QString ConnectionCredentialsDialog::privateKeyPassword() const
{
    return ui->privateKeyPasswordEdit->text();
}

///
/// \brief Browses for the certificate used by certificate authentication.
///
void ConnectionCredentialsDialog::browseCertificate()
{
    const QString certificate = QFileDialog::getOpenFileName(
        this, tr("Select Client Certificate"), ui->certificateEdit->text(),
        tr("Certificates (*.der *.pem *.crt);;All Files (*)"));
    if (certificate.isEmpty())
        return;
    ui->certificateEdit->setText(certificate);
    ui->certificateEdit->setCursorPosition(0);
}

///
/// \brief Browses for the private key used by certificate authentication.
///
void ConnectionCredentialsDialog::browsePrivateKey()
{
    const QString key = QFileDialog::getOpenFileName(
        this, tr("Select Private Key"), ui->privateKeyEdit->text(),
        tr("Private Keys (*.pem *.key);;All Files (*)"));
    if (key.isEmpty())
        return;
    ui->privateKeyEdit->setText(key);
    ui->privateKeyEdit->setCursorPosition(0);
}
