// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file nodesearchcrawler.h
/// \brief Declares an isolated address-space crawler that finds a node by display name.
///

#pragma once

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QQueue>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTimer>

#include <QOpcUaNode>
#include <QOpcUaReferenceDescription>

class QOpcUaClient;

///
/// \brief Breadth-first crawler that locates the first node whose display name matches a pattern.
///
/// Runs on its own QOpcUaNode instances so it never touches the shared browse
/// pipeline used by the address-space browser, whose requests supersede one
/// another. Browsing is serialized: one node is browsed at a time, and each
/// discovered target is visited exactly once. The crawl stops at the first match
/// or once the visit budget is exhausted.
///
class NodeSearchCrawler : public QObject
{
    Q_OBJECT

public:
    /// \brief Largest number of nodes visited before the crawl gives up.
    static constexpr int MaxVisitedNodes = 5000;

    ///
    /// \brief Constructs a crawler bound to a connected client.
    /// \param client Client whose address space is crawled.
    /// \param startNodeId Node whose subtree is searched; it is never matched itself.
    /// \param pattern Case-insensitive substring matched against display names.
    /// \param timeoutMs Per-browse timeout in milliseconds.
    /// \param parent Owning QObject.
    ///
    NodeSearchCrawler(QOpcUaClient *client, const QString &startNodeId, const QString &pattern,
                      int timeoutMs, QObject *parent = nullptr);

    ///
    /// \brief Starts the crawl from the start node.
    ///
    void start();

    ///
    /// \brief Continues a paused crawl, reporting the next match after the last one.
    ///
    void resume();

    ///
    /// \brief Stops the crawl and emits finished() with no match.
    ///
    void cancel();

    ///
    /// \brief Reports whether the crawl is paused on a reported match.
    /// \return True when resume() would continue the crawl.
    ///
    bool isPaused() const;

    ///
    /// \brief Reports whether this crawl already covers a search request.
    /// \param startNodeId Node whose subtree would be searched.
    /// \param pattern Substring that would be matched.
    /// \return True when a resume() would answer the request.
    ///
    bool matches(const QString &startNodeId, const QString &pattern) const;

signals:
    ///
    /// \brief Emitted after each browse with the running unique-node total.
    /// \param visitedNodes Number of unique nodes visited so far.
    ///
    void progress(int visitedNodes);

    ///
    /// \brief Emitted when the crawl finds a match, exhausts the subtree, or fails.
    ///
    /// A match pauses the crawl instead of ending it, so this may be emitted once per
    /// resume(). An empty nodeId means no further match exists.
    /// \param ancestorNodeIds Node ids from the start node down to the match's parent.
    /// \param nodeId Matched NodeId, empty when nothing matched.
    /// \param error Error description, empty on success or cancellation.
    ///
    void finished(QStringList ancestorNodeIds, QString nodeId, QString error);

protected:
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
    /// \brief Feeds a completed browse back into the crawl.
    /// \param children Forward hierarchical references of the node being browsed.
    ///
    void deliverChildren(const QVector<QOpcUaReferenceDescription> &children);

    ///
    /// \brief Returns the client the crawl browses with.
    /// \return Client pointer, possibly null.
    ///
    QOpcUaClient *client() const;

private:
    void browseNext();
    void handleChildren(const QVector<QOpcUaReferenceDescription> &children);
    void deliverNextMatch();
    void releaseCurrent();
    void finish(const QStringList &ancestorNodeIds, const QString &nodeId, const QString &error);
    QStringList ancestorsOf(const QString &nodeId) const;

    QPointer<QOpcUaClient> _client;
    QString _startNodeId;
    QString _pattern;
    int _timeoutMs;
    QQueue<QString> _queue;
    QQueue<QString> _pendingMatches;
    QSet<QString> _visited;
    QHash<QString, QString> _parentOf;
    QString _currentNodeId;
    QPointer<QOpcUaNode> _current;
    QMetaObject::Connection _currentConnection;
    QTimer _timeoutTimer;
    bool _running = false;
    bool _paused = false;
    bool _cancelled = false;
    bool _finished = false;
};
