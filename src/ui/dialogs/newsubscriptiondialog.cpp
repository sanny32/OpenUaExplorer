// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file newsubscriptiondialog.cpp
/// \brief Implements the new-subscription creation dialog.
///

#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include "newsubscriptiondialog.h"
#include "widgets/dialogbuttonbox.h"

namespace {

/// \brief Smallest publishing interval offered by the editor, in milliseconds.
constexpr int minIntervalMs = 50;
/// \brief Largest publishing interval offered by the editor, in milliseconds.
constexpr int maxIntervalMs = 600000;
/// \brief Spin-box step between offered intervals, in milliseconds.
constexpr int intervalStepMs = 100;
/// \brief Publishing interval suggested for a new subscription, in milliseconds.
constexpr int defaultIntervalMs = 1000;

}

///
/// \brief Builds the dialog, rejecting names that already exist.
/// \param existingNames Names already in use; the OK button stays disabled for duplicates.
/// \param parent Parent widget.
///
NewSubscriptionDialog::NewSubscriptionDialog(QStringList existingNames, QWidget *parent)
    : AppBaseDialog(parent)
    , _existingNames(std::move(existingNames))
    , _nameEdit(new QLineEdit(this))
    , _intervalSpin(new QSpinBox(this))
    , _buttonBox(new DialogButtonBox(this))
{
    setWindowTitle(tr("New Subscription"));

    _intervalSpin->setRange(minIntervalMs, maxIntervalMs);
    _intervalSpin->setSingleStep(intervalStepMs);
    _intervalSpin->setSuffix(QStringLiteral(" ms"));
    _intervalSpin->setValue(defaultIntervalMs);

    auto *form = new QFormLayout;
    form->addRow(tr("Name:"), _nameEdit);
    form->addRow(tr("Publishing interval:"), _intervalSpin);

    _buttonBox->setStandardButtons(DialogButtonBox::Ok | DialogButtonBox::Cancel);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(_buttonBox);

    connect(_nameEdit, &QLineEdit::textChanged, this, &NewSubscriptionDialog::validate);
    connect(_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    validate();
}

///
/// \brief Returns the entered subscription name, trimmed of surrounding whitespace.
/// \return Subscription name.
///
QString NewSubscriptionDialog::subscriptionName() const
{
    return _nameEdit->text().trimmed();
}

///
/// \brief Returns the chosen publishing interval.
/// \return Publishing interval in milliseconds.
///
double NewSubscriptionDialog::publishingInterval() const
{
    return static_cast<double>(_intervalSpin->value());
}

///
/// \brief Keeps the OK button enabled only for a non-empty, unused name.
///
void NewSubscriptionDialog::validate()
{
    const QString name = subscriptionName();
    const bool valid = !name.isEmpty() && !_existingNames.contains(name);
    if (QPushButton *ok = _buttonBox->button(DialogButtonBox::Ok))
        ok->setEnabled(valid);
}
