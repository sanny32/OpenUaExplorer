// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file addressspacecrawler.cpp
/// \brief Implements the breadth-first address-space crawl shared by the concrete crawlers.
///

#include <QOpcUaClient>

#include "addressspacecrawler.h"

///
/// \brief Constructs a crawler bound to a connected client.
/// \param client Client whose address space is crawled.
/// \param timeoutMs Per-browse timeout in milliseconds.
/// \param parent Owning QObject.
///
AddressSpaceCrawler::AddressSpaceCrawler(QOpcUaClient *client, int timeoutMs, QObject *parent)
    : QObject(parent)
    , _client(client)
    , _timeoutMs(qMax(1000, timeoutMs))
{
    _timeoutTimer.setSingleShot(true);
    connect(&_timeoutTimer, &QTimer::timeout, this, [this]() {
        releaseCurrent();
        if (_cancelled)
            finish(QString());
        else
            browseNext();
    });
}

///
/// \brief Stops the crawl and reports the result gathered so far.
///
void AddressSpaceCrawler::cancel()
{
    if (_finished)
        return;
    _cancelled = true;
    finish(QString());
}

///
/// \brief Seeds the queue with the root node and starts browsing.
/// \param rootNodeId Node the crawl starts from.
///
void AddressSpaceCrawler::beginCrawl(const QString &rootNodeId)
{
    _running = true;
    _visited.insert(rootNodeId);
    _queue.enqueue(rootNodeId);
    browseNext();
}

///
/// \brief Browses the next queued node, skipping any that cannot be created or started.
///
void AddressSpaceCrawler::browseNext()
{
    if (_finished)
        return;
    if (_cancelled) {
        finish(QString());
        return;
    }
    if (!clientAvailable()) {
        finish(tr("The OPC UA client is no longer available."));
        return;
    }
    QString stopError;
    if (shouldStop(&stopError)) {
        finish(stopError);
        return;
    }
    while (!_queue.isEmpty()) {
        const QString nodeId = _queue.dequeue();
        _currentNodeId = nodeId;
        if (!startBrowse(nodeId))
            continue;
        return;
    }
    finish(QString());
}

///
/// \brief Feeds a completed browse back into the crawl.
/// \param children Forward hierarchical references of the node being browsed.
///
void AddressSpaceCrawler::deliverChildren(const QVector<QOpcUaReferenceDescription> &children)
{
    _timeoutTimer.stop();
    releaseCurrent();
    handleChildren(children);
}

///
/// \brief Records unique children, hands each to the subclass, and continues the crawl.
/// \param children Forward hierarchical references returned by the last browse.
///
void AddressSpaceCrawler::handleChildren(const QVector<QOpcUaReferenceDescription> &children)
{
    if (_finished)
        return;
    if (_cancelled) {
        finish(QString());
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
        visitChild(childId, child, parentNodeId);
        _queue.enqueue(childId);
    }
    emit progress(_visited.size());
    continueCrawl();
}

///
/// \brief Continues the crawl after a browse has been processed.
///
void AddressSpaceCrawler::continueCrawl()
{
    browseNext();
}

///
/// \brief Reports whether the crawl must stop before browsing the next node.
/// \param error Set to the reason when the crawl must stop.
/// \return True to end the crawl.
///
bool AddressSpaceCrawler::shouldStop(QString *error) const
{
    Q_UNUSED(error)
    return false;
}

///
/// \brief Reports whether a client is available to browse with.
/// \return True when browsing may proceed.
///
bool AddressSpaceCrawler::clientAvailable() const
{
    return !_client.isNull();
}

///
/// \brief Returns the client the crawl browses with.
/// \return Client pointer, possibly null.
///
QOpcUaClient *AddressSpaceCrawler::client() const
{
    return _client;
}

///
/// \brief Starts an asynchronous browse of one node, arming the timeout.
/// \param nodeId Node whose children are browsed.
/// \return True when the browse started; false to skip the node.
///
bool AddressSpaceCrawler::startBrowse(const QString &nodeId)
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
/// \brief Disconnects and schedules deletion of the node being browsed.
///
void AddressSpaceCrawler::releaseCurrent()
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
/// \brief Ends the crawl exactly once, tearing down any in-flight browse.
/// \param error Error description, empty on success or cancellation.
///
void AddressSpaceCrawler::finish(const QString &error)
{
    if (_finished)
        return;
    _finished = true;
    _timeoutTimer.stop();
    releaseCurrent();
    emitFinished(error);
}

///
/// \brief Marks a node as visited without browsing it.
/// \param nodeId Node to mark.
///
void AddressSpaceCrawler::markVisited(const QString &nodeId)
{
    _visited.insert(nodeId);
}

///
/// \brief Returns the nodes visited so far.
/// \return Visited NodeIds.
///
const QSet<QString> &AddressSpaceCrawler::visited() const
{
    return _visited;
}

///
/// \brief Returns the node whose browse is in flight.
/// \return NodeId currently being browsed.
///
QString AddressSpaceCrawler::currentNodeId() const
{
    return _currentNodeId;
}

///
/// \brief Reports whether the crawl has already ended.
/// \return True once finish() has run.
///
bool AddressSpaceCrawler::isFinished() const
{
    return _finished;
}

///
/// \brief Reports whether the crawl has been cancelled.
/// \return True after cancel().
///
bool AddressSpaceCrawler::isCancelled() const
{
    return _cancelled;
}

///
/// \brief Reports whether the crawl has been started.
/// \return True after beginCrawl().
///
bool AddressSpaceCrawler::isRunning() const
{
    return _running;
}
