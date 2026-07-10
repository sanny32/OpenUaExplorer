// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file customintervaldialog.h
/// \brief Declares the dialog that picks a custom trend history interval.
///

#pragma once

#include <QDateTime>

#include "appbasedialog.h"

namespace Ui {
class CustomIntervalDialog;
}

///
/// \brief Lets the user choose a trend history interval as an absolute From/To
/// range or as a rolling "last N units" duration.
///
/// Both choices resolve to a concrete start/end pair through fromDateTime() and
/// toDateTime(); the "last" choice is anchored to the current time when read. Use
/// isRelative() to tell whether the range should follow "now" on later refreshes.
///
class CustomIntervalDialog : public AppBaseDialog
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the dialog and wires its controls.
    /// \param parent Parent widget.
    ///
    explicit CustomIntervalDialog(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the dialog and its generated UI.
    ///
    ~CustomIntervalDialog() override;

    ///
    /// \brief Seeds the From/To fields and the last-duration amount.
    /// \param start Interval start used to fill the From fields.
    /// \param end Interval end used to fill the To fields.
    ///
    void setInterval(const QDateTime &start, const QDateTime &end);

    ///
    /// \brief Returns the resolved interval start.
    /// \return Start of the chosen range.
    ///
    QDateTime fromDateTime() const;

    ///
    /// \brief Returns the resolved interval end.
    /// \return End of the chosen range.
    ///
    QDateTime toDateTime() const;

    ///
    /// \brief Reports whether the range is a rolling "last N units" duration.
    /// \return True for the "last" choice, false for an absolute From/To range.
    ///
    bool isRelative() const;

private slots:
    void updateEnabledState();
    void validateAndAccept();

private:
    void applyStyling();

    Ui::CustomIntervalDialog *ui;
};
