// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file attributemodule.h
/// \brief Declares the module that exposes the node attribute read/write API.
///

#pragma once

#include <QVariant>

#include "opcua/opcuatypes.h"
#include "servicemodule.h"

class OpcUaBackend;

///
/// \brief Provides the attribute API: read a node's attributes and write its value.
///
class AttributeModule : public ServiceModule
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs the attribute module.
    /// \param parent Owning QObject.
    ///
    explicit AttributeModule(QObject *parent = nullptr);

    QString name() const override;
    const QLoggingCategory &logCategory() const override;
    void initialize(ServiceContext &context) override;

public slots:
    ///
    /// \brief Reads a node's attribute set.
    /// \param nodeId Node to read.
    ///
    void read(const QString &nodeId);

    ///
    /// \brief Writes a node's value.
    /// \param nodeId Node to write.
    /// \param value Value to write.
    /// \param valueType OPC UA type of the value.
    ///
    void write(const QString &nodeId, const QVariant &value, int valueType);

signals:
    ///
    /// \brief Emitted when a node's attributes have been read.
    /// \param details Read node details.
    /// \param error Read error, empty on success.
    ///
    void attributesReady(OpcUaNodeDetails details, QString error);

    ///
    /// \brief Emitted when a value write finishes.
    /// \param nodeId Written node.
    /// \param success Whether the write succeeded.
    /// \param error Write error, empty on success.
    ///
    void writeFinished(QString nodeId, bool success, QString error);

private:
    void handleNodeDetailsReady(const OpcUaNodeDetails &details, const QString &error);
    void handleWriteFinished(const QString &nodeId, bool success, const QString &error);

    OpcUaBackend *_backend = nullptr;
};
