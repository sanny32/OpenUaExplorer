// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_elidedtextdelegate.cpp
/// \brief Tests when the cell viewer button appears and what it reports.
///

#include <QHeaderView>
#include <QSignalSpy>
#include <QStandardItemModel>
#include <QTest>
#include <QTreeView>

#include "models/valueroles.h"
#include "widgets/elidedtextdelegate.h"

class TestElidedTextDelegate : public QObject
{
    Q_OBJECT

private slots:
    void shortTextNeedsNoButton();
    void truncatedTextOffersTheButton();
    void aPictureOffersTheButtonAtAnyColumnWidth();

private:
    ///
    /// \brief Clicks where the delegate draws its button in a cell.
    /// \param view View holding the cell.
    /// \param index Cell to click into.
    ///
    static void clickButton(QTreeView *view, const QModelIndex &index);
};

///
/// \brief Clicks where the delegate draws its button in a cell.
/// \param view View holding the cell.
/// \param index Cell to click into.
///
void TestElidedTextDelegate::clickButton(QTreeView *view, const QModelIndex &index)
{
    const QRect cell = view->visualRect(index);
    const QPoint target(cell.right() - 12, cell.center().y());
    QTest::mouseClick(view->viewport(), Qt::LeftButton, Qt::KeyboardModifiers(), target);
}

///
/// \brief A value the column shows in full needs nothing but the column.
///
void TestElidedTextDelegate::shortTextNeedsNoButton()
{
    QStandardItemModel model(1, 1);
    model.setData(model.index(0, 0), QStringLiteral("7"));

    QTreeView view;
    view.setModel(&model);
    view.header()->resizeSection(0, 300);
    auto *delegate = new ElidedTextDelegate(&view);
    view.setItemDelegateForColumn(0, delegate);
    view.resize(400, 120);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QSignalSpy spy(delegate, &ElidedTextDelegate::viewRequested);
    clickButton(&view, model.index(0, 0));
    QCOMPARE(spy.count(), 0);
}

///
/// \brief Text the column has to cut short is handed to a viewer instead.
///
void TestElidedTextDelegate::truncatedTextOffersTheButton()
{
    QStandardItemModel model(1, 1);
    model.setData(model.index(0, 0), QString(400, QLatin1Char('a')));

    QTreeView view;
    view.setModel(&model);
    view.header()->resizeSection(0, 80);
    auto *delegate = new ElidedTextDelegate(&view);
    view.setItemDelegateForColumn(0, delegate);
    view.resize(400, 120);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QSignalSpy spy(delegate, &ElidedTextDelegate::viewRequested);
    clickButton(&view, model.index(0, 0));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().first().toModelIndex(), model.index(0, 0));
}

///
/// \brief A picture is offered to the viewer even when its summary fits the column.
///
void TestElidedTextDelegate::aPictureOffersTheButtonAtAnyColumnWidth()
{
    QStandardItemModel model(1, 1);
    const QModelIndex index = model.index(0, 0);
    model.setData(index, QStringLiteral("PNG 2×3"));
    model.setData(index, QByteArrayLiteral("\x89PNG..."), ValueRoles::ImageDataRole);

    QTreeView view;
    view.setModel(&model);
    view.header()->resizeSection(0, 300);
    auto *delegate = new ElidedTextDelegate(&view);
    view.setItemDelegateForColumn(0, delegate);
    view.resize(400, 120);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QSignalSpy spy(delegate, &ElidedTextDelegate::viewRequested);
    clickButton(&view, index);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().first().toModelIndex(), index);
}

QTEST_MAIN(TestElidedTextDelegate)

#include "test_elidedtextdelegate.moc"
