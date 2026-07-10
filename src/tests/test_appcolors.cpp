// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_appcolors.cpp
/// \brief Tests the palette-aware helpers in AppColors.
///

#include <QApplication>
#include <QTest>

#include "appcolors.h"

///
/// \brief Verifies foreground selection against varying backgrounds.
///
class TestAppColors : public QObject
{
    Q_OBJECT

private slots:
    void mostLegiblePicksTheForegroundFurthestInLightness();
    void mostLegibleKeepsThePreferredForegroundOnATie();
};

void TestAppColors::mostLegiblePicksTheForegroundFurthestInLightness()
{
    const QColor white(Qt::white);
    const QColor black(Qt::black);

    QCOMPARE(AppColors::mostLegible(QColor(0x102030), white, black), white);
    QCOMPARE(AppColors::mostLegible(QColor(0xf1f5f9), white, black), black);
}

void TestAppColors::mostLegibleKeepsThePreferredForegroundOnATie()
{
    const QColor first(0x40, 0x40, 0x40);
    const QColor second(0xc0, 0xc0, 0xc0);
    const QColor background(0x80, 0x80, 0x80);

    QCOMPARE(AppColors::mostLegible(background, first, second).lightness(), first.lightness());
}

QTEST_MAIN(TestAppColors)

#include "test_appcolors.moc"
