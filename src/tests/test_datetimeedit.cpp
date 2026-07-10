// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_datetimeedit.cpp
/// \brief Tests the date-time edit and calendar widgets.
///

#include <QFontMetrics>
#include <QHeaderView>
#include <QLocale>
#include <QTableView>
#include <QTest>
#include <QToolButton>

#include <algorithm>

#include "calendarwidget.h"
#include "datetimeedit.h"

///
/// \brief UI tests for the date-time edit and calendar.
///
class TestDateTimeEdit : public QObject
{
    Q_OBJECT

private slots:
    void dateTimeEditInstallsCalendar();
    void calendarFitsLocalizedWeekdayLabels();
    void calendarUsesApplicationNavigationIcons();
};

namespace {

///
/// \brief Returns the widest localized short weekday label.
/// \param calendar Calendar whose font and locale are used.
/// \return Weekday label width in pixels.
///
int widestWeekdayLabel(const CalendarWidget &calendar)
{
    const QFontMetrics metrics(calendar.font());
    int width = 0;
    for (int day = Qt::Monday; day <= Qt::Sunday; ++day) {
        width = std::max(
            width,
            metrics.horizontalAdvance(calendar.locale().standaloneDayName(day, QLocale::ShortFormat)));
    }
    return width;
}

} // namespace

///
/// \brief The date-time edit uses the application calendar popup.
///
void TestDateTimeEdit::dateTimeEditInstallsCalendar()
{
    DateTimeEdit edit;

    QVERIFY(edit.calendarPopup());
    QVERIFY(qobject_cast<CalendarWidget *>(edit.calendarWidget()));
}

///
/// \brief The calendar reserves space for localized weekday labels.
///
void TestDateTimeEdit::calendarFitsLocalizedWeekdayLabels()
{
    CalendarWidget calendar;
    calendar.setLocale(QLocale(QLocale::Russian, QLocale::Russia));

    QVERIFY(calendar.minimumWidth() >= calendar.minimumPopupWidthHint());
    QCOMPARE(calendar.verticalHeaderFormat(), QCalendarWidget::NoVerticalHeader);

    auto *calendarView = calendar.findChild<QTableView *>();
    QVERIFY(calendarView);
    QVERIFY(calendarView->horizontalHeader()->minimumSectionSize() >= widestWeekdayLabel(calendar));
    QVERIFY(calendarView->verticalHeader()->isHidden());
}

///
/// \brief The calendar replaces Qt's month navigation icons.
///
void TestDateTimeEdit::calendarUsesApplicationNavigationIcons()
{
    CalendarWidget calendar;

    auto *previousButton = calendar.findChild<QToolButton *>(QStringLiteral("qt_calendar_prevmonth"));
    auto *nextButton = calendar.findChild<QToolButton *>(QStringLiteral("qt_calendar_nextmonth"));
    QVERIFY(previousButton);
    QVERIFY(nextButton);
    QVERIFY(!previousButton->icon().isNull());
    QVERIFY(!nextButton->icon().isNull());
    QCOMPARE(previousButton->iconSize(), QSize(16, 16));
    QCOMPARE(nextButton->iconSize(), QSize(16, 16));
}

QTEST_MAIN(TestDateTimeEdit)

#include "test_datetimeedit.moc"
