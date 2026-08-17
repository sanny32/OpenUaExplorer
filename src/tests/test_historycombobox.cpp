// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_historycombobox.cpp
/// \brief UI tests for HistoryComboBox entry removal.
///

#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QImage>
#include <QItemSelectionModel>
#include <QScopedPointer>
#include <QSignalSpy>
#include <QStyle>
#include <QStyleFactory>
#include <QStyledItemDelegate>
#include <QTest>

#include "widgets/historycombobox.h"
#include "widgets/separatoritemdelegate.h"

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
    void removeButtonStaysLegibleOnASelectedEntry();
    void actionEntryFollowsASeparatorAndSurvivesHistoryRefills();
    void actionEntryIsNeitherRemovableNorSelectable();
    void clickingTheActionEntryTriggersItWithoutChangingTheText();
    void popupRestoresItsDelegateAfterAStyleReplacesIt();
    void separatorDelegateReattachesAfterAStyleReplacesIt();
    void separatorIsDrawnAsAThinRuleWithoutARemoveButton();
    void separatorStaysVisibleOnADarkPopup();

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
    if (comboBox.count() == 0) {
        comboBox.addItems({QStringLiteral("opc.tcp://alpha:4840"),
                           QStringLiteral("opc.tcp://beta:4840"),
                           QStringLiteral("opc.tcp://gamma:4840")});
    }
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

///
/// \brief Renders a selected entry and measures the cross against the fill behind it.
///
/// The pale highlight is the case that used to break: styles paint it without touching
/// QPalette::Highlight, and HighlightedText is white, so a cross that blindly trusts that
/// role vanishes.
///
void TestHistoryComboBox::removeButtonStaysLegibleOnASelectedEntry()
{
    const QScopedPointer<QStyle> fusion(QStyleFactory::create(QStringLiteral("Fusion")));
    if (!fusion)
        QSKIP("The Fusion style is unavailable.");

    HistoryComboBox comboBox;
    QAbstractItemView *view = openPopup(comboBox);
    if (!view)
        QSKIP("The combo box popup is unavailable on this platform.");

    view->setStyle(fusion.data());
    QPalette palette = view->palette();
    palette.setColor(QPalette::Base, Qt::white);
    palette.setColor(QPalette::Text, Qt::black);
    palette.setColor(QPalette::Highlight, QColor(235, 235, 235));
    palette.setColor(QPalette::HighlightedText, Qt::white);
    view->setPalette(palette);

    const QModelIndex index = comboBox.model()->index(0, 0);
    view->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);

    QImage rendering(view->viewport()->size(), QImage::Format_ARGB32);
    rendering.fill(Qt::white);
    view->viewport()->render(&rendering);

    const QRect buttonRect = HistoryComboBox::removeButtonRect(view->visualRect(index));
    const int fill = rendering.pixelColor(buttonRect.left() - 2, buttonRect.center().y()).lightness();

    int strongestContrast = 0;
    for (int y = buttonRect.top(); y <= buttonRect.bottom(); ++y) {
        for (int x = buttonRect.left(); x <= buttonRect.right(); ++x)
            strongestContrast = qMax(strongestContrast,
                                     qAbs(rendering.pixelColor(x, y).lightness() - fill));
    }

    QVERIFY2(strongestContrast > 40,
             qPrintable(QStringLiteral("The cross blends into the selected entry "
                                       "(strongest contrast was %1, fill lightness %2).")
                            .arg(strongestContrast).arg(fill)));
}

void TestHistoryComboBox::actionEntryFollowsASeparatorAndSurvivesHistoryRefills()
{
    HistoryComboBox comboBox;
    comboBox.setEditable(true);
    comboBox.setActionEntry(QStringLiteral("Find Servers..."));
    comboBox.setHistory({QStringLiteral("opc.tcp://alpha:4840"),
                         QStringLiteral("opc.tcp://beta:4840")});

    QCOMPARE(comboBox.count(), 4);
    QCOMPARE(comboBox.itemText(3), QStringLiteral("Find Servers..."));
    QVERIFY(comboBox.isActionEntry(3));
    QVERIFY(!comboBox.isActionEntry(1));
    QCOMPARE(comboBox.itemData(2, Qt::AccessibleDescriptionRole).toString(),
             QStringLiteral("separator"));

    comboBox.setHistory({QStringLiteral("opc.tcp://gamma:4840")});
    QCOMPARE(comboBox.count(), 3);
    QVERIFY(comboBox.isActionEntry(2));
}

