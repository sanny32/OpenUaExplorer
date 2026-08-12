// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_subtreevariablecrawler.cpp
/// \brief Tests the crawler that collects a subtree's variables.
///

#include <QHash>
#include <QSignalSpy>
#include <QTest>
#include <QTimer>
#include <QVector>

#include <QOpcUaExpandedNodeId>
#include <QOpcUaLocalizedText>
#include <QOpcUaQualifiedName>
#include <QOpcUaReferenceDescription>

#include "opcua/subtreevariablecrawler.h"

///
/// \brief Unit tests for SubtreeVariableCrawler.
///
class TestSubtreeVariableCrawler : public QObject
{
    Q_OBJECT

private slots:
    void collectsVariablesOfTheWholeSubtree();
    void skipsNonVariableNodesAndRevisitedTargets();
    void reportsNothingForASubtreeWithoutVariables();
    void stopsBrowsingOnceTheVariableBudgetIsSpent();
};

namespace {

/// \brief One synthetic child: NodeId, DisplayName, NodeClass.
struct FakeChild
{
    QString nodeId;
    QString displayName;
    QOpcUa::NodeClass nodeClass = QOpcUa::NodeClass::Object;
};

/// \brief Synthetic address space: parent NodeId to ordered children.
using FakeTree = QHash<QString, QVector<FakeChild>>;

///
/// \brief Builds a browse reference for a synthetic child node.
/// \param child Synthetic child to encode.
/// \return Reference description as a browse would return.
///
QOpcUaReferenceDescription makeReference(const FakeChild &child)
{
    QOpcUaReferenceDescription reference;
    QOpcUaExpandedNodeId target;
    target.setNodeId(child.nodeId);
    target.setServerIndex(0);
    reference.setTargetNodeId(target);
    reference.setDisplayName(QOpcUaLocalizedText(QString(), child.displayName));
    reference.setBrowseName(QOpcUaQualifiedName(0, child.displayName));
    reference.setNodeClass(child.nodeClass);
    return reference;
}

///
/// \brief Crawler that browses a synthetic tree instead of a live server.
///
class FakeCrawler : public SubtreeVariableCrawler
{
public:
    ///
    /// \brief Constructs a crawler over a synthetic tree.
    /// \param tree Synthetic address space.
    /// \param rootNodeId Node whose subtree is collected.
    ///
    FakeCrawler(FakeTree tree, const QString &rootNodeId)
        : SubtreeVariableCrawler(nullptr, rootNodeId, 1000)
        , _tree(std::move(tree))
    {
    }

    /// \brief Node ids whose children were browsed, in visit order.
    QStringList browsed;

protected:
    bool clientAvailable() const override
    {
        return true;
    }

    bool startBrowse(const QString &nodeId) override
    {
        browsed.append(nodeId);
        QVector<QOpcUaReferenceDescription> children;
        for (const FakeChild &child : _tree.value(nodeId))
            children.append(makeReference(child));
        QTimer::singleShot(0, this, [this, children]() { deliverChildren(children); });
        return true;
    }

private:
    FakeTree _tree;
};

///
/// \brief Synthetic tree with variables spread over three levels.
/// \return Synthetic address space.
///
FakeTree deviceTree()
{
    return {
        {QStringLiteral("root"), {{QStringLiteral("temp"), QStringLiteral("Temperature"),
                                   QOpcUa::NodeClass::Variable},
                                  {QStringLiteral("limits"), QStringLiteral("Limits")}}},
        {QStringLiteral("limits"), {{QStringLiteral("min"), QStringLiteral("Min"),
                                     QOpcUa::NodeClass::Variable},
                                    {QStringLiteral("deep"), QStringLiteral("Deep")}}},
        {QStringLiteral("deep"), {{QStringLiteral("max"), QStringLiteral("Max"),
                                   QOpcUa::NodeClass::Variable}}},
    };
}

///
/// \brief Waits for finished() and returns the variables it reported.
/// \param spy Spy watching SubtreeVariableCrawler::finished.
/// \return Reported variables, empty when the crawler never reported.
///
QVector<OpcUaNodeInfo> collectedVariables(QSignalSpy &spy)
{
    if (spy.isEmpty() && !spy.wait(2000))
        return {};
    return spy.takeFirst().at(1).value<QVector<OpcUaNodeInfo>>();
}

///
/// \brief Reduces reported variables to their NodeIds.
/// \param variables Variables to list.
/// \return NodeIds in report order.
///
QStringList nodeIdsOf(const QVector<OpcUaNodeInfo> &variables)
{
    QStringList nodeIds;
    for (const OpcUaNodeInfo &variable : variables)
        nodeIds.append(variable.nodeId);
    return nodeIds;
}

} // namespace

