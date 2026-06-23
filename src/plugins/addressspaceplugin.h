// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file addressspaceplugin.h
/// \brief Declares the plugin that exposes the address-space browse API.
///

#pragma once

#include "opcua/opcuatypes.h"
#include "plugin.h"

class OpcUaClientService;

///
/// \brief Provides the address-space browse API: request children and deliver them as a signal.
///
class AddressSpacePlugin : public Plugin
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs the address space plugin.
    /// \param parent Owning QObject.
    ///
    explicit AddressSpacePlugin(QObject *parent = nullptr);

    QString name() const override;
    const QLoggingCategory &logCategory() const override;
    void initialize(PluginContext &context) override;

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

signals:
    ///
    /// \brief Emitted when a node's children have been browsed.
    /// \param parentNodeId Browsed node.
    /// \param children Browse result.
    /// \param error Browse error, empty on success.
    ///
    void childrenReady(QString parentNodeId, QVector<OpcUaNodeInfo> children, QString error);

private:
    void handleBrowseFinished(const QString &parentNodeId,
                              const QVector<OpcUaNodeInfo> &children,
                              const QString &error);

    OpcUaClientService *_clientService = nullptr;
};
