// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_dataaccesswidget_rows.cpp
/// \brief Tests DataAccessWidget row ordering, selection, clearing, and filtering.
///

#include <QTest>

#include "test_dataaccesswidget_support.h"

///
/// \brief Tests the related behavior in an isolated Qt Test executable.
///
class TestDataAccessWidgetRows : public QObject
{
    Q_OBJECT

private slots:
    void draggedRowsReorderTheSavedNodes();
    void draggedRowsFollowTheFilteredRowsTheyLandOn();
    void rowDroppedOnTheDataViewLandsWhereTheIndicatorPointed();
    void pendingRowsAreExcludedFromSelectionActions();
    void deleteKeyRemovesEverySelectedRow();
    void removeButtonDropsEverySelectedRow();
    void confirmedClearRemovesEveryNode();
    void declinedClearKeepsEveryNode();
    void clearOfEmptyTableAsksNothing();
    void silentClearAsksNothing();
    void filterKeepsOnlyMatchingRows();
    void filterMatchesNamesOnly();
    void actionsOnFilteredRowsUseTheirOwnNodes();
    void arrayRowsExpandUnderTheirNode();
    void selectedElementRowActsOnItsNode();
};

///
/// \brief A row dragged onto another one takes its place, and the new order is saved.
///
void TestDataAccessWidgetRows::draggedRowsReorderTheSavedNodes()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    QVERIFY(view);

    const QVector<SessionNode> savedNodes{
        {QStringLiteral("ns=2;s=First"), QStringLiteral("Default"), HighlightMode::FollowDefault},
        {QStringLiteral("ns=2;s=Second"), QString(), HighlightMode::FollowDefault},
        {QStringLiteral("ns=2;s=Third"), QStringLiteral("Fast"), HighlightMode::Enabled}
    };
    widget.restoreMonitoredNodes(savedNodes);

    QAbstractItemModel *model = view->model();
    QScopedPointer<QMimeData> dragged(model->mimeData({model->index(2, 0)}));
    QVERIFY(dragged);
    QVERIFY(model->dropMimeData(dragged.data(), Qt::MoveAction, 0, 0, QModelIndex()));

    const QVector<SessionNode> saved = widget.monitoredNodes();
    QCOMPARE(saved.size(), savedNodes.size());
    QCOMPARE(saved.at(0).nodeId, QStringLiteral("ns=2;s=Third"));
    QCOMPARE(saved.at(1).nodeId, QStringLiteral("ns=2;s=First"));
    QCOMPARE(saved.at(2).nodeId, QStringLiteral("ns=2;s=Second"));
    // The moved row keeps its subscription and its highlight override.
    QCOMPARE(saved.at(0).subscriptionName, QStringLiteral("Fast"));
    QCOMPARE(saved.at(0).highlight, HighlightMode::Enabled);
    QCOMPARE(model->data(model->index(0, DataAccessModel::ColNodeId)).toString(),
             QStringLiteral("ns=2;s=Third"));
}

///
/// \brief A drop while filtering lands in front of the visible row, not of its row number.
///
void TestDataAccessWidgetRows::draggedRowsFollowTheFilteredRowsTheyLandOn()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    auto *filterEdit = widget.findChild<QLineEdit *>(QStringLiteral("filterEdit"));
    QVERIFY(view);
    QVERIFY(filterEdit);

    const QVector<SessionNode> savedNodes{
        {QStringLiteral("ns=2;s=Alpha"), QString(), HighlightMode::FollowDefault},
        {QStringLiteral("ns=2;s=Beta"), QString(), HighlightMode::FollowDefault},
        {QStringLiteral("ns=2;s=Gamma"), QString(), HighlightMode::FollowDefault},
        {QStringLiteral("ns=2;s=Delta"), QString(), HighlightMode::FollowDefault}
    };
    widget.restoreMonitoredNodes(savedNodes);

    // Leaves Beta and Delta visible, with two hidden rows between them.
    filterEdit->setText(QStringLiteral("t"));
    QAbstractItemModel *model = view->model();
    QCOMPARE(model->rowCount(), 2);

    QScopedPointer<QMimeData> dragged(model->mimeData({model->index(1, 0)}));
    QVERIFY(dragged);
    QVERIFY(model->dropMimeData(dragged.data(), Qt::MoveAction, 0, 0, QModelIndex()));

    QStringList order;
    for (const SessionNode &node : widget.monitoredNodes())
        order.append(node.nodeId);
    QCOMPARE(order, QStringList({QStringLiteral("ns=2;s=Alpha"), QStringLiteral("ns=2;s=Delta"),
                                 QStringLiteral("ns=2;s=Beta"), QStringLiteral("ns=2;s=Gamma")}));
}

