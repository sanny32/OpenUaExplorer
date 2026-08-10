// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_dataaccesswidget_add_restore.cpp
/// \brief Tests DataAccessWidget drops, additions, and restoration.
///

#include <QTest>

#include "test_dataaccesswidget_support.h"

///
/// \brief Tests the related behavior in an isolated Qt Test executable.
///
class TestDataAccessWidgetAddRestore : public QObject
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
    void restoredNodesFollowTheSubscriptionNamesOfTheCurrentLanguage();
};

///
/// \brief Dropping a variable address-space node emits a data-access add request.
///
void TestDataAccessWidgetAddRestore::addressSpaceVariableDropRequestsNode()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
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
void TestDataAccessWidgetAddRestore::addressSpaceFolderDropRequestsFolder()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
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
void TestDataAccessWidgetAddRestore::addressSpaceLeafObjectDropIsIgnored()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
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
void TestDataAccessWidgetAddRestore::addNodeWithDefaultSubscriptionRequestsMonitoring()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
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
void TestDataAccessWidgetAddRestore::addNodeWithExplicitSubscriptionRequestsMonitoring()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
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
void TestDataAccessWidgetAddRestore::pendingNodesAreShownAndSettledOnClear()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
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
void TestDataAccessWidgetAddRestore::restoredNodesKeepSavedOrderAndSubscriptions()
{
    DataAccessWidget widget;
    auto *view = widget.findChild<QTreeView *>(QStringLiteral("dataView"));
    QVERIFY(view);

    widget.addNode(makeNodeDetails());
    const QVector<SessionNode> savedNodes{
        {QStringLiteral("ns=2;s=Third"), QStringLiteral("Fast"), HighlightMode::FollowDefault},
        {QStringLiteral("ns=2;s=First"), QString(), HighlightMode::Disabled},
        {QStringLiteral("ns=2;s=Second"), QStringLiteral("Default"), HighlightMode::Enabled}
    };

    widget.restoreMonitoredNodes(savedNodes);

    QCOMPARE(widget.monitoredNodes(), savedNodes);
    QCOMPARE(view->model()->rowCount(), savedNodes.size());
    for (int row = 0; row < savedNodes.size(); ++row) {
        QCOMPARE(view->model()->data(
                     view->model()->index(row, DataAccessModel::ColNodeId)).toString(),
                 savedNodes.at(row).nodeId);
    }
}

///
/// \brief A session saved under another language restores onto the current subscription names.
///
void TestDataAccessWidgetAddRestore::restoredNodesFollowTheSubscriptionNamesOfTheCurrentLanguage()
{
    class SuffixTranslator : public QTranslator
    {
    public:
        QString translate(const char *context, const char *sourceText,
                          const char *disambiguation = nullptr, int n = -1) const override
        {
            Q_UNUSED(disambiguation)
            Q_UNUSED(n)
            if (qstrcmp(context, "SubscriptionsWidget") != 0)
                return QString();
            return QString::fromUtf8(sourceText) + QStringLiteral("-xx");
        }

        bool isEmpty() const override { return false; }
    };

    SuffixTranslator translator;
    QCoreApplication::installTranslator(&translator);

    SubscriptionItem builtin;
    builtin.id = DefaultSubscriptionId;
    builtin.builtin = true;
    builtin.name = QStringLiteral("Default-xx");
    SubscriptionItem custom;
    custom.id = 7;
    custom.name = QStringLiteral("Fast");

    DataAccessWidget widget;
    widget.setSubscriptions({builtin, custom});
    widget.restoreMonitoredNodes({
        {QStringLiteral("ns=2;s=First"), QStringLiteral("Default"), HighlightMode::FollowDefault},
        {QStringLiteral("ns=2;s=Second"), QStringLiteral("Fast"), HighlightMode::FollowDefault},
        {QStringLiteral("ns=2;s=Third"), QStringLiteral("Telemetry"), HighlightMode::FollowDefault}
    });

    const QVector<SessionNode> restored = widget.monitoredNodes();
    QCOMPARE(restored.at(0).subscriptionName, QStringLiteral("Default-xx"));
    QCOMPARE(restored.at(1).subscriptionName, QStringLiteral("Fast"));
    QCOMPARE(restored.at(2).subscriptionName, QStringLiteral("Telemetry"));

    QCoreApplication::removeTranslator(&translator);
}


QTEST_MAIN(TestDataAccessWidgetAddRestore)

#include "test_dataaccesswidget_add_restore.moc"
