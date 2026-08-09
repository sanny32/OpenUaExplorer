// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_treetableview.cpp
/// \brief Tests the shared TreeTableView widget.
///

#include <QFontMetrics>
#include <QHeaderView>
#include <QHelpEvent>
#include <QStandardItemModel>
#include <QTest>
#include <QToolTip>

#include "widgets/headerview.h"
#include "widgets/treetableview.h"

///
/// \brief UI tests for TreeTableView.
///
class TestTreeTableView : public QObject
{
    Q_OBJECT

private slots:
    void headerDefaultsToSingleLineHeight();
    void toolTipOnlyShowsForElidedCells();
};

///
/// \brief The tree carries the shared wrapped header at one-line height.
///
void TestTreeTableView::headerDefaultsToSingleLineHeight()
{
    TreeTableView view;
    auto *header = view.headerView();
    QVERIFY(header);
    QCOMPARE(header, view.header());

    view.resize(800, 200);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    const QFontMetrics fm(header->font());
    QVERIFY(header->height() < fm.lineSpacing() * 2 + 14);
}

///
/// \brief A cell that is cut off shows a tooltip; the same cell shows none once it fits.
///
void TestTreeTableView::toolTipOnlyShowsForElidedCells()
{
    QStandardItemModel model(1, 1);
    model.setItem(0, 0, new QStandardItem(
        QStringLiteral("A value far too long for the column it lives in")));

    TreeTableView view;
    view.setModel(&model);
    view.resize(400, 120);
    view.setColumnWidth(0, 2000);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    const auto sendToolTip = [&view](const QModelIndex &index) {
        const QPoint pos = view.visualRect(index).center();
        QHelpEvent event(QEvent::ToolTip, pos, view.viewport()->mapToGlobal(pos));
        QCoreApplication::sendEvent(view.viewport(), &event);
    };

    // The text fits in the column, so nothing is repeated in a tooltip.
    sendToolTip(model.index(0, 0));
    QVERIFY(!QToolTip::isVisible());

    view.setColumnWidth(0, 40);
    sendToolTip(model.index(0, 0));
    QVERIFY(QToolTip::isVisible());
    QToolTip::hideText();
}

QTEST_MAIN(TestTreeTableView)

#include "test_treetableview.moc"