///
/// \brief The table itself accepts a reorder drop between two rows.
///
/// Drives the view rather than the model so the drag settings of the table are
/// covered too: a drop on a row must insert next to it, never replace it.
///
void TestDataAccessWidgetRows::rowDroppedOnTheDataViewLandsWhereTheIndicatorPointed()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    QVERIFY(view);

    const QVector<SessionNode> savedNodes{
        {QStringLiteral("ns=2;s=First"), QString(), HighlightMode::FollowDefault},
        {QStringLiteral("ns=2;s=Second"), QString(), HighlightMode::FollowDefault},
        {QStringLiteral("ns=2;s=Third"), QString(), HighlightMode::FollowDefault}
    };
    widget.restoreMonitoredNodes(savedNodes);

    // The drop is resolved against the viewport, which needs its real geometry.
    widget.resize(600, 300);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    QAbstractItemModel *model = view->model();
    QScopedPointer<QMimeData> dragged(model->mimeData({model->index(2, 0)}));
    QVERIFY(dragged);

    // The upper part of the first row: the drop indicator sits above it.
    const QRect firstRow = view->visualRect(model->index(0, DataAccessModel::ColNodeId));
    QVERIFY(firstRow.isValid());
    const QPointF pos(firstRow.center().x(), firstRow.top() + firstRow.height() / 4.0);

    QDragEnterEvent enterEvent(pos.toPoint(), Qt::MoveAction, dragged.data(),
                               Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(view->viewport(), &enterEvent);
    QVERIFY(enterEvent.isAccepted());

    QDragMoveEvent moveEvent(pos.toPoint(), Qt::MoveAction, dragged.data(),
                             Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(view->viewport(), &moveEvent);
    QVERIFY(moveEvent.isAccepted());

    QDropEvent dropEvent(pos, Qt::MoveAction, dragged.data(), Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(view->viewport(), &dropEvent);
    QVERIFY(dropEvent.isAccepted());

    QStringList order;
    for (const SessionNode &node : widget.monitoredNodes())
        order.append(node.nodeId);
    QCOMPARE(order, QStringList({QStringLiteral("ns=2;s=Third"), QStringLiteral("ns=2;s=First"),
                                 QStringLiteral("ns=2;s=Second")}));
}

///
/// \brief Read, write and subscribe skip rows that are still pending.
///
void TestDataAccessWidgetRows::pendingRowsAreExcludedFromSelectionActions()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    QVERIFY(view);

    widget.addPendingNodes({makeDroppedNode(OpcUa::Variable)});
    selectAllRows(view);

    QSignalSpy readSpy(&widget, &DataAccessWidget::readRequested);
    QSignalSpy writeSpy(&widget, &DataAccessWidget::writeRequested);
    QSignalSpy monitoringSpy(&widget, &DataAccessWidget::monitoringRequested);

    auto *readButton = widget.findChild<QAbstractButton *>(QStringLiteral("readButton"));
    auto *writeButton = widget.findChild<QAbstractButton *>(QStringLiteral("writeButton"));
    QVERIFY(readButton);
    QVERIFY(writeButton);

    readButton->click();
    writeButton->click();
    widget.applySubscriptionToSelection(QStringLiteral("Default"));

    QCOMPARE(readSpy.size(), 0);
    QCOMPARE(writeSpy.size(), 0);
    QCOMPARE(monitoringSpy.size(), 0);

    widget.clearNodePending(makeDroppedNode(OpcUa::Variable).nodeId);
    widget.applySubscriptionToSelection(QStringLiteral("Default"));
    QCOMPARE(monitoringSpy.size(), 1);
}

///
/// \brief Builds three restorable rows, the first two of them subscribed.
/// \return Saved-node records for the multi-selection tests.
///
QVector<SessionNode> makeThreeSavedNodes()
{
    return {
        {QStringLiteral("ns=2;s=First"), QStringLiteral("Default"), HighlightMode::FollowDefault},
        {QStringLiteral("ns=2;s=Second"), QStringLiteral("Default"), HighlightMode::FollowDefault},
        {QStringLiteral("ns=2;s=Third"), QString(), HighlightMode::FollowDefault}
    };
}

///
/// \brief Selects the leading rows of the data view.
/// \param view Data view to select in.
/// \param count Number of rows to select from the top.
///
void selectLeadingRows(QTreeView *view, int count)
{
    QAbstractItemModel *model = view->model();
    const QItemSelection selection(model->index(0, 0),
                                   model->index(count - 1, model->columnCount() - 1));
    view->selectionModel()->select(selection,
                                   QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

///
/// \brief Del on the data view drops every selected row and stops their monitoring.
///
void TestDataAccessWidgetRows::deleteKeyRemovesEverySelectedRow()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    QVERIFY(view);
    QCOMPARE(view->selectionMode(), QAbstractItemView::ExtendedSelection);

    widget.restoreMonitoredNodes(makeThreeSavedNodes());
    widget.resize(900, 200);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    view->setFocus();

    selectLeadingRows(view, 2);

    QSignalSpy cancelSpy(&widget, &DataAccessWidget::monitoringCancelled);
    QTest::keyClick(view, Qt::Key_Delete);

    const QVector<SessionNode> remaining = widget.monitoredNodes();
    QCOMPARE(remaining.size(), 1);
    QCOMPARE(remaining.first().nodeId, QStringLiteral("ns=2;s=Third"));
    // Only the two subscribed rows had monitoring to cancel.
    QCOMPARE(cancelSpy.size(), 2);
}

///
/// \brief The toolbar Remove button drops every selected row and follows the selection.
///
void TestDataAccessWidgetRows::removeButtonDropsEverySelectedRow()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    QVERIFY(view);
    auto *removeButton = widget.findChild<QAbstractButton *>(QStringLiteral("removeButton"));
    QVERIFY(removeButton);

    widget.restoreMonitoredNodes(makeThreeSavedNodes());
    QVERIFY(!removeButton->isEnabled());

    selectLeadingRows(view, 2);
    QVERIFY(removeButton->isEnabled());
    removeButton->click();

    const QVector<SessionNode> remaining = widget.monitoredNodes();
    QCOMPARE(remaining.size(), 1);
    QCOMPARE(remaining.first().nodeId, QStringLiteral("ns=2;s=Third"));
    QVERIFY(!removeButton->isEnabled());
}

///
/// \brief Confirming the Clear prompt drops every row and stops their monitoring.
///
void TestDataAccessWidgetRows::confirmedClearRemovesEveryNode()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    QVERIFY(view);

    widget.addNodeWithDefaultSubscription(makeNodeDetails());
    widget.addPendingNodes({makeVariable(1)});
    QCOMPARE(view->model()->rowCount(), 2);

    QSignalSpy cancelSpy(&widget, &DataAccessWidget::monitoringCancelled);
    answerNextDialog(DialogButtonBox::Yes);
    widget.removeAllNodes();

    QCOMPARE(view->model()->rowCount(), 0);
    // Only the subscribed row needs its monitoring cancelled.
    QCOMPARE(cancelSpy.size(), 1);
    QCOMPARE(cancelSpy.first().first().toString(), makeNodeDetails().nodeId);
}

