// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file plugin.h
/// \brief Declares the abstract base class for the application's static plugins.
///

#pragma once

#include <QObject>
#include <QString>

class QLoggingCategory;
class PluginContext;

///
/// \brief Base class for a compiled-in functional area (address space, attributes, data access, ...).
///
/// Each plugin owns its area's OpcUaClientService wiring and logs that area's operations
/// under its own logging category, so the activity log's Source column names the plugin.
///
class Plugin : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs an uninitialised plugin.
    /// \param parent Owning QObject.
    ///
    explicit Plugin(QObject *parent = nullptr) : QObject(parent) {}

    ///
    /// \brief Destroys the plugin.
    ///
    ~Plugin() override = default;

    ///
    /// \brief Returns the human-readable plugin name shown in the startup log.
    /// \return Plugin name, for example "Attribute Plugin".
    ///
    virtual QString name() const = 0;

    ///
    /// \brief Returns the logging category whose messages the plugin emits.
    /// \return The plugin's logging category, used as the activity log Source.
    ///
    virtual const QLoggingCategory &logCategory() const = 0;

    ///
    /// \brief Wires the plugin to the shared services once, at startup.
    /// \param context Host context granting access to the client service, widgets, and hub.
    ///
    virtual void initialize(PluginContext &context) = 0;
};
