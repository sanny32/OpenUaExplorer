// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file datetimeedit.h
/// \brief Declares a date-time edit.
///

#pragma once

#include <QDateTimeEdit>

///
/// \brief Date-time edit that installs the application calendar popup.
///
class DateTimeEdit : public QDateTimeEdit
{
    Q_OBJECT

public:
    ///
    /// \brief Builds a date-time edit with the application calendar popup.
    /// \param parent Parent widget.
    ///
    explicit DateTimeEdit(QWidget *parent = nullptr);
};