///
/// \brief Declining the Clear prompt leaves the table untouched.
///
void TestDataAccessWidgetRows::declinedClearKeepsEveryNode()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    QVERIFY(view);

    widget.addNodeWithDefaultSubscription(makeNodeDetails());
    widget.addPendingNodes({makeVariable(1)});

    QSignalSpy cancelSpy(&widget, &DataAccessWidget::monitoringCancelled);
    answerNextDialog(DialogButtonBox::No);
    widget.removeAllNodes();

    QCOMPARE(view->model()->rowCount(), 2);
    QCOMPARE(cancelSpy.size(), 0);
}

///
/// \brief Clearing an already empty table asks nothing.
///
void TestDataAccessWidgetRows::clearOfEmptyTableAsksNothing()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    QVERIFY(view);
    QCOMPARE(view->model()->rowCount(), 0);

    bool dialogSeen = false;
    watchForDialog(&dialogSeen);
    widget.removeAllNodes();
    QCoreApplication::processEvents();

    QVERIFY(!dialogSeen);
}

///
/// \brief A programmatic clear, as used on disconnect, asks nothing.
///
void TestDataAccessWidgetRows::silentClearAsksNothing()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    QVERIFY(view);

    widget.addNodeWithDefaultSubscription(makeNodeDetails());
    QCOMPARE(view->model()->rowCount(), 1);

    bool dialogSeen = false;
    watchForDialog(&dialogSeen);
    widget.clear();
    QCoreApplication::processEvents();

    QVERIFY(!dialogSeen);
    QCOMPARE(view->model()->rowCount(), 0);
}

