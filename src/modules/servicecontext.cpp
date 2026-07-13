// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file servicecontext.cpp
/// \brief Implements the module host context accessors.
///

#include "servicecontext.h"

ServiceContext::ServiceContext(OpcUaBackend *backend,
                             ConnectionController *connectionController)
    : _backend(backend)
    , _connectionController(connectionController)
{
}

OpcUaBackend *ServiceContext::backend() const
{
    return _backend;
}

ConnectionController *ServiceContext::connectionController() const
{
    return _connectionController;
}
