// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include "application.h"

Application::Application(int &argc, char **argv)
    : QApplication(argc, argv)
    , _theme(this)
{
}

AppTheme *Application::theme()
{
    return &_theme;
}

Application *Application::instance()
{
    return static_cast<Application *>(QApplication::instance());
}
