// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_dataaccesswidget_write.cpp
/// \brief Tests DataAccessWidget value editing and double-click behavior.
///

#include <QTest>

#include "test_dataaccesswidget_support.h"

///
/// \brief Tests the related behavior in an isolated Qt Test executable.
///
class TestDataAccessWidgetWrite : public QObject
{
    Q_OBJECT

private slots:
    void writeButtonNeedsOneWritableRow();
    void doubleClickTogglesWritableBooleanValue();
    void declinedDoubleClickWritesNothing();
    void doubleClickOnReadOnlyBooleanOpensReadOnlyDialog();
    void doubleClickOnNonBooleanOpensWriteDialog();
    void doubleClickOnReadOnlyValueOpensReadOnlyDialog();
    void doubleClickOnEnumerationOpensItsNames();
    void declinedEnumerationPickWritesNothing();
    void readOnlyEnumerationOpensTheWriteDialog();
    void doubleClickOnPendingRowWritesNothing();
    void doubleClickOutsideValueColumnWritesNothing();
    void doubleClickWhileOfflineWritesNothing();
};

///
/// \brief Write stays disabled until exactly one writable row is selected.
///
void TestDataAccessWidgetWrite::writeButtonNeedsOneWritableRow()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    auto *writeButton = widget.findChild<QAbstractButton *>(QStringLiteral("writeButton"));
    QVERIFY(view);
    QVERIFY(writeButton);

    OpcUaNodeDetails details = makeNodeDetails();
    details.userAccessLevel = OpcUa::CurrentRead;
    widget.addNode(details);
    selectAllRows(view);

    QSignalSpy writeSpy(&widget, &DataAccessWidget::writeRequested);
    QVERIFY(!writeButton->isEnabled());
    writeButton->click();
    QCOMPARE(writeSpy.size(), 0);

    // The access level of a selected row arrives with its attribute read.
    details.userAccessLevel = OpcUa::CurrentRead | OpcUa::CurrentWrite;
    widget.addNode(details);
    QVERIFY(writeButton->isEnabled());
    writeButton->click();
    QCOMPARE(writeSpy.size(), 1);

    OpcUaNodeDetails second = details;
    second.nodeId = QStringLiteral("ns=2;s=Pressure");
    widget.addNode(second);
    selectAllRows(view);
    QVERIFY(!writeButton->isEnabled());
}

///
/// \brief A confirmed double click on a writable Boolean writes the inverted value.
///
void TestDataAccessWidgetWrite::doubleClickTogglesWritableBooleanValue()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    QVERIFY(view);
    widget.addNode(makeBooleanNodeDetails(false, true));
    widget.resize(900, 200);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    QSignalSpy spy(&widget, &DataAccessWidget::valueWriteRequested);
    answerNextDialog(DialogButtonBox::Yes);
    doubleClickCell(view, 0, DataAccessModel::ColValue);

    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.first().at(0).toString(), QStringLiteral("ns=2;s=Locked"));
    QCOMPARE(spy.first().at(1).userType(), static_cast<int>(QMetaType::Bool));
    QCOMPARE(spy.first().at(1).toBool(), true);
    QCOMPARE(spy.first().at(2).toInt(), 0);

    widget.addNode(makeBooleanNodeDetails(true, true));
    answerNextDialog(DialogButtonBox::Yes);
    doubleClickCell(view, 0, DataAccessModel::ColValue);

    QCOMPARE(spy.size(), 2);
    QCOMPARE(spy.last().at(1).toBool(), false);
}

///
/// \brief Declining the confirmation leaves the value on the server untouched.
///
void TestDataAccessWidgetWrite::declinedDoubleClickWritesNothing()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    QVERIFY(view);
    widget.addNode(makeBooleanNodeDetails(false, true));
    widget.resize(900, 200);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    QSignalSpy spy(&widget, &DataAccessWidget::valueWriteRequested);
    answerNextDialog(DialogButtonBox::No);
    doubleClickCell(view, 0, DataAccessModel::ColValue);

    QCOMPARE(spy.size(), 0);
}

