// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file newsubscriptiondialog.h
/// \brief Declares the dialog for creating a new subscription.
///

#pragma once

#include <QStringList>

#include "appbasedialog.h"

class QLineEdit;
class QSpinBox;
class DialogButtonBox;

///
/// \brief Compact dialog that gathers a name and publishing interval for a new subscription.
///
class NewSubscriptionDialog : public AppBaseDialog
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the dialog, rejecting names that already exist.
    /// \param existingNames Names already in use; the OK button stays disabled for duplicates.
    /// \param parent Parent widget.
    ///
    explicit NewSubscriptionDialog(QStringList existingNames, QWidget *parent = nullptr);

    ///
    /// \brief Returns the entered subscription name, trimmed of surrounding whitespace.
    /// \return Subscription name.
    ///
    QString subscriptionName() const;

    ///
    /// \brief Returns the chosen publishing interval.
    /// \return Publishing interval in milliseconds.
    ///
    double publishingInterval() const;

private:
    void validate();

    QStringList      _existingNames;
    QLineEdit       *_nameEdit;
    QSpinBox        *_intervalSpin;
    DialogButtonBox *_buttonBox;
};
