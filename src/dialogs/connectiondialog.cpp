// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file connectiondialog.cpp
/// \brief Implements the OPC UA connection dialog.
///

#include <QApplication>
#include <QCheckBox>
#include <QEvent>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>

#include "appicons.h"
#include "connectiondialog.h"
#include "ui_connectiondialog.h"
#include "widgets/coloredpushbutton.h"

///
/// \brief ConnectionDialog::ConnectionDialog
/// \param parent
///
ConnectionDialog::ConnectionDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ConnectionDialog)
{
    ui->setupUi(this);
    setWindowIcon(AppIcons::themed("app.ico"));

    ui->clientCertificateHintIcon->setIcon("info", 24);
    ui->serverCertificateIconLabel->setIcon("lock", 48);
    ui->statusIconLabel->setIcon("disconnected", 16);
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
