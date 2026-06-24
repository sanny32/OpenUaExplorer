// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file serverplugin.h
/// \brief Declares the headless plugin that logs the connection lifecycle.
///

#pragma once

#include "opcua/opcuatypes.h"
#include "plugin.h"

class OpcUaClientService;
class ConnectionController;

///
/// \brief Logs server connection lifecycle events under the Server source.
///
class ServerPlugin : public Plugin
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs the server plugin.
    /// \param parent Owning QObject.
    ///
    explicit ServerPlugin(QObject *parent = nullptr);

    QString name() const override;
    const QLoggingCategory &logCategory() const override;
    void initialize(PluginContext &context) override;

private:
    void handleStateChanged(OpcUaConnectionState state);

    OpcUaClientService *_clientService = nullptr;
    ConnectionController *_connectionController = nullptr;
};
