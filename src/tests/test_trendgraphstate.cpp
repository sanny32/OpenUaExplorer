// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_trendgraphstate.cpp
/// \brief Tests the non-visual trend graph state helper.
///

#include <QSet>
#include <QTest>

#include "widgets/trendgraphstate.h"

class TestTrendGraphState : public QObject
{
    Q_OBJECT

private slots:
    void subscriptionsAreIdempotent();
    void pendingHistoryIsConsumedOnce();
};

///
/// \brief Live subscription tracking reports only state changes.
///
void TestTrendGraphState::subscriptionsAreIdempotent()
{
    TrendGraphState state;
    const QString node = QStringLiteral("ns=2;s=Temperature");

    QVERIFY(!state.isSubscribed(node));
    QVERIFY(state.subscribe(node));
    QVERIFY(state.isSubscribed(node));
    QVERIFY(!state.subscribe(node));
    QCOMPARE(state.subscribedNodes(), QSet<QString>({node}));

    QVERIFY(state.unsubscribe(node));
    QVERIFY(!state.isSubscribed(node));
    QVERIFY(!state.unsubscribe(node));

    state.subscribe(node);
    state.clearSubscriptions();
    QVERIFY(!state.isSubscribed(node));
}

///
/// \brief Pending history markers are consumed once and can be cleared.
///
void TestTrendGraphState::pendingHistoryIsConsumedOnce()
{
    TrendGraphState state;
    const QString node = QStringLiteral("ns=2;s=Temperature");

    QVERIFY(!state.consumePendingHistory(node));
    state.addPendingHistory(node);
    QVERIFY(state.consumePendingHistory(node));
    QVERIFY(!state.consumePendingHistory(node));

    state.addPendingHistory(node);
    state.removePendingHistory(node);
    QVERIFY(!state.consumePendingHistory(node));

    state.addPendingHistory(node);
    state.clearPendingHistory();
    QVERIFY(!state.consumePendingHistory(node));
}

QTEST_MAIN(TestTrendGraphState)

#include "test_trendgraphstate.moc"
