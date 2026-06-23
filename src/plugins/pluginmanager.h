// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file pluginmanager.h
/// \brief Declares the registry that owns and initializes the static plugins.
///

#pragma once

#include <QList>
#include <QObject>

class Plugin;
class PluginContext;

///
/// \brief Owns the compiled-in plugins, logs their load, and initializes them once.
///
class PluginManager : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs an empty plugin registry.
    /// \param parent Owning QObject.
    ///
    explicit PluginManager(QObject *parent = nullptr);

    ///
    /// \brief Takes ownership of a plugin and logs that it was loaded.
    /// \param plugin Plugin to register; reparented to the manager.
    ///
    void registerPlugin(Plugin *plugin);

    ///
    /// \brief Initializes every registered plugin with the host context.
    /// \param context Host context shared with the plugins.
    ///
    void initializeAll(PluginContext &context);

    ///
    /// \brief Returns the registered plugins in registration order.
    /// \return Registered plugins.
    ///
    QList<Plugin *> plugins() const;

private:
    QList<Plugin *> _plugins;
};