///
/// \brief Typing in the filter box keeps only the rows matching the text.
///
void TestDataAccessWidgetRows::filterKeepsOnlyMatchingRows()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    auto *filterEdit = widget.findChild<QLineEdit *>(QStringLiteral("filterEdit"));
    QVERIFY(view);
    QVERIFY(filterEdit);

    widget.addPendingNodes({makeVariable(1), makeVariable(2)});
    QCOMPARE(view->model()->rowCount(), 2);

    filterEdit->setText(QStringLiteral("var2"));
    QCOMPARE(view->model()->rowCount(), 1);
    QCOMPARE(view->model()->data(view->model()->index(0, DataAccessModel::ColNodeId)).toString(),
             makeVariable(2).nodeId);
    QCOMPARE(view->model()->data(view->model()->index(0, DataAccessModel::ColNumber)).toInt(), 1);

    filterEdit->setText(QStringLiteral("ns=7"));
    QCOMPARE(view->model()->rowCount(), 0);

    filterEdit->clear();
    QCOMPARE(view->model()->rowCount(), 2);
}

///
/// \brief A digit sequence matches names, not values, timestamps or intervals.
///
void TestDataAccessWidgetRows::filterMatchesNamesOnly()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    auto *filterEdit = widget.findChild<QLineEdit *>(QStringLiteral("filterEdit"));
    QVERIFY(view);
    QVERIFY(filterEdit);

    OpcUaNodeDetails named = makeNodeDetails();
    named.nodeId = QStringLiteral("ns=2;s=Axis");
    named.displayName = QStringLiteral("Axis16Position");

    OpcUaNodeDetails valued = makeNodeDetails();
    valued.nodeId = QStringLiteral("ns=2;s=Speed");
    valued.displayName = QStringLiteral("Speed");
    valued.value = 16;

    widget.addNode(named);
    widget.addNode(valued);
    QCOMPARE(view->model()->rowCount(), 2);

    filterEdit->setText(QStringLiteral("16"));
    QCOMPARE(view->model()->rowCount(), 1);
    QCOMPARE(view->model()->data(view->model()->index(0, DataAccessModel::ColDisplayName)).toString(),
             named.displayName);
}

