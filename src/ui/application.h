// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file application.h
/// \brief Declares the application class.
///

#pragma once

#include <QApplication>

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

private:
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
