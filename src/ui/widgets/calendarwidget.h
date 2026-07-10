// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file calendarwidget.h
/// \brief Declares a calendar widget.
///

#pragma once

#include <QCalendarWidget>

///
/// \brief Calendar widget that reserves enough width for localized weekday labels.
///
class CalendarWidget : public QCalendarWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Builds a calendar with a locale-aware minimum width.
    /// \param parent Parent widget.
    ///
    explicit CalendarWidget(QWidget *parent = nullptr);

    ///
    /// \brief Returns the minimum popup width for the current font and locale.
    /// \return Minimum popup width in pixels.
    ///
    int minimumPopupWidthHint() const;

protected:
    void changeEvent(QEvent *event) override;

private:
    void refreshNavigationIcons();
    void updateMinimumPopupWidth();
};