///
/// \brief Toolbar actions on a filtered table address the visible rows, not the hidden ones.
///
void TestDataAccessWidgetRows::actionsOnFilteredRowsUseTheirOwnNodes()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    auto *filterEdit = widget.findChild<QLineEdit *>(QStringLiteral("filterEdit"));
    QVERIFY(view);
    QVERIFY(filterEdit);

    OpcUaNodeDetails pressure = makeNodeDetails();
    pressure.nodeId = QStringLiteral("ns=2;s=Pressure");
    pressure.displayName = QStringLiteral("Pressure");
    widget.addNode(makeNodeDetails());
    widget.addNode(pressure);

    filterEdit->setText(QStringLiteral("Pressure"));
    QCOMPARE(view->model()->rowCount(), 1);
    selectAllRows(view);

    QSignalSpy readSpy(&widget, &DataAccessWidget::readRequested);
    auto *readButton = widget.findChild<QAbstractButton *>(QStringLiteral("readButton"));
    QVERIFY(readButton);
    readButton->click();

    QCOMPARE(readSpy.size(), 1);
    QCOMPARE(readSpy.first().first().toStringList(), QStringList{pressure.nodeId});
}

///
/// \brief An array node keeps its element rows while the filter and the numbering apply to nodes.
///
void TestDataAccessWidgetRows::arrayRowsExpandUnderTheirNode()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    auto *filterEdit = widget.findChild<QLineEdit *>(QStringLiteral("filterEdit"));
    QVERIFY(view);
    QVERIFY(filterEdit);

    OpcUaNodeDetails scalar = makeNodeDetails();
    OpcUaNodeDetails array = makeNodeDetails();
    array.nodeId = QStringLiteral("ns=2;s=Levels");
    array.displayName = QStringLiteral("Levels");
    array.dataTypeId = QStringLiteral("ns=0;i=4");
    array.value = QVariantList{1, 2, 3};
    widget.addNode(scalar);
    widget.addNode(array);

    QAbstractItemModel *model = view->model();
    const QModelIndex arrayRow = model->index(1, 0);
    QCOMPARE(model->rowCount(arrayRow), 3);
    QCOMPARE(model->data(model->index(0, DataAccessModel::ColNodeId, arrayRow)).toString(),
             QStringLiteral("[0]"));

    // Filtering keeps the elements with the node that matched, and only nodes are numbered.
    filterEdit->setText(QStringLiteral("Levels"));
    QCOMPARE(model->rowCount(), 1);
    const QModelIndex filteredRow = model->index(0, 0);
    QCOMPARE(model->data(model->index(0, DataAccessModel::ColNumber)).toInt(), 1);
    QCOMPARE(model->rowCount(filteredRow), 3);
    QVERIFY(model->data(model->index(0, DataAccessModel::ColNumber, filteredRow))
                .toString().isEmpty());
}

///
/// \brief Acting on a selected array element acts on the node the element belongs to.
///
void TestDataAccessWidgetRows::selectedElementRowActsOnItsNode()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    QVERIFY(view);

    OpcUaNodeDetails array = makeNodeDetails();
    array.nodeId = QStringLiteral("ns=2;s=Levels");
    array.dataTypeId = QStringLiteral("ns=0;i=4");
    array.value = QVariantList{1, 2};
    widget.addNode(array);

    QAbstractItemModel *model = view->model();
    const QModelIndex element = model->index(1, 0, model->index(0, 0));
    QVERIFY(element.isValid());
    view->selectionModel()->select(element,
                                   QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

    QSignalSpy readSpy(&widget, &DataAccessWidget::readRequested);
    auto *readButton = widget.findChild<QAbstractButton *>(QStringLiteral("readButton"));
    QVERIFY(readButton);
    QVERIFY(readButton->isEnabled());
    readButton->click();

    QCOMPARE(readSpy.size(), 1);
    QCOMPARE(readSpy.first().first().toStringList(), QStringList{array.nodeId});

    // A double click on an element writes nothing: only whole values are written.
    QSignalSpy writeSpy(&widget, &DataAccessWidget::writeRequested);
    const QModelIndex elementValue =
        model->index(1, DataAccessModel::ColValue, model->index(0, 0));
    QVERIFY(QMetaObject::invokeMethod(view, "doubleClicked", Q_ARG(QModelIndex, elementValue)));
    QCOMPARE(writeSpy.size(), 0);
}


QTEST_MAIN(TestDataAccessWidgetRows)

#include "test_dataaccesswidget_rows.moc"
