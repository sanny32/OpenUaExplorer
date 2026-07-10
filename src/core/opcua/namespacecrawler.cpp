// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file namespacecrawler.cpp
/// \brief Implements the per-namespace node-counting address-space crawler.
///

#include <QOpcUaClient>

#include "namespacecrawler.h"
#include "standardnodeid.h"

///
/// \brief Constructs a crawler bound to a connected client.
/// \param client Client whose address space is crawled.
/// \param timeoutMs Per-browse timeout in milliseconds.
/// \param parent Owning QObject.
///
NamespaceCrawler::NamespaceCrawler(QOpcUaClient *client, int timeoutMs, QObject *parent)
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
/// \brief Starts the crawl from the Root node.
///
void NamespaceCrawler::start()
{
    if (_running || _finished)
        return;
    _running = true;
    if (!_client) {
        finish(tr("The OPC UA client is not connected."));
        return;
    }
    const QString root = QString::fromLatin1(StandardNodeId::ObjectsFolder);
    _visited.insert(root);
    _counts[namespaceIndexOf(root)] += 1;
    _queue.enqueue(root);
    browseNext();
}

///
/// \brief Stops the crawl and emits finished() with the counts gathered so far.
///
void NamespaceCrawler::cancel()
{
    if (_finished)
        return;
    _cancelled = true;
    finish(QString());
}

///
/// \brief Browses the next queued node, skipping any that cannot be created or started.
///
void NamespaceCrawler::browseNext()
{
    if (_finished)
        return;
    if (_cancelled) {
        finish(QString());
        return;
    }
    if (!_client) {
        finish(tr("The OPC UA client is no longer available."));
        return;
    }
    while (!_queue.isEmpty()) {
        const QString nodeId = _queue.dequeue();
        QOpcUaNode *node = _client->node(nodeId);
        if (!node)
            continue;
        _current = node;
        _currentConnection = connect(node, &QOpcUaNode::browseFinished, this,
            [this](const QVector<QOpcUaReferenceDescription> &children, QOpcUa::UaStatusCode) {
                _timeoutTimer.stop();
                releaseCurrent();
                handleChildren(children);
            });
        if (!node->browseChildren()) {
            releaseCurrent();
            continue;
        }
        _timeoutTimer.start(_timeoutMs);
        return;
    }
    finish(QString());
}

///
/// \brief Records unique child nodes, enqueues them, and continues the crawl.
/// \param children Forward hierarchical references returned by the last browse.
///
void NamespaceCrawler::handleChildren(const QVector<QOpcUaReferenceDescription> &children)
{
    if (_finished)
        return;
    if (_cancelled) {
        finish(QString());
        return;
    }
    for (const QOpcUaReferenceDescription &child : children) {
        if (child.targetNodeId().serverIndex() != 0)
            continue;
        const QString childId = child.targetNodeId().nodeId();
        if (childId.isEmpty() || _visited.contains(childId))
            continue;
        _visited.insert(childId);
        _counts[namespaceIndexOf(childId)] += 1;
        _queue.enqueue(childId);
    }
    emit progress(_visited.size());
    browseNext();
}

///
/// \brief Disconnects and schedules deletion of the node being browsed.
///
void NamespaceCrawler::releaseCurrent()
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
/// \param error Error description, empty on success or cancellation.
///
void NamespaceCrawler::finish(const QString &error)
{
    if (_finished)
        return;
    _finished = true;
    _timeoutTimer.stop();
    releaseCurrent();
    emit finished(_counts, error);
}

///
/// \brief Extracts the namespace index from a NodeId string.
/// \param nodeId NodeId such as "ns=3;s=Square".
/// \return Namespace index, or 0 when the NodeId carries no explicit index.
///
int NamespaceCrawler::namespaceIndexOf(const QString &nodeId)
{
    if (nodeId.startsWith(QLatin1String("ns="))) {
        const int semicolon = nodeId.indexOf(QLatin1Char(';'));
        if (semicolon > 3) {
            bool ok = false;
            const int index = QStringView(nodeId).mid(3, semicolon - 3).toInt(&ok);
            if (ok)
                return index;
        }
    }
    return 0;
}