void TestHistoryComboBox::actionEntryIsNeitherRemovableNorSelectable()
{
    HistoryComboBox comboBox;
    comboBox.setHistory({QStringLiteral("opc.tcp://alpha:4840"),
                         QStringLiteral("opc.tcp://beta:4840"),
                         QStringLiteral("opc.tcp://gamma:4840")});
    comboBox.setActionEntry(QStringLiteral("Find Servers..."));
    QAbstractItemView *view = openPopup(comboBox);
    if (!view)
        QSKIP("The combo box popup is unavailable on this platform.");

    const int actionRow = comboBox.count() - 1;
    QVERIFY(comboBox.isActionEntry(actionRow));

    QSignalSpy removedSpy(&comboBox, &HistoryComboBox::itemRemoved);
    const QPoint target = HistoryComboBox::removeButtonRect(
                              view->visualRect(comboBox.model()->index(actionRow, 0)))
                              .center();
    QTest::mouseClick(view->viewport(), Qt::LeftButton, Qt::KeyboardModifiers(), target);
    QCOMPARE(removedSpy.size(), 0);

    view->setCurrentIndex(comboBox.model()->index(actionRow, 0));
    QTest::keyClick(view, Qt::Key_Delete);
    QCOMPARE(removedSpy.size(), 0);
    QCOMPARE(comboBox.count(), actionRow + 1);

    comboBox.setCurrentIndex(actionRow);
    QVERIFY(!comboBox.isActionEntry(comboBox.currentIndex()));
}

void TestHistoryComboBox::clickingTheActionEntryTriggersItWithoutChangingTheText()
{
    HistoryComboBox comboBox;
    comboBox.setHistory({QStringLiteral("opc.tcp://alpha:4840"),
                         QStringLiteral("opc.tcp://beta:4840"),
                         QStringLiteral("opc.tcp://gamma:4840")});
    comboBox.setActionEntry(QStringLiteral("Find Servers..."));
    QAbstractItemView *view = openPopup(comboBox);
    if (!view)
        QSKIP("The combo box popup is unavailable on this platform.");

    comboBox.setCurrentIndex(1);
    const QString before = comboBox.currentText();

    QSignalSpy actionSpy(&comboBox, &HistoryComboBox::actionTriggered);
    QSignalSpy activatedSpy(&comboBox, &HistoryComboBox::activated);

    const int actionRow = comboBox.count() - 1;
    const QRect itemRect = view->visualRect(comboBox.model()->index(actionRow, 0));
    QTest::mouseClick(view->viewport(), Qt::LeftButton, Qt::KeyboardModifiers(),
                      QPoint(itemRect.left() + 8, itemRect.center().y()));

    QCOMPARE(actionSpy.size(), 1);
    QCOMPARE(activatedSpy.size(), 0);
    QCOMPARE(comboBox.currentIndex(), 1);
    QCOMPARE(comboBox.currentText(), before);
}

///
/// \brief Opening the popup restores the history delegate replaced during style polishing.
///
void TestHistoryComboBox::popupRestoresItsDelegateAfterAStyleReplacesIt()
{
    HistoryComboBox comboBox;
    QAbstractItemView *view = comboBox.view();
    QAbstractItemDelegate *historyDelegate = view->itemDelegate();
    view->setItemDelegate(new QStyledItemDelegate(view));
    QVERIFY(view->itemDelegate() != historyDelegate);

    view = openPopup(comboBox);
    if (!view)
        QSKIP("The combo box popup is unavailable on this platform.");

    QCOMPARE(view->itemDelegate(), historyDelegate);
}

