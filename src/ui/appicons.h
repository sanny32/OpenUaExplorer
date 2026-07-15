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

///
/// \brief Reports whether the active application theme is dark.
/// \return True when the dark theme is active.
///
inline bool isDarkTheme()
{
    return theApp()->theme().isDark();
}

///
/// \brief Builds an icon from the light or dark resource set.
/// \param icon Icon resource file name.
/// \return Theme-matching icon.
///
inline QIcon themed(const QString &icon)
{
    const QString name = icon.contains(QLatin1Char('.')) ? icon : icon + QStringLiteral(".svg");
    return QIcon(QString(":/icons/%1/%2").arg(isDarkTheme() ? "dark" : "light", name));
}

///
/// \brief Builds the platform-specific application icon.
/// \return Linux application artwork or the existing themed icon on other platforms.
///
inline QIcon application()
{
#ifdef Q_OS_LINUX
    return QIcon(QStringLiteral(":/icons/linux/app.svg"));
#else
    return themed(QStringLiteral("app.ico"));
#endif
}

///
/// \brief Keeps an action icon synchronized with the active theme.
/// \param action Action whose icon should be updated.
/// \param icon Icon resource file name.
///
inline void bindIcon(QAction *action, const QString &icon)
{
    auto refresh = [action, icon] {
        action->setIcon(themed(icon));
    };
    refresh();

    QObject::connect(&theApp()->theme(), &AppTheme::colorSchemeChanged, action, refresh);
}

}
