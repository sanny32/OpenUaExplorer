// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file application.cpp
/// \brief Implements the application class.
///

#include "appstyle.h"
#include "application.h"

#if defined(HAVE_QLEMENTINE_APP_STYLE) && defined(Q_OS_MAC)
#include "macappstyle.h"
#elif defined(HAVE_QLEMENTINE_APP_STYLE)
#include "qlementineappstyle.h"
#endif

///
/// \brief Application::Application
/// \param argc Argument count passed to main.
/// \param argv Argument vector passed to main.
///
Application::Application(int &argc, char **argv)
    : QApplication(argc, argv)
    , _theme(this)
{
#if defined(HAVE_QLEMENTINE_APP_STYLE) && defined(Q_OS_MAC)
    setStyle(new MacAppStyle());
#elif defined(HAVE_QLEMENTINE_APP_STYLE)
    setStyle(new QlementineAppStyle());
#else
    setStyle(new AppStyle());
#endif
    _theme.applyInitialScheme();
}

///
/// \brief Application::theme
/// \return Pointer to the application theme manager.
///
AppTheme& Application::theme()
{
    return _theme;
}

///
/// \brief Application::instance
/// \return Pointer to the global Application instance.
///
Application *Application::instance()
{
    return static_cast<Application *>(QApplication::instance());
}
