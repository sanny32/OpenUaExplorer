// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file referencemodule.h
/// \brief Declares the module that exposes the node-reference browse API.
///

#pragma once

#include "opcua/opcuatypes.h"
#include "servicemodule.h"

class OpcUaClientService;

///
/// \brief Provides the reference browse API: request a node's references and deliver them.
///
class ReferenceModule : public ServiceModule
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs the reference module.
    /// \param parent Owning QObject.
    ///
    explicit ReferenceModule(QObject *parent = nullptr);

    QString name() const override;
    const QLoggingCategory &logCategory() const override;
    void initialize(ServiceContext &context) override;

public slots:
    ///
    /// \brief Browses the forward references of a node.
    /// \param nodeId Node to browse.
    ///
    void browseReferences(const QString &nodeId);

signals:
    ///
    /// \brief Emitted when a node's references have been browsed.
    /// \param sourceNodeId Browsed node.
    /// \param references Reference browse result.
    /// \param error Browse error, empty on success.
    ///
    void referencesReady(QString sourceNodeId, QVector<OpcUaNodeInfo> references, QString error);

private:
    void handleReferencesFinished(const QString &sourceNodeId,
                                  const QVector<OpcUaNodeInfo> &references,
                                  const QString &error);

    OpcUaClientService *_clientService = nullptr;
};
