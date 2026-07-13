// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file nodesearchcrawler.cpp
/// \brief Implements the display-name address-space search crawler.
///

#include <QOpcUaLocalizedText>

#include "nodesearchcrawler.h"

///
/// \brief Constructs a crawler bound to a connected client.
/// \param client Client whose address space is crawled.
/// \param startNodeId Node whose subtree is searched; it is never matched itself.
/// \param pattern Case-insensitive substring matched against display names.
/// \param timeoutMs Per-browse timeout in milliseconds.
/// \param parent Owning QObject.
///
NodeSearchCrawler::NodeSearchCrawler(QOpcUaClient *client, const QString &startNodeId,
                                     const QString &pattern, int timeoutMs, QObject *parent)
    : AddressSpaceCrawler(client, timeoutMs, parent)
    , _startNodeId(startNodeId)
    , _pattern(pattern)
{
}

///
/// \brief Starts the crawl from the start node.
///
void NodeSearchCrawler::start()
{
    if (isRunning() || isFinished())
        return;
    if (!clientAvailable()) {
        finish(tr("The OPC UA client is not connected."));
        return;
    }
    if (_startNodeId.isEmpty() || _pattern.isEmpty()) {
        finish(QString());
        return;
    }
    beginCrawl(_startNodeId);
}

///
/// \brief Remembers a child's parent and buffers it when its display name matches.
/// \param childId NodeId of the child.
/// \param child Reference description the child was discovered through.
/// \param parentNodeId Node that was browsed to find the child.
///
void NodeSearchCrawler::visitChild(const QString &childId,
                                   const QOpcUaReferenceDescription &child,
                                   const QString &parentNodeId)
{
    _parentOf.insert(childId, parentNodeId);
    if (child.displayName().text().contains(_pattern, Qt::CaseInsensitive))
        _pendingMatches.enqueue(childId);
}

///
/// \brief Reports the next buffered match, or keeps browsing when none is buffered.
///
/// Reporting a match pauses the crawl rather than ending it, so resume() can continue
/// from the same breadth-first position instead of restarting from the start node.
///
void NodeSearchCrawler::continueCrawl()
{
    deliverNextMatch();
}

///
/// \brief Reports the next buffered match, or browses on when none is buffered.
///
void NodeSearchCrawler::deliverNextMatch()
{
    if (isFinished() || isCancelled())
        return;
    if (!_pendingMatches.isEmpty()) {
        const QString nodeId = _pendingMatches.dequeue();
        _paused = true;
        emit finished(ancestorsOf(nodeId), nodeId, QString());
        return;
    }
    browseNext();
}

///
/// \brief Continues a paused crawl, reporting the next match after the last one.
///
void NodeSearchCrawler::resume()
{
    if (isFinished() || !_paused)
        return;
    _paused = false;
    deliverNextMatch();
}

///
/// \brief Stops the crawl once the visit budget is exhausted.
/// \param error Set to the budget message when the crawl must stop.
/// \return True once the visit budget is exhausted.
///
bool NodeSearchCrawler::shouldStop(QString *error) const
{
    if (visited().size() < MaxVisitedNodes)
        return false;
    if (error)
        *error = tr("The search stopped after visiting %1 nodes.").arg(MaxVisitedNodes);
    return true;
}

///
/// \brief Reports that the crawl ended without a further match.
/// \param error Error description, empty on success or cancellation.
///
void NodeSearchCrawler::emitFinished(const QString &error)
{
    _paused = false;
    emit finished({}, QString(), error);
}

///
/// \brief Reports whether the crawl is paused on a reported match.
/// \return True when resume() would continue the crawl.
///
bool NodeSearchCrawler::isPaused() const
{
    return _paused && !isFinished();
}

///
/// \brief Reports whether this crawl already covers a search request.
/// \param startNodeId Node whose subtree would be searched.
/// \param pattern Substring that would be matched.
/// \return True when a resume() would answer the request.
///
bool NodeSearchCrawler::matches(const QString &startNodeId, const QString &pattern) const
{
    return !isFinished() && _startNodeId == startNodeId && _pattern == pattern;
}

///
/// \brief Rebuilds the chain of browsed parents leading to a node.
/// \param nodeId Node whose ancestors are wanted.
/// \return Node ids from the start node down to the node's parent, parents before children.
///
QStringList NodeSearchCrawler::ancestorsOf(const QString &nodeId) const
{
    QStringList ancestors;
    QString current = _parentOf.value(nodeId);
    while (!current.isEmpty()) {
        ancestors.prepend(current);
        if (current == _startNodeId)
            break;
        current = _parentOf.value(current);
    }
    return ancestors;
}
