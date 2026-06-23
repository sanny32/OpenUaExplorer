// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file pluginmanager.cpp
/// \brief Implements the static plugin registry.
///

#include "pluginmanager.h"

#include "loggingcategories.h"
#include "plugin.h"
#include "plugincontext.h"

PluginManager::PluginManager(QObject *parent)
    : QObject(parent)
{
}

void PluginManager::registerPlugin(Plugin *plugin)
{
    if (!plugin)
        return;
    plugin->setParent(this);
    _plugins.append(plugin);
    qCInfo(lcApp).noquote() << tr("Loaded %1 (Static Plugin)").arg(plugin->name());
}

void PluginManager::initializeAll(PluginContext &context)
{
    for (Plugin *plugin : _plugins)
        plugin->initialize(context);
}

QList<Plugin *> PluginManager::plugins() const
{
    return _plugins;
}
