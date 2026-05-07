// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file connectiondialog.cpp
/// \brief Implements the OPC UA connection dialog.
///

#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>

#include "connectiondialog.h"
#include "ui_connectiondialog.h"
#include "widgets/coloredpushbutton.h"

///
/// \brief ConnectionDialog::ConnectionDialog
/// \param parent
///
ConnectionDialog::ConnectionDialog(QWidget *parent)
    : AppBaseDialog(parent)
    , ui(new Ui::ConnectionDialog)
{
    ui->setupUi(this);

    ui->clientCertificateHintIcon->setIcon(QStringLiteral("info"), 24);
    ui->serverCertificateIconLabel->setIcon(QStringLiteral("lock"), 48);
    ui->statusIconLabel->setIcon(QStringLiteral("disconnected"), 16);
    ui->connectButton->setColors({ QColor(0x0a74d1), QColor(0x1682df), QColor(0x075ca7) });

    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui->connectButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(ui->showPasswordCheckBox, &QCheckBox::toggled, ui->passwordEdit, [this](bool checked) {
        ui->passwordEdit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
    });
}

///
/// \brief ConnectionDialog::~ConnectionDialog
///
ConnectionDialog::~ConnectionDialog()
{
    delete ui;
}
