// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file securityselectorwidget.cpp
/// \brief Implements the security policy selector widget.
///

#include <QString>

#include "securityselectorwidget.h"
#include "ui_securityselectorwidget.h"

///
/// \brief SecuritySelectorWidget::SecuritySelectorWidget
/// \param parent
///
SecuritySelectorWidget::SecuritySelectorWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SecuritySelectorWidget)
{
    ui->setupUi(this);

    ui->securityComboBox->addItem("None");
    ui->securityComboBox->addItem("Basic256Sha256 / Sign");
    ui->securityComboBox->addItem("Basic256Sha256 / Sign & Encrypt");
}

///
/// \brief SecuritySelectorWidget::~SecuritySelectorWidget
///
SecuritySelectorWidget::~SecuritySelectorWidget()
{
    delete ui;
}

///
/// \brief SecuritySelectorWidget::currentSecurityPolicy
/// \return
///
QString SecuritySelectorWidget::currentSecurityPolicy() const
{
    return ui->securityComboBox->currentText();
}