///
/// \brief Variables are collected breadth-first, however deep they sit.
///
void TestSubtreeVariableCrawler::collectsVariablesOfTheWholeSubtree()
{
    FakeCrawler crawler(deviceTree(), QStringLiteral("root"));
    QSignalSpy spy(&crawler, &SubtreeVariableCrawler::finished);

    crawler.start();

    const QVector<OpcUaNodeInfo> variables = collectedVariables(spy);
    QCOMPARE(nodeIdsOf(variables), QStringList({QStringLiteral("temp"), QStringLiteral("min"),
                                                QStringLiteral("max")}));
    QCOMPARE(variables.first().displayName, QStringLiteral("Temperature"));
    QCOMPARE(variables.first().browseName, QStringLiteral("Temperature"));
    QVERIFY(OpcUa::isVariable(variables.first().nodeClass));
}

///
/// \brief Objects and methods are browsed but never collected, and each node is visited once.
///
void TestSubtreeVariableCrawler::skipsNonVariableNodesAndRevisitedTargets()
{
    FakeTree tree = {
        {QStringLiteral("root"), {{QStringLiteral("call"), QStringLiteral("Call"),
                                   QOpcUa::NodeClass::Method},
                                  {QStringLiteral("folder"), QStringLiteral("Folder")},
                                  {QStringLiteral("shared"), QStringLiteral("Shared"),
                                   QOpcUa::NodeClass::Variable}}},
        {QStringLiteral("call"), {{QStringLiteral("args"), QStringLiteral("InputArguments"),
                                   QOpcUa::NodeClass::Variable}}},
        {QStringLiteral("folder"), {{QStringLiteral("shared"), QStringLiteral("Shared"),
                                     QOpcUa::NodeClass::Variable}}},
    };
    FakeCrawler crawler(std::move(tree), QStringLiteral("root"));
    QSignalSpy spy(&crawler, &SubtreeVariableCrawler::finished);

    crawler.start();

    QCOMPARE(nodeIdsOf(collectedVariables(spy)),
             QStringList({QStringLiteral("shared"), QStringLiteral("args")}));
}

///
/// \brief A subtree of objects alone reports an empty result without an error.
///
void TestSubtreeVariableCrawler::reportsNothingForASubtreeWithoutVariables()
{
    FakeTree tree = {
        {QStringLiteral("root"), {{QStringLiteral("a"), QStringLiteral("A")}}},
    };
    FakeCrawler crawler(std::move(tree), QStringLiteral("root"));
    QSignalSpy spy(&crawler, &SubtreeVariableCrawler::finished);

    crawler.start();

    QVERIFY(spy.isEmpty() ? spy.wait(2000) : true);
    const QList<QVariant> result = spy.takeFirst();
    QCOMPARE(result.at(0).toString(), QStringLiteral("root"));
    QVERIFY(result.at(1).value<QVector<OpcUaNodeInfo>>().isEmpty());
    QVERIFY(result.at(2).toString().isEmpty());
}

///
/// \brief The variables gathered before the budget ran out are still reported.
///
void TestSubtreeVariableCrawler::stopsBrowsingOnceTheVariableBudgetIsSpent()
{
    QVector<FakeChild> children;
    for (int i = 0; i < SubtreeVariableCrawler::MaxVariables + 10; ++i) {
        children.append({QStringLiteral("var%1").arg(i), QStringLiteral("Var%1").arg(i),
                         QOpcUa::NodeClass::Variable});
    }
    FakeTree tree = {{QStringLiteral("root"), children}};
    FakeCrawler crawler(std::move(tree), QStringLiteral("root"));
    QSignalSpy spy(&crawler, &SubtreeVariableCrawler::finished);

    crawler.start();

    const QVector<OpcUaNodeInfo> variables = collectedVariables(spy);
    QCOMPARE(variables.size(), SubtreeVariableCrawler::MaxVariables + 10);
    QCOMPARE(crawler.browsed, QStringList{QStringLiteral("root")});
}

QTEST_MAIN(TestSubtreeVariableCrawler)

#include "test_subtreevariablecrawler.moc"
