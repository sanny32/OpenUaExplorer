// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file servicemodule.h
/// \brief Declares the abstract base class for the application's static modules.
///

#pragma once

#include <QObject>
#include <QString>

class QLoggingCategory;
class ServiceContext;

///
/// \brief Base class for a compiled-in functional area (address space, attributes, data access, ...).
///
/// Each module owns its area's OpcUaClientService wiring and logs that area's operations
/// under its own logging category, so the activity log's Source column names the module.
///
class ServiceModule : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs an uninitialised module.
    /// \param parent Owning QObject.
    ///
    explicit ServiceModule(QObject *parent = nullptr) : QObject(parent) {}

    ///
    /// \brief Destroys the module.
    ///
    ~ServiceModule() override = default;

    ///
    /// \brief Returns the human-readable module name shown in the startup log.
    /// \return ServiceModule name, for example "Attribute Module".
    ///
    virtual QString name() const = 0;

    ///
    /// \brief Returns the logging category whose messages the module emits.
    /// \return The module's logging category, used as the activity log Source.
    ///
    virtual const QLoggingCategory &logCategory() const = 0;

    ///
    /// \brief Wires the module to the shared services once, at startup.
    /// \param context Host context granting access to the client service, widgets, and hub.
    ///
    virtual void initialize(ServiceContext &context) = 0;
};
