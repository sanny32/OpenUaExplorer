// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file appicons.h
/// \brief Provides theme-aware application icon helpers.
///

#pragma once

#include <QAction>
#include <QIcon>
#include <QString>

#include "application.h"

namespace AppIcons {

inline bool isDarkTheme()
{
    return theApp()->theme().isDark();
}

inline QIcon themed(const QString &icon)
{
    return QIcon(QString(":/icons/%1/%2").arg(isDarkTheme() ? "dark" : "light", icon));
}

inline void bindIcon(QAction *action, const QString &icon)
{
    auto refresh = [action, icon] {
        action->setIcon(themed(icon));
    };
    refresh();

    QObject::connect(&theApp()->theme(), &AppTheme::colorSchemeChanged, action, refresh);
}

}
