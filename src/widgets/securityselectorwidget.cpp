// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file securityselectorwidget.cpp
/// \brief Implements the security policy selector widget.
///

#include "securityselectorwidget.h"
#include "ui_securityselectorwidget.h"

///
/// \brief Builds the selector and populates the security-policy choices.
/// \param parent Parent widget.
///
SecuritySelectorWidget::SecuritySelectorWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SecuritySelectorWidget)
{
    ui->setupUi(this);

    ui->securityComboBox->addItem(QStringLiteral("None"));
    ui->securityComboBox->addItem(QStringLiteral("Basic256Sha256 / Sign"));
    ui->securityComboBox->addItem(QStringLiteral("Basic256Sha256 / Sign & Encrypt"));
}

///
/// \brief Destroys the widget and its generated UI.
///
SecuritySelectorWidget::~SecuritySelectorWidget()
{
    delete ui;
}

///
/// \brief Returns the selected security policy.
/// \return Current security-policy text.
///
QString SecuritySelectorWidget::currentSecurityPolicy() const
{
    return ui->securityComboBox->currentText();
}
