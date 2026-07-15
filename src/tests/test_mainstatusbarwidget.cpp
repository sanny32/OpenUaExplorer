// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_mainstatusbarwidget.cpp
/// \brief Tests the main status bar widget.
///

#include <QLabel>
#include <QTest>

#include "mainstatusbarwidget.h"

///
/// \brief UI tests for the main status bar.
///
class TestMainStatusBarWidget : public QObject
{
    Q_OBJECT

private slots:
    void clockLabelsReserveStableWidths();
    void clockWidthsFollowFontChanges();
};

///
/// \brief Clock labels reserve enough width for wide digit combinations.
///
void TestMainStatusBarWidget::clockLabelsReserveStableWidths()
{
    MainStatusBarWidget widget;
    auto *serverLabel = widget.findChild<QLabel *>(QStringLiteral("serverTimeLabel"));
    auto *localLabel = widget.findChild<QLabel *>(QStringLiteral("localTimeLabel"));
    QVERIFY(serverLabel);
    QVERIFY(localLabel);

    const int serverMinimumWidth = serverLabel->minimumWidth();
    const int localMinimumWidth = localLabel->minimumWidth();
    QVERIFY(serverMinimumWidth >= serverLabel->fontMetrics().horizontalAdvance(serverLabel->text()));
    QVERIFY(localMinimumWidth >= localLabel->fontMetrics().horizontalAdvance(localLabel->text()));

    serverLabel->setText(QStringLiteral("Server Time: 11:11:11 UTC"));
    localLabel->setText(QStringLiteral("Local Time: 11:11:11 UTC+3"));
    QCOMPARE(serverLabel->minimumWidth(), serverMinimumWidth);
    QCOMPARE(localLabel->minimumWidth(), localMinimumWidth);
}

///
/// \brief Clock width reservations are refreshed after a font change.
///
void TestMainStatusBarWidget::clockWidthsFollowFontChanges()
{
    MainStatusBarWidget widget;
    auto *serverLabel = widget.findChild<QLabel *>(QStringLiteral("serverTimeLabel"));
    auto *localLabel = widget.findChild<QLabel *>(QStringLiteral("localTimeLabel"));
    QVERIFY(serverLabel);
    QVERIFY(localLabel);

    const int serverMinimumWidth = serverLabel->minimumWidth();
    const int localMinimumWidth = localLabel->minimumWidth();
    QFont largerFont = widget.font();
    largerFont.setPointSize(largerFont.pointSize() + 8);
    widget.setFont(largerFont);

    QVERIFY(serverLabel->minimumWidth() > serverMinimumWidth);
    QVERIFY(localLabel->minimumWidth() > localMinimumWidth);
}

QTEST_MAIN(TestMainStatusBarWidget)

#include "test_mainstatusbarwidget.moc"
