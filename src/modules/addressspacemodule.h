// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file addressspacemodule.h
/// \brief Declares the module that exposes the address-space browse API.
///

#pragma once

#include "opcua/opcuatypes.h"
#include "servicemodule.h"

class OpcUaClientService;

///
/// \brief Provides the address-space browse API: request children and deliver them as a signal.
///
class AddressSpaceModule : public ServiceModule
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs the address space module.
    /// \param parent Owning QObject.
    ///
    explicit AddressSpaceModule(QObject *parent = nullptr);

    QString name() const override;
    const QLoggingCategory &logCategory() const override;
    void initialize(ServiceContext &context) override;

public slots:
    ///
    /// \brief Browses the children of a node.
    /// \param nodeId Node to browse.
    ///
    void browse(const QString &nodeId);

    ///
    /// \brief Browses a node, defaulting to the Objects folder when none is given.
    /// \param nodeId Node to browse, or empty for the Objects folder.
    ///
    void refresh(const QString &nodeId);

    ///
    /// \brief Searches a subtree on the server for a node whose display name matches.
    /// \param startNodeId Node whose subtree is searched.
    /// \param pattern Case-insensitive substring matched against display names.
    ///
    void search(const QString &startNodeId, const QString &pattern);

    ///
    /// \brief Cancels an in-progress node search, if any.
    ///
    void cancelSearch();

signals:
    ///
    /// \brief Emitted when a node's children have been browsed.
    /// \param parentNodeId Browsed node.
    /// \param children Browse result.
    /// \param error Browse error, empty on success.
    ///
    void childrenReady(QString parentNodeId, QVector<OpcUaNodeInfo> children, QString error);

    ///
    /// \brief Emitted periodically while a node search runs.
    /// \param visitedNodes Number of unique nodes visited so far.
    ///
    void searchProgress(int visitedNodes);

    ///
    /// \brief Emitted when a node search finds a match, exhausts the subtree, or fails.
    /// \param ancestorNodeIds Node ids from the start node down to the match's parent.
    /// \param nodeId Matched NodeId, empty when nothing matched.
    /// \param error Search error, empty on success.
    ///
    void searchFinished(QStringList ancestorNodeIds, QString nodeId, QString error);

private:
    void handleBrowseFinished(const QString &parentNodeId,
                              const QVector<OpcUaNodeInfo> &children,
                              const QString &error);
    void handleSearchFinished(const QStringList &ancestorNodeIds, const QString &nodeId,
                              const QString &error);

    OpcUaClientService *_clientService = nullptr;
};
