// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_historycombobox.cpp
/// \brief UI tests for HistoryComboBox entry removal.
///

#include <QAbstractItemView>
#include <QApplication>
#include <QSignalSpy>
#include <QTest>

#include "widgets/historycombobox.h"

///
/// \brief Verifies that popup entries can be removed without disturbing the selection.
///
class TestHistoryComboBox : public QObject
{
    Q_OBJECT

private slots:
    void clickingRemoveButtonDropsEntryAndKeepsPopupOpen();
    void removingTheCurrentEntryReplacesTheEditText();
    void deleteKeyRemovesTheHighlightedEntry();
    void clickingEntryTextStillActivatesIt();

private:
    static QAbstractItemView *openPopup(HistoryComboBox &comboBox);
};

///
/// \brief Shows a populated combo box and returns its exposed popup view.
/// \param comboBox Combo box to open.
/// \return Popup view, or nullptr when the popup did not appear.
///
QAbstractItemView *TestHistoryComboBox::openPopup(HistoryComboBox &comboBox)
{
    comboBox.setEditable(true);
    comboBox.addItems({QStringLiteral("opc.tcp://alpha:4840"),
                       QStringLiteral("opc.tcp://beta:4840"),
                       QStringLiteral("opc.tcp://gamma:4840")});
    comboBox.resize(300, 30);
    comboBox.show();
    if (!QTest::qWaitForWindowExposed(&comboBox))
        return nullptr;

    comboBox.showPopup();
    QAbstractItemView *view = comboBox.view();
    if (!view->isVisible())
        return nullptr;
    return view;
}

void TestHistoryComboBox::clickingRemoveButtonDropsEntryAndKeepsPopupOpen()
{
    HistoryComboBox comboBox;
    QAbstractItemView *view = openPopup(comboBox);
    if (!view)
        QSKIP("The combo box popup is unavailable on this platform.");

    QSignalSpy removedSpy(&comboBox, &HistoryComboBox::itemRemoved);
    QSignalSpy activatedSpy(&comboBox, &HistoryComboBox::activated);

    const QModelIndex index = comboBox.model()->index(1, 0);
    const QPoint target =
        HistoryComboBox::removeButtonRect(view->visualRect(index)).center();
    QTest::mouseClick(view->viewport(), Qt::LeftButton, Qt::KeyboardModifiers(), target);

    QCOMPARE(removedSpy.size(), 1);
    QCOMPARE(removedSpy.first().first().toString(), QStringLiteral("opc.tcp://beta:4840"));
    QCOMPARE(activatedSpy.size(), 0);
    QCOMPARE(comboBox.count(), 2);
    QCOMPARE(comboBox.itemText(1), QStringLiteral("opc.tcp://gamma:4840"));
    QVERIFY(view->isVisible());
    QCOMPARE(comboBox.currentText(), QStringLiteral("opc.tcp://alpha:4840"));
}

void TestHistoryComboBox::removingTheCurrentEntryReplacesTheEditText()
{
    HistoryComboBox comboBox;
    QAbstractItemView *view = openPopup(comboBox);
    if (!view)
        QSKIP("The combo box popup is unavailable on this platform.");

    const QModelIndex index = comboBox.model()->index(0, 0);
    const QPoint target =
        HistoryComboBox::removeButtonRect(view->visualRect(index)).center();
    QTest::mouseClick(view->viewport(), Qt::LeftButton, Qt::KeyboardModifiers(), target);

    QCOMPARE(comboBox.count(), 2);
    QVERIFY(comboBox.currentText() != QStringLiteral("opc.tcp://alpha:4840"));
}

void TestHistoryComboBox::deleteKeyRemovesTheHighlightedEntry()
{
    HistoryComboBox comboBox;
    QAbstractItemView *view = openPopup(comboBox);
    if (!view)
        QSKIP("The combo box popup is unavailable on this platform.");

    QSignalSpy removedSpy(&comboBox, &HistoryComboBox::itemRemoved);
    view->setCurrentIndex(comboBox.model()->index(2, 0));
    QTest::keyClick(view, Qt::Key_Delete);

    QCOMPARE(removedSpy.size(), 1);
    QCOMPARE(removedSpy.first().first().toString(), QStringLiteral("opc.tcp://gamma:4840"));
    QCOMPARE(comboBox.count(), 2);
}

void TestHistoryComboBox::clickingEntryTextStillActivatesIt()
{
    HistoryComboBox comboBox;
    QAbstractItemView *view = openPopup(comboBox);
    if (!view)
        QSKIP("The combo box popup is unavailable on this platform.");

    QSignalSpy activatedSpy(&comboBox, &HistoryComboBox::activated);

    const QRect itemRect = view->visualRect(comboBox.model()->index(1, 0));
    QTest::mouseClick(view->viewport(), Qt::LeftButton, Qt::KeyboardModifiers(),
                      QPoint(itemRect.left() + 8, itemRect.center().y()));

    QCOMPARE(activatedSpy.size(), 1);
    QCOMPARE(comboBox.count(), 3);
    QCOMPARE(comboBox.currentText(), QStringLiteral("opc.tcp://beta:4840"));
}

QTEST_MAIN(TestHistoryComboBox)

#include "test_historycombobox.moc"
