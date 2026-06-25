// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file datetimeedit.cpp
/// \brief Implements a date-time edit that uses the application calendar popup.
///

#include "datetimeedit.h"

#include "calendarwidget.h"

///
/// \brief Builds a date-time edit with the application calendar popup.
/// \param parent Parent widget.
///
DateTimeEdit::DateTimeEdit(QWidget *parent)
    : QDateTimeEdit(parent)
{
    setCalendarPopup(true);
    setCalendarWidget(new CalendarWidget(this));
}
