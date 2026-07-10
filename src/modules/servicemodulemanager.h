// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file servicemodulemanager.h
/// \brief Declares the registry that owns and initializes the static modules.
///

#pragma once

#include <QList>
#include <QObject>

#include "servicemodule.h"

class ServiceContext;

///
/// \brief Owns the compiled-in modules, logs their load, and initializes them once.
///
class ServiceModuleManager : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs an empty module registry.
    /// \param parent Owning QObject.
    ///
    explicit ServiceModuleManager(QObject *parent = nullptr);

    ///
    /// \brief Takes ownership of a module and logs that it was loaded.
    /// \param module ServiceModule to register; reparented to the manager.
    ///
    void registerModule(ServiceModule *module);

    ///
    /// \brief Initializes every registered module with the host context.
    /// \param context Host context shared with the modules.
    ///
    void initializeAll(ServiceContext &context);

    ///
    /// \brief Returns the registered modules in registration order.
    /// \return Registered modules.
    ///
    QList<ServiceModule *> modules() const;

    ///
    /// \brief Returns the first registered module of a given type.
    /// \tparam T Concrete module type to look up.
    /// \return Matching module, or nullptr when none is registered.
    ///
    template <class T>
    T *module() const
    {
        for (ServiceModule *candidate : _modules) {
            if (auto *typed = qobject_cast<T *>(candidate))
                return typed;
        }
        return nullptr;
    }

private:
    QList<ServiceModule *> _modules;
};
