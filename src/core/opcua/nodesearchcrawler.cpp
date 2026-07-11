// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file nodesearchcrawler.cpp
/// \brief Implements the display-name address-space search crawler.
///

#include <QOpcUaClient>

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
    : QObject(parent)
    , _client(client)
    , _startNodeId(startNodeId)
    , _pattern(pattern)
    , _timeoutMs(qMax(1000, timeoutMs))
{
    _timeoutTimer.setSingleShot(true);
    connect(&_timeoutTimer, &QTimer::timeout, this, [this]() {
        releaseCurrent();
        if (_cancelled)
            finish({}, QString(), QString());
        else
            browseNext();
    });
}

///
/// \brief Starts the crawl from the start node.
///
void NodeSearchCrawler::start()
{
    if (_running || _finished)
        return;
    _running = true;
    if (!clientAvailable()) {
        finish({}, QString(), tr("The OPC UA client is not connected."));
        return;
    }
    if (_startNodeId.isEmpty() || _pattern.isEmpty()) {
        finish({}, QString(), QString());
        return;
    }
    _visited.insert(_startNodeId);
    _queue.enqueue(_startNodeId);
    browseNext();
}

///
/// \brief Stops the crawl and emits finished() with no match.
///
void NodeSearchCrawler::cancel()
{
    if (_finished)
        return;
    _cancelled = true;
    finish({}, QString(), QString());
}

///
/// \brief Browses the next queued node, skipping any that cannot be created or started.
///
void NodeSearchCrawler::browseNext()
{
    if (_finished)
        return;
    if (_cancelled) {
        finish({}, QString(), QString());
        return;
    }
    if (!clientAvailable()) {
        finish({}, QString(), tr("The OPC UA client is no longer available."));
        return;
    }
    if (_visited.size() >= MaxVisitedNodes) {
        finish({}, QString(),
               tr("The search stopped after visiting %1 nodes.").arg(MaxVisitedNodes));
        return;
    }
    while (!_queue.isEmpty()) {
        const QString nodeId = _queue.dequeue();
        _currentNodeId = nodeId;
        if (!startBrowse(nodeId))
            continue;
        return;
    }
    finish({}, QString(), QString());
}

///
/// \brief Reports whether a client is available to browse with.
/// \return True when browsing may proceed.
///
bool NodeSearchCrawler::clientAvailable() const
{
    return !_client.isNull();
}

///
/// \brief Returns the client the crawl browses with.
/// \return Client pointer, possibly null.
///
QOpcUaClient *NodeSearchCrawler::client() const
{
    return _client;
}

///
/// \brief Starts an asynchronous browse of one node, arming the timeout.
/// \param nodeId Node whose children are browsed.
/// \return True when the browse started; false to skip the node.
///
bool NodeSearchCrawler::startBrowse(const QString &nodeId)
{
    QOpcUaNode *node = _client->node(nodeId);
    if (!node)
        return false;
    _current = node;
    _currentConnection = connect(node, &QOpcUaNode::browseFinished, this,
        [this](const QVector<QOpcUaReferenceDescription> &children, QOpcUa::UaStatusCode) {
            deliverChildren(children);
        });
    if (!node->browseChildren()) {
        releaseCurrent();
        return false;
    }
    _timeoutTimer.start(_timeoutMs);
    return true;
}

///
/// \brief Feeds a completed browse back into the crawl.
/// \param children Forward hierarchical references of the node being browsed.
///
void NodeSearchCrawler::deliverChildren(const QVector<QOpcUaReferenceDescription> &children)
{
    _timeoutTimer.stop();
    releaseCurrent();
    handleChildren(children);
}

///
/// \brief Matches child display names, enqueues unvisited children, and continues the crawl.
///
/// Every child is enqueued, including matching ones, so that a match never hides its
/// own subtree or the siblings that follow it from a later resume().
/// \param children Forward hierarchical references returned by the last browse.
///
void NodeSearchCrawler::handleChildren(const QVector<QOpcUaReferenceDescription> &children)
{
    if (_finished)
        return;
    if (_cancelled) {
        finish({}, QString(), QString());
        return;
    }
    const QString parentNodeId = _currentNodeId;
    for (const QOpcUaReferenceDescription &child : children) {
        if (child.targetNodeId().serverIndex() != 0)
            continue;
        const QString childId = child.targetNodeId().nodeId();
        if (childId.isEmpty() || _visited.contains(childId))
            continue;
        _visited.insert(childId);
        _parentOf.insert(childId, parentNodeId);
        if (child.displayName().text().contains(_pattern, Qt::CaseInsensitive))
            _pendingMatches.enqueue(childId);
        _queue.enqueue(childId);
    }
    emit progress(_visited.size());
    deliverNextMatch();
}

///
/// \brief Reports the next match found so far, or keeps browsing when none is buffered.
///
/// Reporting a match pauses the crawl rather than ending it, so resume() can continue
/// from the same breadth-first position instead of restarting from the start node.
///
void NodeSearchCrawler::deliverNextMatch()
{
    if (_finished || _cancelled)
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
    if (_finished || !_paused)
        return;
    _paused = false;
    deliverNextMatch();
}

///
/// \brief Reports whether the crawl is paused on a reported match.
/// \return True when resume() would continue the crawl.
///
bool NodeSearchCrawler::isPaused() const
{
    return _paused && !_finished;
}

///
/// \brief Reports whether this crawl already covers a search request.
/// \param startNodeId Node whose subtree would be searched.
/// \param pattern Substring that would be matched.
/// \return True when a resume() would answer the request.
///
bool NodeSearchCrawler::matches(const QString &startNodeId, const QString &pattern) const
{
    return !_finished && _startNodeId == startNodeId && _pattern == pattern;
}

///
/// \brief Disconnects and schedules deletion of the node being browsed.
///
void NodeSearchCrawler::releaseCurrent()
{
    if (_currentConnection) {
        QObject::disconnect(_currentConnection);
        _currentConnection = {};
    }
    if (_current) {
        _current->deleteLater();
        _current = nullptr;
    }
}

///
/// \brief Emits finished() exactly once and tears down any in-flight browse.
/// \param ancestorNodeIds Node ids from the start node down to the match's parent.
/// \param nodeId Matched NodeId, empty when nothing matched.
/// \param error Error description, empty on success or cancellation.
///
void NodeSearchCrawler::finish(const QStringList &ancestorNodeIds, const QString &nodeId,
                               const QString &error)
{
    if (_finished)
        return;
    _finished = true;
    _paused = false;
    _timeoutTimer.stop();
    releaseCurrent();
    emit finished(ancestorNodeIds, nodeId, error);
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
