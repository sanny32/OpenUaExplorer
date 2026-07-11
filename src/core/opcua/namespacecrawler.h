// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file namespacecrawler.h
/// \brief Declares an isolated address-space crawler that counts nodes per namespace.
///

#pragma once

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QQueue>
#include <QSet>
#include <QString>
#include <QTimer>

#include <QOpcUaNode>
#include <QOpcUaReferenceDescription>

#include "opcuatypes.h"

class QOpcUaClient;

///
/// \brief Breadth-first crawler that counts unique nodes per OPC UA namespace.
///
/// Runs on its own QOpcUaNode instances so it never touches the shared browse
/// pipeline used by the address-space browser. Browsing is serialized: one node
/// is browsed at a time, and each newly discovered target is visited exactly once.
///
class NamespaceCrawler : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs a crawler bound to a connected client.
    /// \param client Client whose address space is crawled.
    /// \param timeoutMs Per-browse timeout in milliseconds.
    /// \param parent Owning QObject.
    ///
    NamespaceCrawler(QOpcUaClient *client, int timeoutMs, QObject *parent = nullptr);

    ///
    /// \brief Starts the crawl from the Root node.
    ///
    void start();

    ///
    /// \brief Stops the crawl and emits finished() with the counts gathered so far.
    ///
    void cancel();

signals:
    ///
    /// \brief Emitted after each browse with the running unique-node total.
    /// \param visitedNodes Number of unique nodes visited so far.
    ///
    void progress(int visitedNodes);

    ///
    /// \brief Emitted once when the crawl finishes, is cancelled, or fails.
    /// \param nodeCounts Node counts keyed by namespace index.
    /// \param error Error description, empty on success or cancellation.
    ///
    void finished(OpcUaNamespaceNodeCounts nodeCounts, QString error);

private:
    void browseNext();
    void handleChildren(const QVector<QOpcUaReferenceDescription> &children);
    void releaseCurrent();
    void finish(const QString &error);
    static int namespaceIndexOf(const QString &nodeId);

    QPointer<QOpcUaClient> _client;
    int _timeoutMs;
    QQueue<QString> _queue;
    QSet<QString> _visited;
    OpcUaNamespaceNodeCounts _counts;
    QPointer<QOpcUaNode> _current;
    QMetaObject::Connection _currentConnection;
    QTimer _timeoutTimer;
    bool _running = false;
    bool _cancelled = false;
    bool _finished = false;
};
