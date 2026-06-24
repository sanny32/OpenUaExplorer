// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file servicemodulemanager.cpp
/// \brief Implements the static module registry.
///

#include "servicemodulemanager.h"

#include "loggingcategories.h"
#include "servicemodule.h"
#include "servicecontext.h"

ServiceModuleManager::ServiceModuleManager(QObject *parent)
    : QObject(parent)
{
}

void ServiceModuleManager::registerModule(ServiceModule *module)
{
    if (!module)
        return;
    module->setParent(this);
    _modules.append(module);
    qCInfo(lcApp).noquote() << tr("Loaded %1 (Service Module)").arg(module->name());
}

void ServiceModuleManager::initializeAll(ServiceContext &context)
{
    for (ServiceModule *module : _modules)
        module->initialize(context);
}

QList<ServiceModule *> ServiceModuleManager::modules() const
{
    return _modules;
}
