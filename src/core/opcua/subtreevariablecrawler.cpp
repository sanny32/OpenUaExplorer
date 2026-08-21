// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file subtreevariablecrawler.cpp
/// \brief Implements the subtree variable-collecting address-space crawler.
///

#include <QOpcUaExpandedNodeId>
#include <QOpcUaLocalizedText>
#include <QOpcUaQualifiedName>

#include "subtreevariablecrawler.h"

///
/// \brief Constructs a crawler bound to a connected client.
/// \param client Client whose address space is crawled.
/// \param rootNodeId Node whose subtree is collected; it is never collected itself.
/// \param timeoutMs Per-browse timeout in milliseconds.
/// \param parent Owning QObject.
///
SubtreeVariableCrawler::SubtreeVariableCrawler(QOpcUaClient *client, const QString &rootNodeId,
                                               int timeoutMs, QObject *parent)
    : AddressSpaceCrawler(client, timeoutMs, parent)
    , _rootNodeId(rootNodeId)
{
}

///
/// \brief Starts the crawl from the root node.
///
void SubtreeVariableCrawler::start()
{
    if (isRunning() || isFinished())
        return;
    if (!clientAvailable()) {
        finish(tr("The OPC UA client is not connected."));
        return;
    }
    if (_rootNodeId.isEmpty()) {
        finish(QString());
        return;
    }
    beginCrawl(_rootNodeId);
}

///
/// \brief Collects a newly discovered child when it is a Variable node.
/// \param childId NodeId of the child.
/// \param child Reference description the child was discovered through.
/// \param parentNodeId Node that was browsed to find the child.
///
void SubtreeVariableCrawler::visitChild(const QString &childId,
                                        const QOpcUaReferenceDescription &child,
                                        const QString &parentNodeId)
{
    Q_UNUSED(parentNodeId)
    const int nodeClass = static_cast<int>(child.nodeClass());
    if (!OpcUa::isVariable(nodeClass))
        return;

    OpcUaNodeInfo info;
    info.nodeId = childId;
    info.browseName = child.browseName().name();
    info.displayName = child.displayName().text();
    info.referenceTypeId = child.refTypeId();
    info.typeDefinitionId = child.typeDefinition().nodeId();
    info.nodeClass = nodeClass;
    _variables.append(info);
}

///
/// \brief Stops the crawl once one of the budgets is exhausted.
/// \param error Set to the reason when the crawl must stop.
/// \return True once a budget is exhausted.
///
/// A budget is not a failure: the error stays empty so the variables gathered so far
/// are still reported.
///
bool SubtreeVariableCrawler::shouldStop(QString *error) const
{
    Q_UNUSED(error)
    return visited().size() >= MaxVisitedNodes || _variables.size() >= MaxVariables;
}

///
/// \brief Reports the variables gathered so far.
/// \param error Error description, empty on success or cancellation.
///
void SubtreeVariableCrawler::emitFinished(const QString &error)
{
    emit finished(_rootNodeId, _variables, error);
}
