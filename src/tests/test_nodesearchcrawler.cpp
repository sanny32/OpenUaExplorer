// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file test_nodesearchcrawler.cpp
/// \brief Tests the breadth-first address-space search crawler.
///

#include <QHash>
#include <QSignalSpy>
#include <QTest>
#include <QTimer>
#include <QVector>

#include <QOpcUaExpandedNodeId>
#include <QOpcUaLocalizedText>
#include <QOpcUaReferenceDescription>

#include "opcua/nodesearchcrawler.h"

///
/// \brief Unit tests for NodeSearchCrawler.
///
class TestNodeSearchCrawler : public QObject
{
    Q_OBJECT

private slots:
    void reportsMatchesBreadthFirst();
    void resumeWalksSiblingsBeforeDescending();
    void resumeSearchesInsideAMatchedNode();
    void reportsNoMatchWhenSubtreeExhausted();
};

namespace {

/// \brief Synthetic address space: parent NodeId to ordered child (nodeId, displayName) pairs.
using FakeTree = QHash<QString, QVector<QPair<QString, QString>>>;

///
/// \brief Builds a browse reference for a synthetic child node.
/// \param nodeId Child NodeId.
/// \param displayName Child DisplayName.
/// \return Reference description as a browse would return.
///
QOpcUaReferenceDescription makeReference(const QString &nodeId, const QString &displayName)
{
    QOpcUaReferenceDescription reference;
    QOpcUaExpandedNodeId target;
    target.setNodeId(nodeId);
    target.setServerIndex(0);
    reference.setTargetNodeId(target);
    reference.setDisplayName(QOpcUaLocalizedText(QString(), displayName));
    reference.setNodeClass(QOpcUa::NodeClass::Object);
    return reference;
}

///
/// \brief Crawler that browses a synthetic tree instead of a live server.
///
class FakeCrawler : public NodeSearchCrawler
{
public:
    ///
    /// \brief Constructs a crawler over a synthetic tree.
    /// \param tree Synthetic address space.
    /// \param startNodeId Node whose subtree is searched.
    /// \param pattern Case-insensitive substring matched against display names.
    ///
    FakeCrawler(FakeTree tree, const QString &startNodeId, const QString &pattern)
        : NodeSearchCrawler(nullptr, startNodeId, pattern, 1000)
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
        for (const auto &child : _tree.value(nodeId))
            children.append(makeReference(child.first, child.second));
        QTimer::singleShot(0, this, [this, children]() { deliverChildren(children); });
        return true;
    }

private:
    FakeTree _tree;
};

///
/// \brief Synthetic tree with matches spread across breadth and depth.
///
/// Root has three children; "AlphaLevel" and "Gamma" are siblings, and a deeper
/// "DeepLevel" lives under the matching node itself.
/// \return Synthetic address space.
///
FakeTree levelTree()
{
    return {
        {QStringLiteral("root"), {{QStringLiteral("a"), QStringLiteral("AlphaLevel")},
                                  {QStringLiteral("b"), QStringLiteral("Beta")},
                                  {QStringLiteral("c"), QStringLiteral("GammaLevel")}}},
        {QStringLiteral("a"), {{QStringLiteral("a1"), QStringLiteral("DeepLevel")}}},
        {QStringLiteral("b"), {{QStringLiteral("b1"), QStringLiteral("Quiet")}}},
    };
}

///
/// \brief Waits for the next finished() emission and takes its arguments.
/// \param spy Spy watching NodeSearchCrawler::finished.
/// \return Emitted arguments, empty when the crawler never reported.
///
QList<QVariant> nextResult(QSignalSpy &spy)
{
    if (spy.isEmpty() && !spy.wait(2000))
        return {};
    return spy.takeFirst();
}

///
/// \brief Waits for the next finished() emission and returns its matched NodeId.
/// \param spy Spy watching NodeSearchCrawler::finished.
/// \return Matched NodeId, or a null string when the crawler never reported.
///
QString nextMatch(QSignalSpy &spy)
{
    const QList<QVariant> result = nextResult(spy);
    return result.isEmpty() ? QString() : result.at(1).toString();
}

} // namespace

///
/// \brief Matches on one level are reported in browse order before deeper matches.
///
void TestNodeSearchCrawler::reportsMatchesBreadthFirst()
{
    FakeCrawler crawler(levelTree(), QStringLiteral("root"), QStringLiteral("level"));
    QSignalSpy spy(&crawler, &NodeSearchCrawler::finished);

    crawler.start();

    QCOMPARE(nextMatch(spy), QStringLiteral("a"));
    QVERIFY(crawler.isPaused());
}

///
/// \brief Resuming reports the matching sibling before descending into the first match.
///
void TestNodeSearchCrawler::resumeWalksSiblingsBeforeDescending()
{
    FakeCrawler crawler(levelTree(), QStringLiteral("root"), QStringLiteral("level"));
    QSignalSpy spy(&crawler, &NodeSearchCrawler::finished);

    crawler.start();
    QCOMPARE(nextMatch(spy), QStringLiteral("a"));

    crawler.resume();
    QCOMPARE(nextMatch(spy), QStringLiteral("c"));
    QVERIFY(crawler.isPaused());
}

///
/// \brief A matched node's own subtree is still searched on a later resume.
///
void TestNodeSearchCrawler::resumeSearchesInsideAMatchedNode()
{
    FakeCrawler crawler(levelTree(), QStringLiteral("root"), QStringLiteral("level"));
    QSignalSpy spy(&crawler, &NodeSearchCrawler::finished);

    crawler.start();
    QCOMPARE(nextMatch(spy), QStringLiteral("a"));
    crawler.resume();
    QCOMPARE(nextMatch(spy), QStringLiteral("c"));

    crawler.resume();
    const QList<QVariant> result = nextResult(spy);
    QVERIFY(!result.isEmpty());
    QCOMPARE(result.at(1).toString(), QStringLiteral("a1"));
    QCOMPARE(result.at(0).toStringList(),
             QStringList({QStringLiteral("root"), QStringLiteral("a")}));
}

///
/// \brief Resuming past the last match reports exhaustion instead of a match.
///
void TestNodeSearchCrawler::reportsNoMatchWhenSubtreeExhausted()
{
    FakeCrawler crawler(levelTree(), QStringLiteral("root"), QStringLiteral("level"));
    QSignalSpy spy(&crawler, &NodeSearchCrawler::finished);

    crawler.start();
    QCOMPARE(nextMatch(spy), QStringLiteral("a"));
    crawler.resume();
    QCOMPARE(nextMatch(spy), QStringLiteral("c"));
    crawler.resume();
    QCOMPARE(nextMatch(spy), QStringLiteral("a1"));

    crawler.resume();
    const QList<QVariant> result = nextResult(spy);
    QVERIFY(!result.isEmpty());
    QVERIFY(result.at(1).toString().isEmpty());
    QVERIFY(result.at(2).toString().isEmpty());
    QVERIFY(!crawler.isPaused());
}

QTEST_MAIN(TestNodeSearchCrawler)

#include "test_nodesearchcrawler.moc"
