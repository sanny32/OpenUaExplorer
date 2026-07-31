// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_dataaccesswidget.cpp
/// \brief Tests DataAccessWidget drag/drop, subscription and value toggle behaviour.
///

#include <QCoreApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QAbstractButton>
#include <QApplication>
#include <QDialog>
#include <QFont>
#include <QLineEdit>
#include <QMimeData>
#include <QPushButton>
#include <QScopedPointer>
#include <QSignalSpy>
#include <QTableView>
#include <QTest>
#include <QTimer>

#include "models/addressspacemimedata.h"
#include "models/dataaccessmodel.h"
#include "widgets/dataaccesswidget.h"
#include "widgets/dialogbuttonbox.h"

///
/// \brief UI tests for DataAccessWidget.
///
class TestDataAccessWidget : public QObject
{
    Q_OBJECT

private slots:
    void addressSpaceVariableDropRequestsNode();
    void addressSpaceFolderDropRequestsFolder();
    void addressSpaceLeafObjectDropIsIgnored();
    void addNodeWithDefaultSubscriptionRequestsMonitoring();
    void addNodeWithExplicitSubscriptionRequestsMonitoring();
    void pendingNodesAreShownAndSettledOnClear();
    void restoredNodesKeepSavedOrderAndSubscriptions();
    void pendingRowsAreExcludedFromSelectionActions();
    void confirmedClearRemovesEveryNode();
    void declinedClearKeepsEveryNode();
    void clearOfEmptyTableAsksNothing();
    void silentClearAsksNothing();
    void filterKeepsOnlyMatchingRows();
    void filterMatchesNamesOnly();
    void actionsOnFilteredRowsUseTheirOwnNodes();
    void doubleClickTogglesWritableBooleanValue();
    void declinedDoubleClickWritesNothing();
    void doubleClickOnReadOnlyBooleanWritesNothing();
    void doubleClickOnNonBooleanWritesNothing();
    void doubleClickOutsideValueColumnWritesNothing();
    void doubleClickWhileOfflineWritesNothing();
};