///
/// \brief A Boolean the user may not write is shown in the dialog instead of toggling.
///
void TestDataAccessWidgetWrite::doubleClickOnReadOnlyBooleanOpensReadOnlyDialog()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    QVERIFY(view);
    widget.addNode(makeBooleanNodeDetails(false, false));
    widget.resize(900, 200);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    QSignalSpy spy(&widget, &DataAccessWidget::valueWriteRequested);
    QSignalSpy writeSpy(&widget, &DataAccessWidget::writeRequested);
    doubleClickCell(view, 0, DataAccessModel::ColValue);

    QCOMPARE(spy.size(), 0);
    QCOMPARE(writeSpy.size(), 1);
    QCOMPARE(writeSpy.first().at(4).toBool(), false);
}

///
/// \brief Values of other types ask for the write dialog instead of toggling.
///
void TestDataAccessWidgetWrite::doubleClickOnNonBooleanOpensWriteDialog()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    QVERIFY(view);
    OpcUaNodeDetails details = makeNodeDetails();
    details.userAccessLevel = OpcUa::CurrentRead | OpcUa::CurrentWrite;
    widget.addNode(details);
    widget.resize(900, 200);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    QSignalSpy spy(&widget, &DataAccessWidget::valueWriteRequested);
    QSignalSpy writeSpy(&widget, &DataAccessWidget::writeRequested);
    doubleClickCell(view, 0, DataAccessModel::ColValue);

    QCOMPARE(spy.size(), 0);
    QCOMPARE(writeSpy.size(), 1);
    QCOMPARE(writeSpy.first().at(0).toString(), details.nodeId);
    QCOMPARE(writeSpy.first().at(1).toDouble(), details.value.toDouble());
    QCOMPARE(writeSpy.first().at(3).toString(), details.dataTypeId);
    QCOMPARE(writeSpy.first().at(4).toBool(), true);
}

///
/// \brief A value the user may not write opens the dialog in read-only mode.
///
void TestDataAccessWidgetWrite::doubleClickOnReadOnlyValueOpensReadOnlyDialog()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    QVERIFY(view);
    OpcUaNodeDetails details = makeNodeDetails();
    details.userAccessLevel = OpcUa::CurrentRead;
    widget.addNode(details);
    widget.resize(900, 200);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    QSignalSpy writeSpy(&widget, &DataAccessWidget::writeRequested);
    doubleClickCell(view, 0, DataAccessModel::ColValue);

    QCOMPARE(writeSpy.size(), 1);
    QCOMPARE(writeSpy.first().at(0).toString(), details.nodeId);
    QCOMPARE(writeSpy.first().at(4).toBool(), false);
}

///
/// \brief A writable enumeration is picked from its named values, right in the cell.
///
void TestDataAccessWidgetWrite::doubleClickOnEnumerationOpensItsNames()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    QVERIFY(view);
    const OpcUaNodeDetails details = makeEnumNodeDetails(0, true);
    widget.addNode(details);
    widget.resize(900, 200);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    const QModelIndex value = view->model()->index(0, DataAccessModel::ColValue);
    QCOMPARE(value.data().toString(), QStringLiteral("0 (Disabled)"));

    QSignalSpy spy(&widget, &DataAccessWidget::valueWriteRequested);
    QSignalSpy writeSpy(&widget, &DataAccessWidget::writeRequested);
    doubleClickCell(view, 0, DataAccessModel::ColValue);

    // The list replaces the dialog, and picking an entry writes the number behind it.
    QCOMPARE(writeSpy.size(), 0);
    auto *combo = view->viewport()->findChild<QComboBox *>();
    QVERIFY(combo);
    QCOMPARE(combo->count(), 3);
    QCOMPARE(combo->itemText(2), QStringLiteral("2 (Error)"));
    QCOMPARE(combo->currentIndex(), 0);

    answerNextDialog(DialogButtonBox::Yes);
    combo->setCurrentIndex(2);
    emit combo->activated(2);

    QTRY_COMPARE(spy.size(), 1);
    QCOMPARE(spy.first().at(0).toString(), details.nodeId);
    QCOMPARE(spy.first().at(1).toInt(), 2);
    QCOMPARE(spy.first().at(2).toInt(), int(QOpcUa::Types::Int32));
}

