// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_dataaccessmonitoringstate.cpp
/// \brief Tests data-access monitoring state transitions.
///

#include <QTest>

#include "dataaccessmonitoringstate.h"

class TestDataAccessMonitoringState : public QObject
{
    Q_OBJECT

private slots:
    void successfulRequestsUpdateSubscriptionState();
    void failedRequestsOnlyClearPendingState();
};

///
/// \brief Successful monitoring requests update subscribed and pending state.
///
void TestDataAccessMonitoringState::successfulRequestsUpdateSubscriptionState()
{
    DataAccessMonitoringState state;
    const QString node = QStringLiteral("ns=2;s=Temperature");

    state.beginRequest(node);
    QVERIFY(state.isPending(node));
    state.finishRequest(node, true, true);
    QVERIFY(!state.isPending(node));
    QVERIFY(state.isSubscribed(node));

    state.beginRequest(node);
    state.finishRequest(node, false, true);
    QVERIFY(!state.isPending(node));
    QVERIFY(!state.isSubscribed(node));
}

///
/// \brief Failed monitoring requests leave the previous subscription state intact.
///
void TestDataAccessMonitoringState::failedRequestsOnlyClearPendingState()
{
    DataAccessMonitoringState state;
    const QString node = QStringLiteral("ns=2;s=Temperature");

    state.beginRequest(node);
    state.finishRequest(node, true, false);
    QVERIFY(!state.isPending(node));
    QVERIFY(!state.isSubscribed(node));

    state.beginRequest(node);
    state.finishRequest(node, true, true);
    state.beginRequest(node);
    state.finishRequest(node, false, false);
    QVERIFY(!state.isPending(node));
    QVERIFY(state.isSubscribed(node));

    state.clear();
    QVERIFY(!state.isSubscribed(node));
}

QTEST_MAIN(TestDataAccessMonitoringState)

#include "test_dataaccessmonitoringstate.moc"
