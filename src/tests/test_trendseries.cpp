// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_trendseries.cpp
/// \brief Tests numeric coercion and buffering in TrendSeries.
///

#include <QDateTime>
#include <QTest>

#include "models/trendseries.h"
#include "opcua/opcuatypes.h"

namespace {

///
/// \brief Builds a streamed value with a numeric or non-numeric payload.
/// \param value Value payload.
/// \param msec Source timestamp in milliseconds since the epoch.
/// \return Data value item.
///
OpcUaDataValue makeValue(const QVariant &value, qint64 msec, const QString &status = QString())
{
    OpcUaDataValue item;
    item.nodeId = QStringLiteral("ns=2;s=Demo");
    item.value = value;
    item.sourceTimestamp = QDateTime::fromMSecsSinceEpoch(msec);
    item.status = status;
    return item;
}

} // namespace

///
/// \brief Verifies coercion, live appends, history replacement, and trimming.
///
class TestTrendSeries : public QObject
{
    Q_OBJECT

private slots:
    void coercesNumericTypes();
    void rejectsNonNumeric();
    void appendsLiveSamples();
    void replacesHistory();
    void trimsToMaxPoints();
    void keepsStatusAlignedWithPoints();
    void labelPrefersPathThenNameThenNodeId();
    void seriesLabelHonoursModeWithFallbacks();
    void colorAndVisibilityRoundTrip();
    void appendLiveWithoutTimestampUsesNow();
    void setHistorySkipsUndatedSamples();
    void backfillHistoryKeepsNewerLivePoints();
    void backfillHistoryWithoutSamplesIsNoOp();
    void clearRemovesAllPoints();
};

namespace {

OpcUaHistoryValue makeHistory(const QVariant &value, qint64 msec, bool dated = true)
{
    OpcUaHistoryValue item;
    item.value = value;
    if (dated)
        item.sourceTimestamp = QDateTime::fromMSecsSinceEpoch(msec);
    return item;
}

} // namespace

///
/// \brief Numeric and boolean values coerce to doubles.
///
void TestTrendSeries::coercesNumericTypes()
{
    double out = 0.0;
    QVERIFY(TrendSeries::toNumeric(QVariant(42), &out));
    QCOMPARE(out, 42.0);
    QVERIFY(TrendSeries::toNumeric(QVariant(3.5), &out));
    QCOMPARE(out, 3.5);
    QVERIFY(TrendSeries::toNumeric(QVariant(true), &out));
    QCOMPARE(out, 1.0);
}

///
/// \brief Strings and other non-numeric values are rejected.
///
void TestTrendSeries::rejectsNonNumeric()
{
    double out = 0.0;
    QVERIFY(!TrendSeries::toNumeric(QVariant(QStringLiteral("12")), &out));
    QVERIFY(!TrendSeries::toNumeric(QVariant(QDateTime::currentDateTime()), &out));
}

///
/// \brief Live appends add numeric points and skip non-numeric ones.
///
void TestTrendSeries::appendsLiveSamples()
{
    TrendSeries series(QStringLiteral("ns=2;s=Demo"), QStringLiteral("Demo"), QString());
    QVERIFY(series.appendLive(makeValue(QVariant(1.0), 1000)));
    QVERIFY(series.appendLive(makeValue(QVariant(2.0), 2000)));
    QVERIFY(!series.appendLive(makeValue(QVariant(QStringLiteral("x")), 3000)));

    QCOMPARE(series.points().size(), 2);
    QCOMPARE(series.points().at(0).x(), 1000.0);
    QCOMPARE(series.points().at(1).y(), 2.0);
}

///
/// \brief History replacement drops non-numeric and timestamp-less samples.
///
void TestTrendSeries::replacesHistory()
{
    TrendSeries series(QStringLiteral("ns=2;s=Demo"), QStringLiteral("Demo"), QString());
    series.appendLive(makeValue(QVariant(9.0), 500));

    QVector<OpcUaHistoryValue> history;
    OpcUaHistoryValue a;
    a.value = QVariant(5.0);
    a.sourceTimestamp = QDateTime::fromMSecsSinceEpoch(1000);
    OpcUaHistoryValue b;
    b.value = QVariant(QStringLiteral("nope"));
    b.sourceTimestamp = QDateTime::fromMSecsSinceEpoch(2000);
    history << a << b;

    series.setHistory(history);
    QCOMPARE(series.points().size(), 1);
    QCOMPARE(series.points().at(0).y(), 5.0);
}

///
/// \brief Appending past the cap drops the oldest points.
///
void TestTrendSeries::trimsToMaxPoints()
{
    TrendSeries series(QStringLiteral("ns=2;s=Demo"), QStringLiteral("Demo"), QString());
    series.setMaxPoints(3);
    for (int i = 0; i < 5; ++i)
        series.appendLive(makeValue(QVariant(double(i)), 1000 + i));

    QCOMPARE(series.points().size(), 3);
    QCOMPARE(series.points().first().y(), 2.0);
    QCOMPARE(series.points().last().y(), 4.0);
}

///
/// \brief Status text stays aligned with points through appends and trimming.
///
void TestTrendSeries::keepsStatusAlignedWithPoints()
{
    TrendSeries series(QStringLiteral("ns=2;s=Demo"), QStringLiteral("Demo"), QString());
    series.setMaxPoints(2);
    series.appendLive(makeValue(QVariant(1.0), 1000, QStringLiteral("Good")));
    series.appendLive(makeValue(QVariant(2.0), 2000, QStringLiteral("Uncertain")));
    series.appendLive(makeValue(QVariant(3.0), 3000, QStringLiteral("Bad")));

    QCOMPARE(series.points().size(), 2);
    QCOMPARE(series.statuses().size(), 2);
    QCOMPARE(series.points().at(0).y(), 2.0);
    QCOMPARE(series.statuses().at(0), QStringLiteral("Uncertain"));
    QCOMPARE(series.statuses().at(1), QStringLiteral("Bad"));
}