///
/// \brief A declined enumeration prompt leaves the value alone.
///
void TestDataAccessWidgetWrite::declinedEnumerationPickWritesNothing()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    QVERIFY(view);
    widget.addNode(makeEnumNodeDetails(0, true));
    widget.resize(900, 200);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    QSignalSpy spy(&widget, &DataAccessWidget::valueWriteRequested);
    doubleClickCell(view, 0, DataAccessModel::ColValue);
    auto *combo = view->viewport()->findChild<QComboBox *>();
    QVERIFY(combo);

    bool dialogSeen = false;
    answerNextDialog(DialogButtonBox::No, &dialogSeen);
    combo->setCurrentIndex(2);
    emit combo->activated(2);

    QTRY_VERIFY(dialogSeen);
    QCOMPARE(spy.size(), 0);
    QCOMPARE(view->model()->index(0, DataAccessModel::ColValue).data().toString(),
             QStringLiteral("0 (Disabled)"));
}

///
/// \brief An enumeration the user may not write keeps the read-only dialog.
///
void TestDataAccessWidgetWrite::readOnlyEnumerationOpensTheWriteDialog()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    QVERIFY(view);
    widget.addNode(makeEnumNodeDetails(1, false));
    widget.resize(900, 200);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    QSignalSpy writeSpy(&widget, &DataAccessWidget::writeRequested);
    doubleClickCell(view, 0, DataAccessModel::ColValue);

    QVERIFY(!view->viewport()->findChild<QComboBox *>());
    QCOMPARE(writeSpy.size(), 1);
    QCOMPARE(writeSpy.first().at(4).toBool(), false);
    // The dialog is handed the names so it can offer them too.
    QCOMPARE(writeSpy.first().at(5).value<OpcUaEnumEntries>().size(), 3);
}

///
/// \brief A row still waiting for its attributes opens no dialog.
///
void TestDataAccessWidgetWrite::doubleClickOnPendingRowWritesNothing()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    QVERIFY(view);
    widget.addPendingNodes({makeDroppedNode(OpcUa::Variable)});
    widget.resize(900, 200);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    QSignalSpy writeSpy(&widget, &DataAccessWidget::writeRequested);
    doubleClickCell(view, 0, DataAccessModel::ColValue);

    QCOMPARE(writeSpy.size(), 0);
}

///
/// \brief Only the Value column writes; the other columns stay inert.
///
void TestDataAccessWidgetWrite::doubleClickOutsideValueColumnWritesNothing()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    QVERIFY(view);
    widget.addNode(makeBooleanNodeDetails(false, true));
    widget.resize(900, 200);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    QSignalSpy spy(&widget, &DataAccessWidget::valueWriteRequested);
    QSignalSpy writeSpy(&widget, &DataAccessWidget::writeRequested);
    bool dialogSeen = false;
    watchForDialog(&dialogSeen);
    doubleClickCell(view, 0, DataAccessModel::ColDisplayName);
    QCoreApplication::processEvents();

    QVERIFY(!dialogSeen);
    QCOMPARE(spy.size(), 0);
    QCOMPARE(writeSpy.size(), 0);
}

///
/// \brief Nothing is written while the server connection is gone.
///
void TestDataAccessWidgetWrite::doubleClickWhileOfflineWritesNothing()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    QVERIFY(view);
    widget.addNode(makeBooleanNodeDetails(false, true));
    widget.resize(900, 200);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    widget.setOffline(true);

    QSignalSpy spy(&widget, &DataAccessWidget::valueWriteRequested);
    QSignalSpy writeSpy(&widget, &DataAccessWidget::writeRequested);
    bool dialogSeen = false;
    watchForDialog(&dialogSeen);
    doubleClickCell(view, 0, DataAccessModel::ColValue);
    QCoreApplication::processEvents();

    QVERIFY(!dialogSeen);
    QCOMPARE(spy.size(), 0);
    QCOMPARE(writeSpy.size(), 0);
}


QTEST_MAIN(TestDataAccessWidgetWrite)

#include "test_dataaccesswidget_write.moc"
