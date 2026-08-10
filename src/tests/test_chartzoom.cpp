// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_chartzoom.cpp
/// \brief Tests the mouse wheel zoom arithmetic shared by the chart hosts.
///

#include <QTest>

#include "chartzoom.h"

///
/// \brief Verifies wheel factors, anchored scaling and span clamping.
///
class TestChartZoom : public QObject
{
    Q_OBJECT

private slots:
    void wheelFactorFollowsRotation();
    void scalingKeepsAnchorInPlace();
    void scalingClampsSpan();
    void scalingRejectsUnusableInput();
};

///
/// \brief A forward notch zooms in, a backward notch zooms out symmetrically.
///
void TestChartZoom::wheelFactorFollowsRotation()
{
    QCOMPARE(ChartZoom::factorFromWheel(0), 1.0);

    const qreal zoomIn = ChartZoom::factorFromWheel(120);
    const qreal zoomOut = ChartZoom::factorFromWheel(-120);
    QVERIFY(zoomIn < 1.0);
    QVERIFY(zoomOut > 1.0);
    QVERIFY(qFuzzyCompare(zoomIn * zoomOut, 1.0));

    QVERIFY(ChartZoom::factorFromWheel(60) > zoomIn);
    QVERIFY(ChartZoom::factorFromWheel(240) < zoomIn);
}

///
/// \brief The anchor keeps its relative position while the span scales.
///
void TestChartZoom::scalingKeepsAnchorInPlace()
{
    ChartRange range{ 0.0, 100.0 };
    QVERIFY(ChartZoom::scaleRange(&range, 25.0, 0.5, 0.0, 0.0));
    QCOMPARE(range.span(), 50.0);
    QCOMPARE(range.min, 12.5);
    QCOMPARE(range.max, 62.5);

    ChartRange edge{ 0.0, 100.0 };
    QVERIFY(ChartZoom::scaleRange(&edge, 100.0, 0.5, 0.0, 0.0));
    QCOMPARE(edge.max, 100.0);
    QCOMPARE(edge.min, 50.0);

    ChartRange outside{ 0.0, 100.0 };
    QVERIFY(ChartZoom::scaleRange(&outside, 500.0, 0.5, 0.0, 0.0));
    QCOMPARE(outside.max, 500.0);
}

///
/// \brief The scaled span stays inside the allowed limits.
///
void TestChartZoom::scalingClampsSpan()
{
    ChartRange narrow{ 0.0, 100.0 };
    QVERIFY(ChartZoom::scaleRange(&narrow, 50.0, 0.01, 10.0, 0.0));
    QCOMPARE(narrow.span(), 10.0);

    ChartRange wide{ 0.0, 100.0 };
    QVERIFY(ChartZoom::scaleRange(&wide, 50.0, 100.0, 10.0, 400.0));
    QCOMPARE(wide.span(), 400.0);

    ChartRange atLimit{ 0.0, 10.0 };
    QVERIFY(!ChartZoom::scaleRange(&atLimit, 5.0, 0.5, 10.0, 0.0));
    QCOMPARE(atLimit.span(), 10.0);
}

///
/// \brief A missing, empty or inverted range and a non-positive factor are refused.
///
void TestChartZoom::scalingRejectsUnusableInput()
{
    QVERIFY(!ChartZoom::scaleRange(nullptr, 0.0, 0.5, 0.0, 0.0));

    ChartRange empty{ 5.0, 5.0 };
    QVERIFY(!ChartZoom::scaleRange(&empty, 5.0, 0.5, 0.0, 0.0));

    ChartRange inverted{ 10.0, 0.0 };
    QVERIFY(!ChartZoom::scaleRange(&inverted, 5.0, 0.5, 0.0, 0.0));

    ChartRange range{ 0.0, 100.0 };
    QVERIFY(!ChartZoom::scaleRange(&range, 50.0, 0.0, 0.0, 0.0));
    QCOMPARE(range.span(), 100.0);
}

QTEST_GUILESS_MAIN(TestChartZoom)

#include "test_chartzoom.moc"
