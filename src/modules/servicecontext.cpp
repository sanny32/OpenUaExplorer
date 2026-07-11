// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file servicecontext.cpp
/// \brief Implements the module host context accessors.
///

#include "servicecontext.h"

ServiceContext::ServiceContext(OpcUaClientService *clientService,
                             ConnectionController *connectionController)
    : _clientService(clientService)
    , _connectionController(connectionController)
{
}

OpcUaClientService *ServiceContext::clientService() const
{
    return _clientService;
}

ConnectionController *ServiceContext::connectionController() const
{
    return _connectionController;
}
