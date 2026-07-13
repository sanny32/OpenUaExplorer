// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file addressspacecrawler.h
/// \brief Declares the breadth-first address-space crawl shared by the concrete crawlers.
///

#pragma once

#include <QObject>
#include <QPointer>
#include <QQueue>
#include <QSet>
#include <QString>
#include <QTimer>

#include <QOpcUaNode>
#include <QOpcUaReferenceDescription>

class QOpcUaClient;

///
/// \brief Breadth-first walk of the address space, serialized over its own QOpcUaNode instances.
///
/// Runs on nodes it creates itself, so it never touches the shared browse pipeline used by
/// the address-space browser, whose requests supersede one another. One node is browsed at a
/// time and each discovered target is visited exactly once. Subclasses decide what to record
/// for each child, whether to keep going after a browse, and which signal reports the result.
///
class AddressSpaceCrawler : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs a crawler bound to a connected client.
    /// \param client Client whose address space is crawled.
    /// \param timeoutMs Per-browse timeout in milliseconds.
    /// \param parent Owning QObject.
    ///
    AddressSpaceCrawler(QOpcUaClient *client, int timeoutMs, QObject *parent = nullptr);

    ///
    /// \brief Stops the crawl and reports the result gathered so far.
    ///
    void cancel();

signals:
    ///
    /// \brief Emitted after each browse with the running unique-node total.
    /// \param visitedNodes Number of unique nodes visited so far.
    ///
    void progress(int visitedNodes);

protected:
    ///
    /// \brief Seeds the queue with the root node and starts browsing.
    /// \param rootNodeId Node the crawl starts from.
    ///
    void beginCrawl(const QString &rootNodeId);

    ///
    /// \brief Browses the next queued node, skipping any that cannot be created or started.
    ///
    void browseNext();

    ///
    /// \brief Ends the crawl exactly once, tearing down any in-flight browse.
    /// \param error Error description, empty on success or cancellation.
    ///
    void finish(const QString &error);

    ///
    /// \brief Feeds a completed browse back into the crawl.
    /// \param children Forward hierarchical references of the node being browsed.
    ///
    void deliverChildren(const QVector<QOpcUaReferenceDescription> &children);

    ///
    /// \brief Records one newly discovered child.
    /// \param childId NodeId of the child, already marked visited.
    /// \param child Reference description the child was discovered through.
    /// \param parentNodeId Node that was browsed to find the child.
    ///
    virtual void visitChild(const QString &childId, const QOpcUaReferenceDescription &child,
                            const QString &parentNodeId) = 0;

    ///
    /// \brief Emits the subclass's own finished() signal.
    /// \param error Error description, empty on success or cancellation.
    ///
    virtual void emitFinished(const QString &error) = 0;

    ///
    /// \brief Continues the crawl after a browse has been processed.
    ///
    /// The default browses the next queued node. Subclasses that report intermediate
    /// results override this to pause instead.
    ///
    virtual void continueCrawl();

    ///
    /// \brief Reports whether the crawl must stop before browsing the next node.
    /// \param error Set to the reason when the crawl must stop.
    /// \return True to end the crawl.
    ///
    virtual bool shouldStop(QString *error) const;

    ///
    /// \brief Reports whether a client is available to browse with.
    /// \return True when browsing may proceed.
    ///
    virtual bool clientAvailable() const;

    ///
    /// \brief Starts an asynchronous browse of one node, arming the timeout.
    ///
    /// The result must be handed back through deliverChildren(). Overridden by tests to
    /// walk a synthetic tree without a live server.
    /// \param nodeId Node whose children are browsed.
    /// \return True when the browse started; false to skip the node.
    ///
    virtual bool startBrowse(const QString &nodeId);

    ///
    /// \brief Returns the client the crawl browses with.
    /// \return Client pointer, possibly null.
    ///
    QOpcUaClient *client() const;

    ///
    /// \brief Disconnects and schedules deletion of the node being browsed.
    ///
    void releaseCurrent();

    ///
    /// \brief Marks a node as visited without browsing it.
    /// \param nodeId Node to mark.
    ///
    void markVisited(const QString &nodeId);

    ///
    /// \brief Returns the nodes visited so far.
    /// \return Visited NodeIds.
    ///
    const QSet<QString> &visited() const;

    ///
    /// \brief Returns the node whose browse is in flight.
    /// \return NodeId currently being browsed.
    ///
    QString currentNodeId() const;

    ///
    /// \brief Reports whether the crawl has already ended.
    /// \return True once finish() has run.
    ///
    bool isFinished() const;

    ///
    /// \brief Reports whether the crawl has been cancelled.
    /// \return True after cancel().
    ///
    bool isCancelled() const;

    ///
    /// \brief Reports whether the crawl has been started.
    /// \return True after beginCrawl().
    ///
    bool isRunning() const;

private:
    void handleChildren(const QVector<QOpcUaReferenceDescription> &children);

    QPointer<QOpcUaClient> _client;
    int _timeoutMs;
    QQueue<QString> _queue;
    QSet<QString> _visited;
    QString _currentNodeId;
    QPointer<QOpcUaNode> _current;
    QMetaObject::Connection _currentConnection;
    QTimer _timeoutTimer;
    bool _running = false;
    bool _cancelled = false;
    bool _finished = false;
};
