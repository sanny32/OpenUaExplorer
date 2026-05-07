// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file application.h
/// \brief Declares the application class.
///

#pragma once

#include <QApplication>

#include "appstyle.h"
#include "apptheme.h"

class Application : public QApplication
{
    Q_OBJECT

public:
    Application(int &argc, char **argv);

    AppTheme& theme();

    static Application *instance();

private:
    AppStyle _style;
    AppTheme _theme;
};

inline Application *theApp()
{
    return static_cast<Application *>(qApp);
}
