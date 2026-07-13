// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file customintervaldialog.cpp
/// \brief Implements the custom trend history interval dialog.
///

#include <QDateTime>
#include <QDialogButtonBox>
#include <QPushButton>

#include "appcolors.h"
#include "customintervaldialog.h"
#include "messageboxdialog.h"
#include "ui_customintervaldialog.h"

namespace {

/// \brief Combo-box item role carrying a unit length in milliseconds.
constexpr int kUnitMsRole = Qt::UserRole;

constexpr qint64 kMinuteMs = 60000;
constexpr qint64 kHourMs = 3600000;
constexpr qint64 kDayMs = 86400000;

} // namespace

///
/// \brief Builds the dialog and wires its controls.
/// \param parent Parent widget.
///
CustomIntervalDialog::CustomIntervalDialog(QWidget *parent)
    : AppBaseDialog(parent)
    , ui(new Ui::CustomIntervalDialog)
{
    ui->setupUi(this);
    applyStyling();

    ui->unitCombo->addItem(tr("minutes"), QVariant::fromValue(kMinuteMs));
    ui->unitCombo->addItem(tr("hours"), QVariant::fromValue(kHourMs));
    ui->unitCombo->addItem(tr("days"), QVariant::fromValue(kDayMs));
    ui->unitCombo->setCurrentIndex(1);

    ui->fromToRadio->setChecked(true);
    updateEnabledState();

    if (QPushButton *applyButton = ui->buttonBox->button(QDialogButtonBox::Apply))
        applyButton->setDefault(true);

    connect(ui->fromToRadio, &QAbstractButton::toggled,
            this, &CustomIntervalDialog::updateEnabledState);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    if (QPushButton *applyButton = ui->buttonBox->button(QDialogButtonBox::Apply)) {
        connect(applyButton, &QPushButton::clicked,
                this, &CustomIntervalDialog::validateAndAccept);
    }
}

///
/// \brief Applies the theme-aware grouping-frame styling.
///
void CustomIntervalDialog::applyStyling()
{
    ui->fromToFrame->setStyleSheet(
        QStringLiteral("#fromToFrame { border: 1px solid %1; border-radius: 8px; }")
            .arg(AppColors::noticeNeutralBorder().name()));
}

///
/// \brief Destroys the dialog and its generated UI.
///
CustomIntervalDialog::~CustomIntervalDialog()
{
    delete ui;
}

///
/// \brief Seeds the From/To fields and the last-duration amount.
/// \param start Interval start used to fill the From fields.
/// \param end Interval end used to fill the To fields.
///
void CustomIntervalDialog::setInterval(const QDateTime &start, const QDateTime &end)
{
    ui->fromDateEdit->setDate(start.date());
    ui->fromTimeEdit->setTime(start.time());
    ui->toDateEdit->setDate(end.date());
    ui->toTimeEdit->setTime(end.time());
}

///
/// \brief Returns the resolved interval start.
/// \return Start of the chosen range.
///
QDateTime CustomIntervalDialog::fromDateTime() const
{
    if (!isRelative())
        return QDateTime(ui->fromDateEdit->date(), ui->fromTimeEdit->time());

    const qint64 unitMs = ui->unitCombo->currentData(kUnitMsRole).toLongLong();
    const qint64 spanMs = static_cast<qint64>(ui->lastAmountSpin->value()) * unitMs;
    return toDateTime().addMSecs(-spanMs);
}

///
/// \brief Returns the resolved interval end.
/// \return End of the chosen range.
///
QDateTime CustomIntervalDialog::toDateTime() const
{
    if (isRelative())
        return QDateTime::currentDateTime();
    return QDateTime(ui->toDateEdit->date(), ui->toTimeEdit->time());
}

///
/// \brief Reports whether the range is a rolling "last N units" duration.
/// \return True for the "last" choice, false for an absolute From/To range.
///
bool CustomIntervalDialog::isRelative() const
{
    return ui->lastRadio->isChecked();
}

///
/// \brief Enables the field group matching the selected choice.
///
void CustomIntervalDialog::updateEnabledState()
{
    const bool absolute = ui->fromToRadio->isChecked();
    ui->fromDateEdit->setEnabled(absolute);
    ui->fromTimeEdit->setEnabled(absolute);
    ui->toDateEdit->setEnabled(absolute);
    ui->toTimeEdit->setEnabled(absolute);
    ui->lastAmountSpin->setEnabled(!absolute);
    ui->unitCombo->setEnabled(!absolute);
}

///
/// \brief Rejects an empty or inverted From/To range before accepting.
///
void CustomIntervalDialog::validateAndAccept()
{
    if (!isRelative() && fromDateTime() >= toDateTime()) {
        MessageBoxDialog::warning(this, tr("Custom interval"),
                                  tr("The 'From' time must be earlier than the 'To' time."),
                                  DialogButtonBox::Ok);
        return;
    }
    accept();
}
