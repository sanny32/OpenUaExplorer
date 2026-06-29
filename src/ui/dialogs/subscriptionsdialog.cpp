// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file subscriptionsdialog.cpp
/// \brief Implements the subscriptions management dialog.
///

#include <QDialogButtonBox>

#include "dialogs/subscriptionsdialog.h"
#include "ui_subscriptionsdialog.h"
#include "widgets/subscriptionswidget.h"

///
/// \brief Builds the dialog and its hosted subscriptions widget.
/// \param parent Parent widget.
///
SubscriptionsDialog::SubscriptionsDialog(QWidget *parent)
    : AppBaseDialog(parent)
    , ui(new Ui::SubscriptionsDialog)
{
    ui->setupUi(this);
    ui->buttonBox->setStandardButtons(QDialogButtonBox::Close);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::hide);
}

///
/// \brief Destroys the dialog and its generated UI.
///
SubscriptionsDialog::~SubscriptionsDialog()
{
    delete ui;
}

///
/// \brief Returns the hosted subscriptions widget.
/// \return Subscriptions widget.
///
SubscriptionsWidget *SubscriptionsDialog::subscriptions() const
{
    return ui->subscriptionsWidget;
}
