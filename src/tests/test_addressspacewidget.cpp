// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_addressspacewidget.cpp
/// \brief Tests AddressSpaceWidget selection-dependent detail views.
///

#include <QAbstractItemModel>
#include <QSignalSpy>
#include <QTableView>
#include <QTest>
#include <QTreeView>

#include "widgets/addressspacewidget.h"

///
/// \brief UI tests for AddressSpaceWidget.
///
class TestAddressSpaceWidget : public QObject
{
    Q_OBJECT

private slots:
    void selectionRefreshesReferences();
};

namespace {

///
/// \brief Builds an OPC UA node info item for widget tests.
/// \param nodeId NodeId string.
/// \param displayName DisplayName text.
/// \param referenceTypeId Reference type NodeId.
/// \param hasChildren Whether the node may have children.
/// \return Node info item.
///
OpcUaNodeInfo makeNode(const QString &nodeId, const QString &displayName,
                       const QString &referenceTypeId = QString(), bool hasChildren = false)
{
    OpcUaNodeInfo node;
    node.nodeId = nodeId;
    node.browseName = displayName;
    node.displayName = displayName;
    node.referenceTypeId = referenceTypeId;
    node.nodeClass = hasChildren ? 1 : 2;
    node.hasChildren = hasChildren;
    return node;
}

} // namespace

///
/// \brief Selecting another node replaces the references table instead of leaving stale rows.
///
void TestAddressSpaceWidget::selectionRefreshesReferences()
{
    AddressSpaceWidget widget;
    auto *tree = widget.findChild<QTreeView *>(QStringLiteral("addressTree"));
    auto *references = widget.findChild<QTableView *>(QStringLiteral("referencesTable"));
    QVERIFY(tree);
    QVERIFY(references);
    QSignalSpy referencesSpy(&widget, &AddressSpaceWidget::referencesRequested);

    const OpcUaNodeInfo root = makeNode(QStringLiteral("ns=0;i=84"),
                                        QStringLiteral("Root"), QString(), true);
    const OpcUaNodeInfo device = makeNode(QStringLiteral("ns=2;s=Device"),
                                          QStringLiteral("Device"),
                                          QStringLiteral("ns=0;i=35"), true);
    const OpcUaNodeInfo levelViaOrganizes = makeNode(QStringLiteral("ns=2;s=Level"),
                                                     QStringLiteral("Level"),
                                                     QStringLiteral("ns=0;i=35"));
    const OpcUaNodeInfo levelViaHasComponent = makeNode(QStringLiteral("ns=2;s=Level"),
                                                        QStringLiteral("Level"),
                                                        QStringLiteral("ns=0;i=47"));

    widget.setRootNode(root);
    widget.setBrowseChildren(root.nodeId, {device}, QString());
    widget.setBrowseReferences(root.nodeId, {device, levelViaOrganizes,
                                             levelViaHasComponent}, QString());

    QCOMPARE(references->model()->rowCount(), 3);
    QCOMPARE(references->model()->data(references->model()->index(2, 0)).toString(),
             QStringLiteral("HasComponent"));

    const QModelIndex rootIndex = tree->model()->index(0, 0);
    const QModelIndex deviceIndex = tree->model()->index(0, 0, rootIndex);
    QVERIFY(deviceIndex.isValid());

    const int requestsBeforeDeviceSelection = referencesSpy.size();
    tree->setCurrentIndex(deviceIndex);
    QCOMPARE(referencesSpy.size(), requestsBeforeDeviceSelection + 1);
    QCOMPARE(referencesSpy.last().first().toString(), device.nodeId);
    QCOMPARE(references->model()->rowCount(), 0);

    const OpcUaNodeInfo alarm = makeNode(QStringLiteral("ns=2;s=Alarm"),
                                         QStringLiteral("Alarm"),
                                         QStringLiteral("ns=0;i=47"));
    widget.setBrowseChildren(device.nodeId, {alarm}, QString());
    widget.setBrowseReferences(device.nodeId, {alarm}, QString());
    QCOMPARE(references->model()->rowCount(), 1);
    QCOMPARE(references->model()->data(references->model()->index(0, 1)).toString(),
             alarm.displayName);

    const QModelIndex alarmIndex = tree->model()->index(0, 0, deviceIndex);
    QVERIFY(alarmIndex.isValid());
    tree->setCurrentIndex(alarmIndex);
    QCOMPARE(references->model()->rowCount(), 0);

    tree->setCurrentIndex(rootIndex);
    QCOMPARE(references->model()->rowCount(), 3);
}

QTEST_MAIN(TestAddressSpaceWidget)

#include "test_addressspacewidget.moc"
