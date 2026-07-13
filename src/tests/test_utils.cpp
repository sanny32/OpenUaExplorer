// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_utils.cpp
/// \brief Tests the shared file-name helpers.
///

#include <QDateTime>
#include <QTest>
#include <QTimeZone>

#include "utils.h"

class TestUtils : public QObject
{
    Q_OBJECT

private slots:
    void fileNameSegmentSanitizesIllegalCharacters();
    void fileNameSegmentFallsBackWhenEmpty();
    void fileNameDateTimeIsCompactAndSortable();
};

///
/// \brief Characters illegal in file names and runs of whitespace collapse into underscores.
///
void TestUtils::fileNameSegmentSanitizesIllegalCharacters()
{
    QCOMPARE(Utils::fileNameSegment(QStringLiteral("Area 1/Device:A"), QStringLiteral("x")),
             QStringLiteral("Area_1_Device_A"));
    QCOMPARE(Utils::fileNameSegment(QStringLiteral("a<b>c|d?e*f\"g"), QStringLiteral("x")),
             QStringLiteral("a_b_c_d_e_f_g"));
    QCOMPARE(Utils::fileNameSegment(QStringLiteral("many   spaces"), QStringLiteral("x")),
             QStringLiteral("many_spaces"));
}

///
/// \brief A segment that sanitizes away, or trails in dots and spaces, resolves to the fallback.
///
void TestUtils::fileNameSegmentFallsBackWhenEmpty()
{
    const QString fallback = QStringLiteral("history");

    QCOMPARE(Utils::fileNameSegment(QString(), fallback), fallback);
    QCOMPARE(Utils::fileNameSegment(QStringLiteral("   "), fallback), fallback);
    QCOMPARE(Utils::fileNameSegment(QStringLiteral("name..."), fallback), QStringLiteral("name"));
}

///
/// \brief The stamp is file-name safe and orders chronologically as plain text.
///
void TestUtils::fileNameDateTimeIsCompactAndSortable()
{
    const QDateTime early(QDate(2026, 7, 13), QTime(8, 5, 1), QTimeZone::UTC);
    const QDateTime late(QDate(2026, 7, 13), QTime(19, 30, 0), QTimeZone::UTC);

    QCOMPARE(Utils::fileNameDateTime(early), QStringLiteral("20260713_080501"));
    QVERIFY(Utils::fileNameDateTime(early) < Utils::fileNameDateTime(late));
}

QTEST_MAIN(TestUtils)

#include "test_utils.moc"
