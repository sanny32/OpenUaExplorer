// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_tableview.cpp
/// \brief Tests the shared TableView widget.
///

#include <QFontMetrics>
#include <QHeaderView>
#include <QTest>

#include "widgets/headerview.h"
#include "widgets/tableview.h"

///
/// \brief UI tests for TableView.
///
class TestTableView : public QObject
{
    Q_OBJECT

private slots:
    void horizontalHeaderDefaultsToSingleLineHeight();
};

///
/// \brief The shared table header starts at one-line height.
///
void TestTableView::horizontalHeaderDefaultsToSingleLineHeight()
{
    TableView view;
    auto *header = view.headerView();
    QVERIFY(header);

    view.resize(800, 200);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    const QFontMetrics fm(header->font());
    QVERIFY(header->height() < fm.lineSpacing() * 2 + 14);
}

QTEST_MAIN(TestTableView)

#include "test_tableview.moc"
