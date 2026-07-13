// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file servermodule.h
/// \brief Declares the headless module that logs the connection lifecycle.
///

#pragma once

#include "opcua/opcuatypes.h"
#include "servicemodule.h"

class OpcUaBackend;
class ConnectionController;

///
/// \brief Logs server connection lifecycle events under the Server source.
///
class ServerModule : public ServiceModule
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs the server module.
    /// \param parent Owning QObject.
    ///
    explicit ServerModule(QObject *parent = nullptr);

    QString name() const override;
    const QLoggingCategory &logCategory() const override;
    void initialize(ServiceContext &context) override;

private:
    void handleStateChanged(OpcUaConnectionState state);

    OpcUaBackend *_backend = nullptr;
    ConnectionController *_connectionController = nullptr;
};
