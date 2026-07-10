// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file calendarwidget.cpp
/// \brief Implements a calendar widget.
///

#include <algorithm>

#include <QEvent>
#include <QFontMetrics>
#include <QHeaderView>
#include <QIcon>
#include <QLocale>
#include <QSize>
#include <QStyle>
#include <QTableView>
#include <QToolButton>

#include "application.h"
#include "calendarwidget.h"

namespace {

///
/// \brief Builds a themed navigation icon with a style fallback for test binaries.
/// \param name Icon resource base name.
/// \param fallback Fallback standard icon.
/// \param widget Widget whose style provides the fallback.
/// \return Navigation icon.
///
QIcon navigationIcon(const QString &name, QStyle::StandardPixmap fallback, const QWidget *widget)
{
    auto *app = qobject_cast<Application *>(qApp);
    const QString theme = app != nullptr && app->theme().isDark()
        ? QStringLiteral("dark") : QStringLiteral("light");
    QIcon icon(QStringLiteral(":/icons/%1/%2.svg").arg(theme, name));
    if (icon.isNull())
        icon = widget->style()->standardIcon(fallback, nullptr, widget);
    return icon;
}

} // namespace

///
/// \brief Builds a calendar with a locale-aware minimum width.
/// \param parent Parent widget.
///
CalendarWidget::CalendarWidget(QWidget *parent)
    : QCalendarWidget(parent)
{
    setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    updateMinimumPopupWidth();
    refreshNavigationIcons();

    if (auto *app = qobject_cast<Application *>(qApp)) {
        connect(&app->theme(), &AppTheme::colorSchemeChanged,
                this, &CalendarWidget::refreshNavigationIcons);
    }
}

///
/// \brief Returns the minimum popup width for the current font and locale.
/// \return Minimum popup width in pixels.
///
int CalendarWidget::minimumPopupWidthHint() const
{
    const QFontMetrics metrics(font());
    const QLocale currentLocale = locale();
    int weekdayWidth = 0;
    for (int day = Qt::Monday; day <= Qt::Sunday; ++day) {
        weekdayWidth = std::max(
            weekdayWidth,
            metrics.horizontalAdvance(currentLocale.standaloneDayName(day, QLocale::ShortFormat)));
    }

    const int dayColumnWidth = weekdayWidth + 24;
    const int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, nullptr, this);
    return dayColumnWidth * 7 + frameWidth * 2 + 16;
}

///
/// \brief Recomputes the minimum width when visual inputs change.
/// \param event Change event.
///
void CalendarWidget::changeEvent(QEvent *event)
{
    QCalendarWidget::changeEvent(event);
    switch (event->type()) {
    case QEvent::FontChange:
    case QEvent::LanguageChange:
    case QEvent::LocaleChange:
    case QEvent::PaletteChange:
    case QEvent::StyleChange:
        updateMinimumPopupWidth();
        refreshNavigationIcons();
        break;
    default:
        break;
    }
}

///
/// \brief Applies application icons to the month navigation buttons.
///
void CalendarWidget::refreshNavigationIcons()
{
    auto *previousButton = findChild<QToolButton *>(QStringLiteral("qt_calendar_prevmonth"));
    auto *nextButton = findChild<QToolButton *>(QStringLiteral("qt_calendar_nextmonth"));
    if (previousButton) {
        previousButton->setIcon(navigationIcon(
            QStringLiteral("chevron-left"), QStyle::SP_ArrowLeft, this));
        previousButton->setIconSize(QSize(16, 16));
    }
    if (nextButton) {
        nextButton->setIcon(navigationIcon(
            QStringLiteral("chevron-right"), QStyle::SP_ArrowRight, this));
        nextButton->setIconSize(QSize(16, 16));
    }
}

///
/// \brief Applies the minimum popup width for the current font and locale.
///
void CalendarWidget::updateMinimumPopupWidth()
{
    if (auto *calendarView = findChild<QTableView *>()) {
        const QFontMetrics metrics(font());
        int weekdayWidth = 0;
        for (int day = Qt::Monday; day <= Qt::Sunday; ++day) {
            weekdayWidth = std::max(
                weekdayWidth,
                metrics.horizontalAdvance(locale().standaloneDayName(day, QLocale::ShortFormat)));
        }

        calendarView->horizontalHeader()->setMinimumSectionSize(weekdayWidth + 24);
    }
    setMinimumWidth(minimumPopupWidthHint());
}
