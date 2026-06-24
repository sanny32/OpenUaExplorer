// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file plugincontext.cpp
/// \brief Implements the plugin host context accessors.
///

#include "plugincontext.h"

PluginContext::PluginContext(OpcUaClientService *clientService,
                             ConnectionController *connectionController)
    : _clientService(clientService)
    , _connectionController(connectionController)
{
}

OpcUaClientService *PluginContext::clientService() const
{
    return _clientService;
}

ConnectionController *PluginContext::connectionController() const
{
    return _connectionController;
}
