// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file subtreevariablecrawler.h
/// \brief Declares an isolated address-space crawler that collects a subtree's variables.
///

#pragma once

#include <QString>
#include <QVector>

#include "addressspacecrawler.h"
#include "opcuatypes.h"

class QOpcUaClient;

///
/// \brief Breadth-first crawler that collects every Variable node below a start node.
///
/// Serves the folder drop that lists a whole subtree at once, so the variables arrive in
/// breadth-first order: the ones the user sees in the tree come first. The crawl gives up
/// once either budget is exhausted and reports what it gathered up to that point.
///
class SubtreeVariableCrawler : public AddressSpaceCrawler
{
    Q_OBJECT

public:
    /// \brief Largest number of nodes visited before the crawl gives up.
    static constexpr int MaxVisitedNodes = 5000;

    /// \brief Largest number of variables collected before the crawl gives up.
    static constexpr int MaxVariables = 1000;

    ///
    /// \brief Constructs a crawler bound to a connected client.
    /// \param client Client whose address space is crawled.
    /// \param rootNodeId Node whose subtree is collected; it is never collected itself.
    /// \param timeoutMs Per-browse timeout in milliseconds.
    /// \param parent Owning QObject.
    ///
    SubtreeVariableCrawler(QOpcUaClient *client, const QString &rootNodeId, int timeoutMs,
                           QObject *parent = nullptr);

    ///
    /// \brief Starts the crawl from the root node.
    ///
    void start();

signals:
    ///
    /// \brief Emitted when the subtree is exhausted, a budget runs out, or the crawl fails.
    /// \param rootNodeId Node the crawl started from.
    /// \param variables Variable nodes found, in breadth-first order.
    /// \param error Error description, empty on success or cancellation.
    ///
    void finished(QString rootNodeId, QVector<OpcUaNodeInfo> variables, QString error);

protected:
    void visitChild(const QString &childId, const QOpcUaReferenceDescription &child,
                    const QString &parentNodeId) override;
    void emitFinished(const QString &error) override;
    bool shouldStop(QString *error) const override;

private:
    QString _rootNodeId;
    QVector<OpcUaNodeInfo> _variables;
};