namespace {

///
/// \brief Builds a node for drag/drop tests.
/// \param nodeClass OPC UA NodeClass value.
/// \return Node info item.
///
OpcUaNodeInfo makeDroppedNode(int nodeClass)
{
    OpcUaNodeInfo node;
    node.nodeId = nodeClass == OpcUa::Variable
        ? QStringLiteral("ns=2;s=Temperature")
        : QStringLiteral("ns=2;s=Device");
    node.browseName = node.nodeId;
    node.displayName = nodeClass == OpcUa::Variable
        ? QStringLiteral("Temperature")
        : QStringLiteral("Device");
    node.nodeClass = nodeClass;
    node.hasChildren = nodeClass != OpcUa::Variable;
    return node;
}

///
/// \brief Sends drag-enter and drop events to the data table viewport.
/// \param view Target table view.
/// \param mimeData Drag MIME data.
/// \return Whether the drag-enter event was accepted.
///
bool dropOnDataView(QTableView *view, const QMimeData *mimeData)
{
    QDragEnterEvent enterEvent(QPoint(4, 4), Qt::CopyAction, mimeData,
                               Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(view->viewport(), &enterEvent);

    QDropEvent dropEvent(QPointF(4, 4), Qt::CopyAction, mimeData,
                         Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(view->viewport(), &dropEvent);
    return enterEvent.isAccepted();
}

///
/// \brief Builds node details for Data Access tests.
/// \return Node details item.
///
OpcUaNodeDetails makeNodeDetails()
{
    OpcUaNodeDetails details;
    details.nodeId = QStringLiteral("ns=2;s=Temperature");
    details.displayName = QStringLiteral("Temperature");
    details.nodeClass = OpcUa::Variable;
    details.value = 21.5;
    details.dataTypeId = QStringLiteral("ns=0;i=11");
    details.status = QStringLiteral("Good");
    return details;
}

///
/// \brief Builds Boolean node details for the double-click toggle tests.
/// \param value Current value of the node.
/// \param writable Whether the UserAccessLevel grants CurrentWrite.
/// \return Node details item.
///
OpcUaNodeDetails makeBooleanNodeDetails(bool value, bool writable)
{
    OpcUaNodeDetails details = makeNodeDetails();
    details.nodeId = QStringLiteral("ns=2;s=Locked");
    details.displayName = QStringLiteral("Locked");
    details.value = value;
    details.valueType = 0;
    details.dataTypeId = QStringLiteral("ns=0;i=1");
    details.userAccessLevel = writable
        ? (OpcUa::CurrentRead | OpcUa::CurrentWrite)
        : OpcUa::CurrentRead;
    return details;
}

///
/// \brief Double-clicks the centre of a cell in the data table.
/// \param view Data table view.
/// \param row Row to click.
/// \param column Column to click.
///
/// The press and the double click are sent straight to the viewport: the view only
/// emits doubleClicked() when the double click lands on the cell it recorded on the
/// preceding press, and the offscreen platform does not synthesise that pair.
///
void doubleClickCell(QTableView *view, int row, int column)
{
    const QModelIndex index = view->model()->index(row, column);
    QVERIFY(index.isValid());
    view->scrollTo(index);

    const QPoint pos = view->visualRect(index).center();
    const QPointF global = view->viewport()->mapToGlobal(pos);
    QMouseEvent press(QEvent::MouseButtonPress, QPointF(pos), global,
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QMouseEvent doubleClick(QEvent::MouseButtonDblClick, QPointF(pos), global,
                            Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(view->viewport(), &press);
    QCoreApplication::sendEvent(view->viewport(), &doubleClick);
}

///
/// \brief Builds a variable node with a distinct NodeId.
/// \param index Index folded into the NodeId and display name.
/// \return Node info item.
///
OpcUaNodeInfo makeVariable(int index)
{
    OpcUaNodeInfo node;
    node.nodeId = QStringLiteral("ns=2;s=Var%1").arg(index);
    node.displayName = QStringLiteral("Var%1").arg(index);
    node.nodeClass = OpcUa::Variable;
    node.hasChildren = false;
    return node;
}

///
/// \brief Answers the next modal dialog, waiting for it to appear.
/// \param answer Standard button to click once the dialog is up.
///
void answerNextDialog(DialogButtonBox::StandardButton answer)
{
    QTimer::singleShot(0, qApp, [answer]() {
        auto *modal = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!modal) {
            answerNextDialog(answer);
            return;
        }
        auto *buttons = modal->findChild<DialogButtonBox *>();
        QVERIFY(buttons);
        QPushButton *button = buttons->button(answer);
        QVERIFY(button);
        QTest::mouseClick(button, Qt::LeftButton);
    });
}

///
/// \brief Dismisses a modal dialog if one appears, recording that it did.
/// \param seen Set to true when a dialog had to be dismissed.
///
/// The caller must drain the event queue afterwards so the pending check runs
/// exactly once, while \a seen is still alive.
///
void watchForDialog(bool *seen)
{
    QTimer::singleShot(0, qApp, [seen]() {
        auto *modal = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!modal)
            return;
        *seen = true;
        modal->reject();
    });
}

} // namespace

///
/// \brief Dropping a variable address-space node emits a data-access add request.
///
void TestDataAccessWidget::addressSpaceVariableDropRequestsNode()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTableView *>(QStringLiteral("dataView"));
    QVERIFY(view);

    const OpcUaNodeInfo node = makeDroppedNode(OpcUa::Variable);
    QScopedPointer<QMimeData> mimeData(AddressSpaceMime::createNodeMimeData(node));
    QSignalSpy spy(&widget, &DataAccessWidget::nodeDropRequested);

    QVERIFY(dropOnDataView(view, mimeData.data()));
    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.first().first().toString(), node.nodeId);
}

///
/// \brief Dropping a folder address-space node emits a folder add request.
///
void TestDataAccessWidget::addressSpaceFolderDropRequestsFolder()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTableView *>(QStringLiteral("dataView"));
    QVERIFY(view);

    const OpcUaNodeInfo node = makeDroppedNode(OpcUa::Object);
    QScopedPointer<QMimeData> mimeData(AddressSpaceMime::createNodeMimeData(node));
    QSignalSpy nodeSpy(&widget, &DataAccessWidget::nodeDropRequested);
    QSignalSpy folderSpy(&widget, &DataAccessWidget::folderDropRequested);

    QVERIFY(dropOnDataView(view, mimeData.data()));
    QCOMPARE(nodeSpy.size(), 0);
    QCOMPARE(folderSpy.size(), 1);
    QCOMPARE(folderSpy.first().first().toString(), node.nodeId);
}

///
/// \brief Dropping an object that cannot hold children is ignored.
///
void TestDataAccessWidget::addressSpaceLeafObjectDropIsIgnored()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTableView *>(QStringLiteral("dataView"));
    QVERIFY(view);

    OpcUaNodeInfo node = makeDroppedNode(OpcUa::Object);
    node.hasChildren = false;
    QScopedPointer<QMimeData> mimeData(AddressSpaceMime::createNodeMimeData(node));
    QSignalSpy nodeSpy(&widget, &DataAccessWidget::nodeDropRequested);
    QSignalSpy folderSpy(&widget, &DataAccessWidget::folderDropRequested);

    QVERIFY(!dropOnDataView(view, mimeData.data()));
    QCOMPARE(nodeSpy.size(), 0);
    QCOMPARE(folderSpy.size(), 0);
}

