// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_durationformatter.cpp
/// \brief Unit tests for shared duration formatting helpers.
///

#include <QTest>

#include "formatters/durationformatter.h"

///
/// \brief Tests compact duration labels used by UI surfaces.
///
class TestDurationFormatter : public QObject
{
    Q_OBJECT

private slots:
    void formatsMillisecondsAsLargestUnits();
};

///
/// \brief Verifies the formatter keeps at most two non-zero units.
///
void TestDurationFormatter::formatsMillisecondsAsLargestUnits()
{
    QCOMPARE(formatDuration(-1000), QStringLiteral("0 s"));
    QCOMPARE(formatDuration(0), QStringLiteral("0 s"));
    QCOMPARE(formatDuration(5000), QStringLiteral("5 s"));
    QCOMPARE(formatDuration(600000), QStringLiteral("10 min"));
    QCOMPARE(formatDuration(3900000), QStringLiteral("1 h 5 min"));
    QCOMPARE(formatDuration(187200000), QStringLiteral("2 d 4 h"));
}

QTEST_MAIN(TestDurationFormatter)

#include "test_durationformatter.moc"
