// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file subscriptionsdialog.h
/// \brief Declares the subscriptions management dialog.
///

#pragma once

#include "dialogs/appbasedialog.h"

namespace Ui {
class SubscriptionsDialog;
}

class QEvent;
class SubscriptionsWidget;

///
/// \brief Modeless dialog that hosts subscription management.
///
class SubscriptionsDialog : public AppBaseDialog
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the dialog and its hosted subscriptions widget.
    /// \param parent Parent widget.
    ///
    explicit SubscriptionsDialog(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the dialog and its generated UI.
    ///
    ~SubscriptionsDialog() override;

    ///
    /// \brief Returns the hosted subscriptions widget.
    /// \return Subscriptions widget.
    ///
    SubscriptionsWidget *subscriptions() const;

protected:
    void changeEvent(QEvent *event) override;

private:
    Ui::SubscriptionsDialog *ui;
};
