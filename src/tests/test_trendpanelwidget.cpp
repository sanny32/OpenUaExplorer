// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_trendpanelwidget.cpp
/// \brief Tests TrendPanelWidget mode switching and data routing.
///

#include <QAbstractButton>
#include <QDateTime>
#include <QSignalSpy>
#include <QTest>

#include "opcua/opcuatypes.h"
#include "widgets/trendpanelwidget.h"

namespace {

constexpr auto kNodeId = "ns=2;s=Demo";

} // namespace

///
/// \brief Verifies subscribe/history requests and history routing.
///
class TestTrendPanelWidget : public QObject
{
    Q_OBJECT

private slots:
    void addingNodeInLiveModeSubscribes();
    void switchingToHistoryModeReadsHistory();
    void consumeHistoryMatchesPendingNode();
};

///
/// \brief Adding a node while live requests a subscription.
///
void TestTrendPanelWidget::addingNodeInLiveModeSubscribes()
{
    TrendPanelWidget panel;
    QSignalSpy spy(&panel, &TrendPanelWidget::subscribeRequested);

    panel.addNode(QString::fromLatin1(kNodeId), QStringLiteral("Demo"));

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(0).toString(), QString::fromLatin1(kNodeId));
}

///
/// \brief Choosing a range button unsubscribes and reads history.
///
void TestTrendPanelWidget::switchingToHistoryModeReadsHistory()
{
    TrendPanelWidget panel;
    panel.addNode(QString::fromLatin1(kNodeId), QStringLiteral("Demo"));

    QSignalSpy unsubscribeSpy(&panel, &TrendPanelWidget::unsubscribeRequested);
    QSignalSpy historySpy(&panel, &TrendPanelWidget::historyReadRequested);

    auto *oneMinute = panel.findChild<QAbstractButton *>(QStringLiteral("oneMinuteButton"));
    QVERIFY(oneMinute);
    oneMinute->click();

    QCOMPARE(unsubscribeSpy.count(), 1);
    QCOMPARE(historySpy.count(), 1);
    QCOMPARE(historySpy.first().at(0).toString(), QString::fromLatin1(kNodeId));
}

///
/// \brief consumeHistory claims pending nodes and ignores others.
///
void TestTrendPanelWidget::consumeHistoryMatchesPendingNode()
{
    TrendPanelWidget panel;
    panel.addNode(QString::fromLatin1(kNodeId), QStringLiteral("Demo"));

    auto *oneMinute = panel.findChild<QAbstractButton *>(QStringLiteral("oneMinuteButton"));
    QVERIFY(oneMinute);
    oneMinute->click();

    QVector<OpcUaHistoryValue> values;
    QVERIFY(!panel.consumeHistory(QStringLiteral("ns=2;s=Other"), QString(), values));
    QVERIFY(panel.consumeHistory(QString::fromLatin1(kNodeId), QString(), values));
    QVERIFY(!panel.consumeHistory(QString::fromLatin1(kNodeId), QString(), values));
}

QTEST_MAIN(TestTrendPanelWidget)

#include "test_trendpanelwidget.moc"
