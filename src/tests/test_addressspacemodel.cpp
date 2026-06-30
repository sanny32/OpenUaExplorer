// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_addressspacemodel.cpp
/// \brief Tests the lazy OPC UA address-space tree model.
///

#include <QIcon>
#include <QMimeData>
#include <QScopedPointer>
#include <QSignalSpy>
#include <QTest>

#include "models/addressspacemimedata.h"
#include "models/addressspacemodel.h"
#include "opcua/standardnodeid.h"

namespace {

///
/// \brief Builds a browsable root node for the address space tests.
/// \return Root node metadata.
///
OpcUaNodeInfo makeRoot()
{
    OpcUaNodeInfo root;
    root.nodeId = QString::fromLatin1(StandardNodeId::ObjectsFolder);
    root.displayName = QStringLiteral("Root");
    root.nodeClass = 1;
    root.hasChildren = true;
    return root;
}

} // namespace

///
/// \brief Unit tests for AddressSpaceModel.
///
class TestAddressSpaceModel : public QObject
{
    Q_OBJECT

private slots:
    void fetchMoreEmitsBrowseOnce();
    void browseFailedAllowsRetry();
    void leafDoesNotFetch();
    void findByNodeIdAndDisplayName();
    void setChildrenDeduplicatesNodeIds();
    void dragMimeIncludesNodesWithNodeId();
    void dataRolesAndTreeOps();
};

///
/// \brief fetchMore requests a browse once; a second call is a no-op.
///
void TestAddressSpaceModel::fetchMoreEmitsBrowseOnce()
{
    AddressSpaceModel model;
    model.setRootNode(makeRoot());

    const QModelIndex rootIndex = model.index(0, 0);
    QVERIFY(model.canFetchMore(rootIndex));

    QSignalSpy browseSpy(&model, &AddressSpaceModel::browseRequested);
    model.fetchMore(rootIndex);
    model.fetchMore(rootIndex);
    QCOMPARE(browseSpy.size(), 1);
}

///
/// \brief A failed browse clears the in-flight flag so the node can be retried.
///
void TestAddressSpaceModel::browseFailedAllowsRetry()
{
    AddressSpaceModel model;
    model.setRootNode(makeRoot());

    const QModelIndex rootIndex = model.index(0, 0);
    model.fetchMore(rootIndex);
    QVERIFY(!model.canFetchMore(rootIndex));

    model.setBrowseFailed(makeRoot().nodeId);
    QVERIFY(model.canFetchMore(rootIndex));
}

///
/// \brief A node declared without children never offers a fetch.
///
void TestAddressSpaceModel::leafDoesNotFetch()
{
    AddressSpaceModel model;
    OpcUaNodeInfo leaf = makeRoot();
    leaf.hasChildren = false;
    model.setRootNode(leaf);

    const QModelIndex rootIndex = model.index(0, 0);
    QVERIFY(!model.canFetchMore(rootIndex));
    QVERIFY(!model.hasChildren(rootIndex));
}

///
/// \brief findByNodeId and findFirst locate nodes after children are applied.
///
void TestAddressSpaceModel::findByNodeIdAndDisplayName()
{
    AddressSpaceModel model;
    model.setRootNode(makeRoot());
    model.fetchMore(model.index(0, 0));

    OpcUaNodeInfo child;
    child.nodeId = QStringLiteral("ns=0;i=85");
    child.displayName = QStringLiteral("Objects");
    child.nodeClass = 1;
    model.setChildren(makeRoot().nodeId, {child});

    const QModelIndex byId = model.findByNodeId(child.nodeId);
    QVERIFY(byId.isValid());
    QCOMPARE(model.nodeInfo(byId).displayName, child.displayName);
    QCOMPARE(model.nodeInfo(byId).displayPath, QStringLiteral("Root/Objects"));

    const QModelIndex byName = model.findFirst(child.displayName);
    QVERIFY(byName.isValid());
    QCOMPARE(byName, byId);

    QVERIFY(!model.findByNodeId(QStringLiteral("ns=0;i=9999")).isValid());
}

///
/// \brief setChildren keeps one sibling for repeated NodeIds.
///
void TestAddressSpaceModel::setChildrenDeduplicatesNodeIds()
{
    AddressSpaceModel model;
    model.setRootNode(makeRoot());

    OpcUaNodeInfo first;
    first.nodeId = QStringLiteral("ns=2;s=MyDevice");
    first.displayName = QStringLiteral("MyDevice");
    first.referenceTypeId = QStringLiteral("ns=0;i=47");
    first.nodeClass = 1;

    OpcUaNodeInfo duplicate = first;
    duplicate.referenceTypeId = QStringLiteral("ns=0;i=35");

    OpcUaNodeInfo sameNameDifferentNode;
    sameNameDifferentNode.nodeId = QStringLiteral("ns=2;s=OtherDevice");
    sameNameDifferentNode.displayName = QStringLiteral("MyDevice");
    sameNameDifferentNode.nodeClass = 1;

    model.setChildren(makeRoot().nodeId, {first, duplicate, sameNameDifferentNode});

    const QModelIndex rootIndex = model.index(0, 0);
    QCOMPARE(model.rowCount(rootIndex), 2);
    QCOMPARE(model.nodeInfo(model.index(0, 0, rootIndex)).referenceTypeId,
             first.referenceTypeId);
    QCOMPARE(model.nodeInfo(model.index(1, 0, rootIndex)).nodeId,
             sameNameDifferentNode.nodeId);
}

