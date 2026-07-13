// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file nodesearchcrawler.h
/// \brief Declares an isolated address-space crawler that finds a node by display name.
///

#pragma once

#include <QHash>
#include <QQueue>
#include <QString>
#include <QStringList>

#include "addressspacecrawler.h"

class QOpcUaClient;

///
/// \brief Breadth-first crawler that locates the nodes whose display name matches a pattern.
///
/// A match pauses the crawl instead of ending it, so resume() reports the next match from
/// the same breadth-first position. The crawl gives up once the visit budget is exhausted.
///
class NodeSearchCrawler : public AddressSpaceCrawler
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
    void visitChild(const QString &childId, const QOpcUaReferenceDescription &child,
                    const QString &parentNodeId) override;
    void emitFinished(const QString &error) override;
    void continueCrawl() override;
    bool shouldStop(QString *error) const override;

private:
    void deliverNextMatch();
    QStringList ancestorsOf(const QString &nodeId) const;

    QString _startNodeId;
    QString _pattern;
    QQueue<QString> _pendingMatches;
    QHash<QString, QString> _parentOf;
    bool _paused = false;
};