///
/// \brief label() prefers the path, then the display name, then the NodeId.
///
void TestTrendSeries::labelPrefersPathThenNameThenNodeId()
{
    QCOMPARE(TrendSeries(QStringLiteral("id"), QStringLiteral("Name"),
                         QStringLiteral("Path")).label(),
             QStringLiteral("Path"));
    QCOMPARE(TrendSeries(QStringLiteral("id"), QStringLiteral("Name"), QString()).label(),
             QStringLiteral("Name"));
    QCOMPARE(TrendSeries(QStringLiteral("id"), QString(), QString()).label(),
             QStringLiteral("id"));
}

///
/// \brief seriesLabel() honours the requested mode and falls back sensibly.
///
void TestTrendSeries::seriesLabelHonoursModeWithFallbacks()
{
    const TrendSeries full(QStringLiteral("id"), QStringLiteral("Name"), QStringLiteral("Path"));
    QCOMPARE(full.seriesLabel(TrendLabelMode::NodeId), QStringLiteral("id"));
    QCOMPARE(full.seriesLabel(TrendLabelMode::Path), QStringLiteral("Path"));
    QCOMPARE(full.seriesLabel(TrendLabelMode::DisplayName), QStringLiteral("Name"));

    const TrendSeries noPath(QStringLiteral("id"), QStringLiteral("Name"), QString());
    QCOMPARE(noPath.seriesLabel(TrendLabelMode::Path), QStringLiteral("Name"));

    const TrendSeries idOnly(QStringLiteral("id"), QString(), QString());
    QCOMPARE(idOnly.seriesLabel(TrendLabelMode::Path), QStringLiteral("id"));
    QCOMPARE(idOnly.seriesLabel(TrendLabelMode::DisplayName), QStringLiteral("id"));
}

///
/// \brief Colour and visibility accessors round-trip.
///
void TestTrendSeries::colorAndVisibilityRoundTrip()
{
    TrendSeries series(QStringLiteral("id"), QString(), QString());
    QVERIFY(!series.color().isValid());
    series.setColor(QColor(Qt::red));
    QCOMPARE(series.color(), QColor(Qt::red));

    QVERIFY(series.isVisible());
    series.setVisible(false);
    QVERIFY(!series.isVisible());
}

///
/// \brief A value without any timestamp is stamped with the current time.
///
void TestTrendSeries::appendLiveWithoutTimestampUsesNow()
{
    TrendSeries series(QStringLiteral("id"), QString(), QString());
    const qint64 before = QDateTime::currentMSecsSinceEpoch();
    OpcUaDataValue value;
    value.value = QVariant(7.0); // numeric but no source/server timestamp
    QVERIFY(series.appendLive(value));
    QCOMPARE(series.points().size(), 1);
    QVERIFY(series.points().first().x() >= static_cast<qreal>(before));
}

///
/// \brief History samples without a valid timestamp are dropped.
///
void TestTrendSeries::setHistorySkipsUndatedSamples()
{
    TrendSeries series(QStringLiteral("id"), QString(), QString());
    QVector<OpcUaHistoryValue> history;
    history << makeHistory(QVariant(1.0), 1000)
            << makeHistory(QVariant(2.0), 0, /*dated=*/false);
    series.setHistory(history);
    QCOMPARE(series.points().size(), 1);
    QCOMPARE(series.points().first().y(), 1.0);
}

///
/// \brief backfillHistory prepends history and keeps live points newer than it.
///
void TestTrendSeries::backfillHistoryKeepsNewerLivePoints()
{
    TrendSeries series(QStringLiteral("id"), QString(), QString());
    series.appendLive(makeValue(QVariant(10.0), 1000)); // older than history end
    series.appendLive(makeValue(QVariant(30.0), 3000)); // newer than history end

    QVector<OpcUaHistoryValue> history;
    history << makeHistory(QVariant(5.0), 500) << makeHistory(QVariant(20.0), 2000);
    series.backfillHistory(history);

    QCOMPARE(series.points().size(), 3);
    QCOMPARE(series.points().at(0).x(), 500.0);
    QCOMPARE(series.points().at(1).x(), 2000.0);
    QCOMPARE(series.points().at(2).x(), 3000.0); // live point past the boundary kept
}

///
/// \brief backfillHistory with no usable samples leaves the buffer untouched.
///
void TestTrendSeries::backfillHistoryWithoutSamplesIsNoOp()
{
    TrendSeries series(QStringLiteral("id"), QString(), QString());
    series.appendLive(makeValue(QVariant(1.0), 1000));
    series.backfillHistory({});
    QCOMPARE(series.points().size(), 1);
    QCOMPARE(series.points().first().y(), 1.0);
}

///
/// \brief clear() empties both points and statuses.
///
void TestTrendSeries::clearRemovesAllPoints()
{
    TrendSeries series(QStringLiteral("id"), QString(), QString());
    series.appendLive(makeValue(QVariant(1.0), 1000, QStringLiteral("Good")));
    series.clear();
    QVERIFY(series.points().isEmpty());
    QVERIFY(series.statuses().isEmpty());
}

QTEST_MAIN(TestTrendSeries)

#include "test_trendseries.moc"
