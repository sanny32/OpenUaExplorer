// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file namespacecrawler.h
/// \brief Declares an isolated address-space crawler that counts nodes per namespace.
///

#pragma once

#include <QString>

#include "addressspacecrawler.h"
#include "opcuatypes.h"

class QOpcUaClient;

///
/// \brief Breadth-first crawler that counts unique nodes per OPC UA namespace.
///
class NamespaceCrawler : public AddressSpaceCrawler
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
    /// \brief Starts the crawl from the Objects folder.
    ///
    void start();

signals:
    ///
    /// \brief Emitted once when the crawl finishes, is cancelled, or fails.
    /// \param nodeCounts Node counts keyed by namespace index.
    /// \param error Error description, empty on success or cancellation.
    ///
    void finished(OpcUaNamespaceNodeCounts nodeCounts, QString error);

protected:
    void visitChild(const QString &childId, const QOpcUaReferenceDescription &child,
                    const QString &parentNodeId) override;
    void emitFinished(const QString &error) override;

private:
    static int namespaceIndexOf(const QString &nodeId);

    OpcUaNamespaceNodeCounts _counts;
};
