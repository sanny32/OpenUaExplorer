// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_dataaccesswidget.cpp
/// \brief Tests DataAccessWidget drag/drop behavior.
///

#include <QCoreApplication>
#include <QDateTimeEdit>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QScopedPointer>
#include <QSignalSpy>
#include <QSpinBox>
#include <QTableView>
#include <QTabWidget>
#include <QTest>
#include <QToolButton>

#include "models/addressspacemimedata.h"
#include "models/dataaccessmodel.h"
#include "widgets/themedtoolbutton.h"
#include "widgets/dataaccesswidget.h"

///
/// \brief UI tests for DataAccessWidget.
///
class TestDataAccessWidget : public QObject
{
    Q_OBJECT

private slots:
    void addressSpaceVariableDropRequestsNode();
    void addressSpaceObjectDropIsIgnored();
    void addNodeWithDefaultSubscriptionRequestsMonitoring();
    void addNodeWithExplicitSubscriptionRequestsMonitoring();
    void historyTabFollowsQtSupport();
    void historyExportButtonFollowsResults();
    void historyExportFileNameDescribesQuery();
    void historyClearButtonUsesTrashIcon();
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

} // namespace

///
/// \brief Builds node details for Data Access tests.
/// \return Node details item.
///
static OpcUaNodeDetails makeNodeDetails()
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
    QCOMPARE(widget.currentPage(), int(DataAccessWidget::DataAccessPage));
}

///
/// \brief Dropping a non-variable address-space node is ignored.
///
void TestDataAccessWidget::addressSpaceObjectDropIsIgnored()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTableView *>(QStringLiteral("dataView"));
    QVERIFY(view);

    const OpcUaNodeInfo node = makeDroppedNode(OpcUa::Object);
    QScopedPointer<QMimeData> mimeData(AddressSpaceMime::createNodeMimeData(node));
    QSignalSpy spy(&widget, &DataAccessWidget::nodeDropRequested);

    QVERIFY(!dropOnDataView(view, mimeData.data()));
    QCOMPARE(spy.size(), 0);
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
/// \brief The History page is only visible when Qt OPC UA can perform HistoryRead.
///
void TestDataAccessWidget::historyTabFollowsQtSupport()
{
    DataAccessWidget widget;
    auto *tabs = widget.findChild<QTabWidget *>(QStringLiteral("mainTabs"));
    QVERIFY(tabs);

    QCOMPARE(tabs->isTabVisible(DataAccessWidget::HistoryPage),
             OpcUa::isHistoryReadSupported());
}

///
/// \brief The history export button is enabled only when there are rows to save.
///
void TestDataAccessWidget::historyExportButtonFollowsResults()
{
    if (!OpcUa::isHistoryReadSupported())
        QSKIP("HistoryRead is not supported by this Qt OPC UA build.");

    DataAccessWidget widget;
    widget.setHistoryAvailable(true);
    auto *button = widget.findChild<QToolButton *>(QStringLiteral("historyExportButton"));
    QVERIFY(button);
    QVERIFY(!button->isEnabled());

    OpcUaHistoryValue value;
    value.value = 42;
    widget.setHistoryResults({value});
    QVERIFY(button->isEnabled());

    widget.setHistoryResults({});
    QVERIFY(!button->isEnabled());
}

///
/// \brief The suggested CSV file name includes tag, interval, and max limit.
///
void TestDataAccessWidget::historyExportFileNameDescribesQuery()
{
    if (!OpcUa::isHistoryReadSupported())
        QSKIP("HistoryRead is not supported by this Qt OPC UA build.");

    DataAccessWidget widget;
    widget.requestHistoryForNode(QStringLiteral("ns=2;s=Temperature"),
                                 QStringLiteral("Area 1/Temperature:PV"));

    auto *startEdit = widget.findChild<QDateTimeEdit *>(QStringLiteral("historyStartEdit"));
    auto *endEdit = widget.findChild<QDateTimeEdit *>(QStringLiteral("historyEndEdit"));
    auto *maxEdit = widget.findChild<QSpinBox *>(QStringLiteral("historyMaxEdit"));
    QVERIFY(startEdit);
    QVERIFY(endEdit);
    QVERIFY(maxEdit);

    startEdit->setDateTime(QDateTime(QDate(2026, 6, 25), QTime(12, 41, 36), Qt::UTC));
    endEdit->setDateTime(QDateTime(QDate(2026, 6, 25), QTime(13, 41, 36), Qt::UTC));
    maxEdit->setValue(1000);

    QCOMPARE(widget.suggestedHistoryCsvFileName(),
             QStringLiteral("Area_1_Temperature_PV_20260625_124136_20260625_134136_max1000.csv"));

    maxEdit->setValue(maxEdit->minimum());
    QCOMPARE(widget.suggestedHistoryCsvFileName(),
             QStringLiteral("Area_1_Temperature_PV_20260625_124136_20260625_134136.csv"));
}

///
/// \brief The history Clear button uses the trash icon.
///
void TestDataAccessWidget::historyClearButtonUsesTrashIcon()
{
    DataAccessWidget widget;
    auto *button = widget.findChild<ThemedToolButton *>(QStringLiteral("historyClearButton"));
    QVERIFY(button);
    QCOMPARE(button->iconName(), QStringLiteral("trash"));
}

QTEST_MAIN(TestDataAccessWidget)

#include "test_dataaccesswidget.moc"
