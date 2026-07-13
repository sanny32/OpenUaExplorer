// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file application.h
/// \brief Declares the application class.
///

#pragma once

#include <QApplication>

#include "appsettings.h"
#include "apptheme.h"

class Application : public QApplication
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs the application, installs the platform style, and applies the initial theme.
    /// \param argc Argument count passed to main.
    /// \param argv Argument vector passed to main.
    ///
    Application(int &argc, char **argv);

    ///
    /// \brief Gives access to the application theme manager.
    /// \return Reference to the application theme manager.
    ///
    AppTheme& theme();

    ///
    /// \brief Returns the running application downcast to this type.
    /// \return Pointer to the global Application instance.
    ///
    static Application *instance();

    ///
    /// \brief Persists the OPC UA timestamp display preference and notifies listeners.
    /// \param mode Timestamp mode to apply.
    ///
    void setTimestampMode(AppSettings::TimestampMode mode);

signals:
    ///
    /// \brief Emitted when the OPC UA timestamp display preference changes.
    /// \param mode The newly applied timestamp mode.
    ///
    void timestampModeChanged(AppSettings::TimestampMode mode);

private:
    void configureCertificateStore();
    void ensureClientCertificate();

    AppTheme _theme;
};

///
/// \brief Returns the running application downcast to this type.
/// \return Pointer to the global Application instance.
///
inline Application *theApp()
{
    return static_cast<Application *>(qApp);
}