///
/// \brief Drag MIME is exported for any node that carries a NodeId.
///
void TestAddressSpaceModel::dragMimeIncludesNodesWithNodeId()
{
    AddressSpaceModel model;
    model.setRootNode(makeRoot());

    OpcUaNodeInfo variable;
    variable.nodeId = QStringLiteral("ns=2;s=Temperature");
    variable.browseName = QStringLiteral("2:Temperature");
    variable.displayName = QStringLiteral("Temperature");
    variable.nodeClass = OpcUa::Variable;
    variable.historizing = true;

    OpcUaNodeInfo object;
    object.nodeId = QStringLiteral("ns=2;s=Device");
    object.displayName = QStringLiteral("Device");
    object.nodeClass = OpcUa::Object;
    object.eventNotifier = OpcUa::SubscribeToEvents | OpcUa::HistoryRead;

    model.setChildren(makeRoot().nodeId, {variable, object});

    const QModelIndex root = model.index(0, 0);
    const QModelIndex variableIndex = model.index(0, 0, root);
    const QModelIndex objectIndex = model.index(1, 0, root);
    QVERIFY(model.mimeTypes().contains(AddressSpaceMime::nodeMimeType()));
    QVERIFY(model.supportedDragActions().testFlag(Qt::CopyAction));
    QVERIFY(model.flags(variableIndex).testFlag(Qt::ItemIsDragEnabled));
    QVERIFY(model.flags(objectIndex).testFlag(Qt::ItemIsDragEnabled));

    QScopedPointer<QMimeData> variableMime(model.mimeData({variableIndex}));
    OpcUaNodeInfo decoded;
    QVERIFY(AddressSpaceMime::decodeNode(variableMime.data(), &decoded));
    QCOMPARE(decoded.nodeId, variable.nodeId);
    QCOMPARE(decoded.displayName, variable.displayName);
    QCOMPARE(decoded.displayPath, QStringLiteral("Root/Temperature"));
    QCOMPARE(decoded.nodeClass, variable.nodeClass);
    QCOMPARE(decoded.historizing, variable.historizing);

    QScopedPointer<QMimeData> objectMime(model.mimeData({objectIndex}));
    QVERIFY(AddressSpaceMime::decodeNode(objectMime.data(), &decoded));
    QCOMPARE(decoded.nodeId, object.nodeId);
    QCOMPARE(decoded.nodeClass, object.nodeClass);
    QCOMPARE(decoded.eventNotifier, object.eventNotifier);
}

///
/// \brief AddressSpaceModel: data roles, the parent chain, icons and tree edits.
///
void TestAddressSpaceModel::dataRolesAndTreeOps()
{
    AddressSpaceModel model;

    const QVector<AddressSpaceItem> items = {
        {QStringLiteral("Objects"), AddressSpaceItem::NodeType::Folder,
         {{QStringLiteral("Temp"), AddressSpaceItem::NodeType::Variable, {}}}},
        {QStringLiteral("DoIt"), AddressSpaceItem::NodeType::Method, {}},
        {QStringLiteral("Plain"), AddressSpaceItem::NodeType::Node, {}},
    };
    model.setItems(items);
    QCOMPARE(model.rowCount(), 3);

    const QModelIndex folder = model.index(0, 0);
    const QModelIndex method = model.index(1, 0);
    const QModelIndex temp = model.index(0, 0, folder);
    QVERIFY(temp.isValid());

    QCOMPARE(model.parent(temp), folder);
    QVERIFY(!model.parent(folder).isValid());
    QVERIFY(!model.parent(QModelIndex()).isValid());

    QCOMPARE(model.data(folder, Qt::DisplayRole).toString(), QStringLiteral("Objects"));
    QCOMPARE(model.data(folder, Qt::ToolTipRole).toString(),
             model.data(folder, Qt::UserRole).toString());
    QVERIFY(!model.data(folder, Qt::SizeHintRole).isValid());
    QVERIFY(!model.data(QModelIndex()).isValid());

    QVERIFY(!model.data(folder, Qt::DecorationRole).isValid());
    model.setIconProvider([](AddressSpaceItem::NodeType) { return QIcon(); });
    model.data(folder, Qt::DecorationRole);
    model.data(temp, Qt::DecorationRole);
    model.data(method, Qt::DecorationRole);

    QVERIFY(!model.index(99, 0).isValid());
    QVERIFY(!model.findFirst(QStringLiteral("nope")).isValid());
    QVERIFY(!model.canFetchMore(QModelIndex()));
    model.setChildren(QStringLiteral("does-not-exist"), {});

    model.setRootNode(makeRoot());
    const QModelIndex rootIndex = model.index(0, 0);
    model.fetchMore(rootIndex);

    OpcUaNodeInfo first;
    first.nodeId = QStringLiteral("ns=0;i=85");
    first.displayName = QStringLiteral("Objects");
    first.nodeClass = 1;
    model.setChildren(makeRoot().nodeId, {first});
    QCOMPARE(model.rowCount(rootIndex), 1);

    OpcUaNodeInfo second;
    second.nodeId = QStringLiteral("ns=0;i=86");
    second.displayName = QStringLiteral("Types");
    second.nodeClass = 1;
    model.setChildren(makeRoot().nodeId, {second});
    QCOMPARE(model.rowCount(rootIndex), 1);
    QCOMPARE(model.findByNodeId(second.nodeId).isValid(), true);

    model.clear();
    QCOMPARE(model.rowCount(), 0);
}

QTEST_MAIN(TestAddressSpaceModel)

#include "test_addressspacemodel.moc"
