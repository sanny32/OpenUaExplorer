// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QApplication>

#include "apptheme.h"

class Application : public QApplication
{
    Q_OBJECT

public:
    Application(int &argc, char **argv);

    AppTheme *theme();

    static Application *instance();

private:
    AppTheme _theme;
};

inline Application *theApp()
{
    return static_cast<Application *>(qApp);
}