///
/// \brief Adding a dropped node assigns Default and starts monitoring at its interval.
///
void TestDataAccessWidget::addNodeWithDefaultSubscriptionRequestsMonitoring()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTableView *>(QStringLiteral("dataView"));
    QVERIFY(view);

    const OpcUaNodeDetails details = makeNodeDetails();
    QSignalSpy spy(&widget, &DataAccessWidget::monitoringRequested);

    widget.addNodeWithDefaultSubscription(details);

    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.first().at(0).toString(), details.nodeId);
    QCOMPARE(spy.first().at(1).toDouble(), SubscriptionItem().publishingInterval);
    QCOMPARE(view->model()->rowCount(), 1);
    QCOMPARE(view->model()->data(view->model()->index(0, DataAccessModel::ColSubscription)).toString(),
             QStringLiteral("Default"));
}

///
/// \brief Passing an explicit subscription uses its name and interval.
///
void TestDataAccessWidget::addNodeWithExplicitSubscriptionRequestsMonitoring()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTableView *>(QStringLiteral("dataView"));
    QVERIFY(view);

    const OpcUaNodeDetails details = makeNodeDetails();
    const SubscriptionItem subscription{QStringLiteral("Fast"), 250.0, 1};
    QSignalSpy spy(&widget, &DataAccessWidget::monitoringRequested);

    widget.addNodeWithDefaultSubscription(details, subscription);

    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.first().at(0).toString(), details.nodeId);
    QCOMPARE(spy.first().at(1).toDouble(), subscription.publishingInterval);
    QCOMPARE(view->model()->rowCount(), 1);
    QCOMPARE(view->model()->data(view->model()->index(0, DataAccessModel::ColSubscription)).toString(),
             subscription.name);
}

///
/// \brief Pending rows appear at once, read as pending, and settle when cleared.
///
void TestDataAccessWidget::pendingNodesAreShownAndSettledOnClear()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTableView *>(QStringLiteral("dataView"));
    QVERIFY(view);

    const OpcUaNodeInfo node = makeDroppedNode(OpcUa::Variable);
    widget.addPendingNodes({node});

    QAbstractItemModel *model = view->model();
    QCOMPARE(model->rowCount(), 1);
    QCOMPARE(model->data(model->index(0, DataAccessModel::ColDisplayName)).toString(),
             node.displayName);
    QVERIFY(!model->data(model->index(0, DataAccessModel::ColStatus)).toString().isEmpty());
    QVERIFY(model->data(model->index(0, DataAccessModel::ColStatus),
                        Qt::FontRole).value<QFont>().italic());
    QVERIFY(!(model->flags(model->index(0, DataAccessModel::ColSubscription))
              & Qt::ItemIsEditable));

    // A second placeholder for the same node must not duplicate the row.
    widget.addPendingNodes({node});
    QCOMPARE(model->rowCount(), 1);

    widget.addNode(makeNodeDetails());
    widget.clearNodePending(node.nodeId);

    QCOMPARE(model->data(model->index(0, DataAccessModel::ColStatus)).toString(),
             QStringLiteral("Good"));
    QVERIFY(!model->data(model->index(0, DataAccessModel::ColStatus),
                         Qt::FontRole).value<QFont>().italic());
    QVERIFY(model->flags(model->index(0, DataAccessModel::ColSubscription))
            & Qt::ItemIsEditable);
}

///
/// \brief Restored rows replace existing data and retain their saved order and subscriptions.
///
void TestDataAccessWidget::restoredNodesKeepSavedOrderAndSubscriptions()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTableView *>(QStringLiteral("dataView"));
    QVERIFY(view);

    widget.addNode(makeNodeDetails());
    const QVector<QPair<QString, QString>> savedNodes{
        {QStringLiteral("ns=2;s=Third"), QStringLiteral("Fast")},
        {QStringLiteral("ns=2;s=First"), QString()},
        {QStringLiteral("ns=2;s=Second"), QStringLiteral("Default")}
    };

    widget.restoreMonitoredNodes(savedNodes);

    QCOMPARE(widget.monitoredNodes(), savedNodes);
    QCOMPARE(view->model()->rowCount(), savedNodes.size());
    for (int row = 0; row < savedNodes.size(); ++row) {
        QCOMPARE(view->model()->data(
                     view->model()->index(row, DataAccessModel::ColNodeId)).toString(),
                 savedNodes.at(row).first);
    }
}

