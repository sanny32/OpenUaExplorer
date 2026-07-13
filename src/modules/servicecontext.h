// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file servicecontext.h
/// \brief Declares the host context handed to each module during initialization.
///

#pragma once

class OpcUaBackend;
class ConnectionController;

///
/// \brief Bundles the core services a module needs to access server data.
///
/// The context deliberately carries no UI: modules expose their own data API and never
/// touch widgets, which keeps the modules library dependent on ouaexp_core only.
///
class ServiceContext
{
public:
    ///
    /// \brief Captures the core services shared with the modules.
    /// \param backend OPC UA backend performing all operations.
    /// \param connectionController Connection/profile controller.
    ///
    ServiceContext(OpcUaBackend *backend,
                  ConnectionController *connectionController);

    ///
    /// \brief Returns the OPC UA backend.
    /// \return Backend.
    ///
    OpcUaBackend *backend() const;

    ///
    /// \brief Returns the connection controller.
    /// \return Connection controller.
    ///
    ConnectionController *connectionController() const;

private:
    OpcUaBackend *_backend;
    ConnectionController *_connectionController;
};
