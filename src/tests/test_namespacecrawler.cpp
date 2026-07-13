// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_namespacecrawler.cpp
/// \brief Tests the per-namespace node-counting address-space crawler.
///

#include <QHash>
#include <QSignalSpy>
#include <QTest>
#include <QTimer>
#include <QVector>

#include <QOpcUaExpandedNodeId>
#include <QOpcUaLocalizedText>
#include <QOpcUaReferenceDescription>

#include "opcua/namespacecrawler.h"
#include "opcua/standardnodeid.h"

///
/// \brief Unit tests for NamespaceCrawler.
///
class TestNamespaceCrawler : public QObject
{
    Q_OBJECT

private slots:
    void countsNodesPerNamespace();
    void visitsSharedChildrenOnlyOnce();
    void cancelReportsTheCountsGatheredSoFar();
};

namespace {

/// \brief Synthetic address space: parent NodeId to its ordered child NodeIds.
using FakeTree = QHash<QString, QVector<QString>>;

///
/// \brief Returns the NodeId the crawl starts from.
/// \return Objects folder NodeId.
///
QString rootNodeId()
{
    return QString::fromLatin1(StandardNodeId::ObjectsFolder);
}

///
/// \brief Builds a browse reference for a synthetic child node.
/// \param nodeId Child NodeId.
/// \return Reference description as a browse would return.
///
QOpcUaReferenceDescription makeReference(const QString &nodeId)
{
    QOpcUaReferenceDescription reference;
    QOpcUaExpandedNodeId target;
    target.setNodeId(nodeId);
    target.setServerIndex(0);
    reference.setTargetNodeId(target);
    reference.setDisplayName(QOpcUaLocalizedText(QString(), nodeId));
    reference.setNodeClass(QOpcUa::NodeClass::Object);
    return reference;
}

///
/// \brief Crawler that browses a synthetic tree instead of a live server.
///
class FakeCrawler : public NamespaceCrawler
{
public:
    ///
    /// \brief Constructs a crawler over a synthetic tree.
    /// \param tree Synthetic address space.
    ///
    explicit FakeCrawler(FakeTree tree)
        : NamespaceCrawler(nullptr, 1000)
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
        for (const QString &child : _tree.value(nodeId))
            children.append(makeReference(child));
        QTimer::singleShot(0, this, [this, children]() { deliverChildren(children); });
        return true;
    }

private:
    FakeTree _tree;
};

} // namespace

///
/// \brief Every visited node is counted against the namespace of its NodeId.
///
void TestNamespaceCrawler::countsNodesPerNamespace()
{
    const FakeTree tree = {
        {rootNodeId(), {QStringLiteral("ns=2;s=A"), QStringLiteral("ns=3;s=B")}},
        {QStringLiteral("ns=2;s=A"), {QStringLiteral("ns=2;s=A1")}},
    };
    FakeCrawler crawler(tree);
    QSignalSpy spy(&crawler, &NamespaceCrawler::finished);

    crawler.start();
    QVERIFY(spy.wait());

    const auto counts = spy.constFirst().at(0).value<OpcUaNamespaceNodeCounts>();
    QVERIFY(spy.constFirst().at(1).toString().isEmpty());
    QCOMPARE(counts.value(0), 1); // the Objects folder itself
    QCOMPARE(counts.value(2), 2); // ns=2;s=A and ns=2;s=A1
    QCOMPARE(counts.value(3), 1); // ns=3;s=B
}

///
/// \brief A node reachable through two parents is counted and browsed exactly once.
///
void TestNamespaceCrawler::visitsSharedChildrenOnlyOnce()
{
    const QString shared = QStringLiteral("ns=2;s=Shared");
    const FakeTree tree = {
        {rootNodeId(), {QStringLiteral("ns=2;s=A"), QStringLiteral("ns=2;s=B")}},
        {QStringLiteral("ns=2;s=A"), {shared}},
        {QStringLiteral("ns=2;s=B"), {shared}},
    };
    FakeCrawler crawler(tree);
    QSignalSpy spy(&crawler, &NamespaceCrawler::finished);

    crawler.start();
    QVERIFY(spy.wait());

    const auto counts = spy.constFirst().at(0).value<OpcUaNamespaceNodeCounts>();
    QCOMPARE(counts.value(2), 3); // A, B and Shared, counted once each
    QCOMPARE(crawler.browsed.count(shared), 1);
}

///
/// \brief Cancelling ends the crawl without an error, reporting the partial counts.
///
void TestNamespaceCrawler::cancelReportsTheCountsGatheredSoFar()
{
    const FakeTree tree = {
        {rootNodeId(), {QStringLiteral("ns=2;s=A")}},
        {QStringLiteral("ns=2;s=A"), {QStringLiteral("ns=2;s=A1")}},
    };
    FakeCrawler crawler(tree);
    QSignalSpy spy(&crawler, &NamespaceCrawler::finished);

    crawler.start();
    crawler.cancel();

    QCOMPARE(spy.count(), 1);
    const auto counts = spy.constFirst().at(0).value<OpcUaNamespaceNodeCounts>();
    QVERIFY(spy.constFirst().at(1).toString().isEmpty());
    QCOMPARE(counts.value(0), 1); // only the seeded root was counted before cancelling
}

QTEST_MAIN(TestNamespaceCrawler)

#include "test_namespacecrawler.moc"