///
/// \brief Read, write and subscribe skip rows that are still pending.
///
void TestDataAccessWidget::pendingRowsAreExcludedFromSelectionActions()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTableView *>(QStringLiteral("dataView"));
    QVERIFY(view);

    widget.addPendingNodes({makeDroppedNode(OpcUa::Variable)});
    view->selectAll();

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
/// \brief Confirming the Clear prompt drops every row and stops their monitoring.
///
void TestDataAccessWidget::confirmedClearRemovesEveryNode()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTableView *>(QStringLiteral("dataView"));
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
void TestDataAccessWidget::declinedClearKeepsEveryNode()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTableView *>(QStringLiteral("dataView"));
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
void TestDataAccessWidget::clearOfEmptyTableAsksNothing()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTableView *>(QStringLiteral("dataView"));
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
void TestDataAccessWidget::silentClearAsksNothing()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTableView *>(QStringLiteral("dataView"));
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
void TestDataAccessWidget::filterKeepsOnlyMatchingRows()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTableView *>(QStringLiteral("dataView"));
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
void TestDataAccessWidget::filterMatchesNamesOnly()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTableView *>(QStringLiteral("dataView"));
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
void TestDataAccessWidget::actionsOnFilteredRowsUseTheirOwnNodes()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTableView *>(QStringLiteral("dataView"));
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
    view->selectAll();

    QSignalSpy readSpy(&widget, &DataAccessWidget::readRequested);
    auto *readButton = widget.findChild<QAbstractButton *>(QStringLiteral("readButton"));
    QVERIFY(readButton);
    readButton->click();

    QCOMPARE(readSpy.size(), 1);
    QCOMPARE(readSpy.first().first().toStringList(), QStringList{pressure.nodeId});
}

///
/// \brief A confirmed double click on a writable Boolean writes the inverted value.
///
void TestDataAccessWidget::doubleClickTogglesWritableBooleanValue()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTableView *>(QStringLiteral("dataView"));
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
void TestDataAccessWidget::declinedDoubleClickWritesNothing()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTableView *>(QStringLiteral("dataView"));
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
/// \brief A Boolean the user may not write is left alone.
///
void TestDataAccessWidget::doubleClickOnReadOnlyBooleanWritesNothing()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTableView *>(QStringLiteral("dataView"));
    QVERIFY(view);
    widget.addNode(makeBooleanNodeDetails(false, false));
    widget.resize(900, 200);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    QSignalSpy spy(&widget, &DataAccessWidget::valueWriteRequested);
    bool dialogSeen = false;
    watchForDialog(&dialogSeen);
    doubleClickCell(view, 0, DataAccessModel::ColValue);
    QCoreApplication::processEvents();

    QVERIFY(!dialogSeen);
    QCOMPARE(spy.size(), 0);
}

///
/// \brief Values of other types keep opening the write dialog instead of toggling.
///
void TestDataAccessWidget::doubleClickOnNonBooleanWritesNothing()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTableView *>(QStringLiteral("dataView"));
    QVERIFY(view);
    OpcUaNodeDetails details = makeNodeDetails();
    details.userAccessLevel = OpcUa::CurrentRead | OpcUa::CurrentWrite;
    widget.addNode(details);
    widget.resize(900, 200);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    QSignalSpy spy(&widget, &DataAccessWidget::valueWriteRequested);
    bool dialogSeen = false;
    watchForDialog(&dialogSeen);
    doubleClickCell(view, 0, DataAccessModel::ColValue);
    QCoreApplication::processEvents();

    QVERIFY(!dialogSeen);
    QCOMPARE(spy.size(), 0);
}

///
/// \brief Only the Value column toggles; the other columns stay inert.
///
void TestDataAccessWidget::doubleClickOutsideValueColumnWritesNothing()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTableView *>(QStringLiteral("dataView"));
    QVERIFY(view);
    widget.addNode(makeBooleanNodeDetails(false, true));
    widget.resize(900, 200);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    QSignalSpy spy(&widget, &DataAccessWidget::valueWriteRequested);
    bool dialogSeen = false;
    watchForDialog(&dialogSeen);
    doubleClickCell(view, 0, DataAccessModel::ColDisplayName);
    QCoreApplication::processEvents();

    QVERIFY(!dialogSeen);
    QCOMPARE(spy.size(), 0);
}

///
/// \brief Nothing is written while the server connection is gone.
///
void TestDataAccessWidget::doubleClickWhileOfflineWritesNothing()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTableView *>(QStringLiteral("dataView"));
    QVERIFY(view);
    widget.addNode(makeBooleanNodeDetails(false, true));
    widget.resize(900, 200);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    widget.setOffline(true);

    QSignalSpy spy(&widget, &DataAccessWidget::valueWriteRequested);
    bool dialogSeen = false;
    watchForDialog(&dialogSeen);
    doubleClickCell(view, 0, DataAccessModel::ColValue);
    QCoreApplication::processEvents();

    QVERIFY(!dialogSeen);
    QCOMPARE(spy.size(), 0);
}

QTEST_MAIN(TestDataAccessWidget)

#include "test_dataaccesswidget.moc"