///
/// \brief Reattaching a separator delegate replaces a delegate installed by the style.
///
void TestHistoryComboBox::separatorDelegateReattachesAfterAStyleReplacesIt()
{
    QComboBox comboBox;
    SeparatorItemDelegate::attachTo(&comboBox);
    QAbstractItemView *view = comboBox.view();
    QAbstractItemDelegate *separatorDelegate = view->itemDelegate();

    view->setItemDelegate(new QStyledItemDelegate(view));
    QVERIFY(view->itemDelegate() != separatorDelegate);

    SeparatorItemDelegate::attachTo(&comboBox);
    QCOMPARE(view->itemDelegate(), separatorDelegate);
}

///
/// \brief Guards the separator row, whose painting QComboBoxDelegate would normally own.
///
void TestHistoryComboBox::separatorIsDrawnAsAThinRuleWithoutARemoveButton()
{
    HistoryComboBox comboBox;
    comboBox.setHistory({QStringLiteral("opc.tcp://alpha:4840"),
                         QStringLiteral("opc.tcp://beta:4840")});
    comboBox.setActionEntry(QStringLiteral("Find Servers..."));
    QAbstractItemView *view = openPopup(comboBox);
    if (!view)
        QSKIP("The combo box popup is unavailable on this platform.");

    const QRect entryRect = view->visualRect(comboBox.model()->index(0, 0));
    const QRect separatorRect = view->visualRect(comboBox.model()->index(2, 0));
    QVERIFY2(separatorRect.height() < entryRect.height() / 2,
             qPrintable(QStringLiteral("The separator occupies a full entry row (%1 of %2 px).")
                            .arg(separatorRect.height()).arg(entryRect.height())));

    QImage rendering(view->viewport()->size(), QImage::Format_ARGB32);
    rendering.fill(Qt::white);
    view->viewport()->render(&rendering);

    const QRect buttonRect = HistoryComboBox::removeButtonRect(separatorRect);
    for (int y = buttonRect.top(); y <= buttonRect.bottom(); ++y) {
        for (int x = buttonRect.left(); x <= buttonRect.right(); ++x) {
            if (!rendering.rect().contains(x, y))
                continue;
            QVERIFY2(rendering.pixelColor(x, y).lightness() > 200,
                     "A remove button was painted over the separator.");
        }
    }
}

///
/// \brief Renders the separator on a dark popup and measures it against the background.
///
/// The style's toolbar-separator primitive draws a rule that is invisible on a dark
/// popup, which is why the delegate derives the colour from QPalette::Text instead.
///
void TestHistoryComboBox::separatorStaysVisibleOnADarkPopup()
{
    const QColor background(0x2b, 0x2b, 0x2b);

    HistoryComboBox comboBox;
    comboBox.setHistory({QStringLiteral("opc.tcp://alpha:4840"),
                         QStringLiteral("opc.tcp://beta:4840")});
    comboBox.setActionEntry(QStringLiteral("Find Servers..."));
    QAbstractItemView *view = openPopup(comboBox);
    if (!view)
        QSKIP("The combo box popup is unavailable on this platform.");

    QPalette dark = view->palette();
    dark.setColor(QPalette::Base, background);
    dark.setColor(QPalette::Text, QColor(0xe0, 0xe0, 0xe0));
    view->setPalette(dark);

    QImage rendering(view->viewport()->size(), QImage::Format_ARGB32);
    rendering.fill(background);
    view->viewport()->render(&rendering);

    const QRect separatorRect = view->visualRect(comboBox.model()->index(2, 0));
    int strongestContrast = 0;
    for (int y = separatorRect.top(); y <= separatorRect.bottom(); ++y) {
        for (int x = separatorRect.left(); x <= separatorRect.right(); ++x) {
            if (!rendering.rect().contains(x, y))
                continue;
            strongestContrast = qMax(strongestContrast,
                                     qAbs(rendering.pixelColor(x, y).lightness()
                                          - background.lightness()));
        }
    }

    QVERIFY2(strongestContrast > 15,
             qPrintable(QStringLiteral("The separator blends into the dark popup "
                                       "(strongest contrast was %1).")
                            .arg(strongestContrast)));
}

QTEST_MAIN(TestHistoryComboBox)

#include "test_historycombobox.moc"
