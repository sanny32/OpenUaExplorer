// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file namespacecrawler.cpp
/// \brief Implements the per-namespace node-counting address-space crawler.
///

#include "namespacecrawler.h"
#include "standardnodeid.h"

///
/// \brief Constructs a crawler bound to a connected client.
/// \param client Client whose address space is crawled.
/// \param timeoutMs Per-browse timeout in milliseconds.
/// \param parent Owning QObject.
///
NamespaceCrawler::NamespaceCrawler(QOpcUaClient *client, int timeoutMs, QObject *parent)
    : AddressSpaceCrawler(client, timeoutMs, parent)
{
}

///
/// \brief Starts the crawl from the Objects folder.
///
void NamespaceCrawler::start()
{
    if (isRunning() || isFinished())
        return;
    if (!clientAvailable()) {
        finish(tr("The OPC UA client is not connected."));
        return;
    }
    const QString root = QString::fromLatin1(StandardNodeId::ObjectsFolder);
    _counts[namespaceIndexOf(root)] += 1;
    beginCrawl(root);
}

///
/// \brief Counts a newly discovered child against its namespace.
/// \param childId NodeId of the child.
/// \param child Reference description the child was discovered through.
/// \param parentNodeId Node that was browsed to find the child.
///
void NamespaceCrawler::visitChild(const QString &childId, const QOpcUaReferenceDescription &child,
                                  const QString &parentNodeId)
{
    Q_UNUSED(child)
    Q_UNUSED(parentNodeId)
    _counts[namespaceIndexOf(childId)] += 1;
}

///
/// \brief Reports the counts gathered so far.
/// \param error Error description, empty on success or cancellation.
///
void NamespaceCrawler::emitFinished(const QString &error)
{
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
