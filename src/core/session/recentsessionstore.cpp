// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file recentsessionstore.cpp
/// \brief Implements persistent storage for the most recently used session files.
///

#include <QDir>

#include "recentsessionstore.h"
#include "settingsstore.h"

namespace {
const char recentKey[] = "session/recent";

///
/// \brief Normalizes a path to a canonical form for comparison and storage.
/// \param path Path to normalize.
/// \return Cleaned path with forward slashes.
///
QString normalize(const QString &path)
{
    return QDir::cleanPath(QDir::fromNativeSeparators(path));
}
}

///
/// \brief Returns the recent session file paths, most-recent first.
/// \return Recent session paths.
///
QStringList RecentSessionStore::sessions() const
{
    SettingsStore settings;
    return settings.value(QLatin1String(recentKey)).toStringList();
}

///
/// \brief Records a session file as most-recent, de-duplicating and trimming to the cap.
/// \param path Session file path that was opened or saved.
///
void RecentSessionStore::record(const QString &path)
{
    if (path.isEmpty())
        return;

    const QString canonical = normalize(path);
    QStringList recent = sessions();
    recent.removeAll(canonical);
    recent.prepend(canonical);
    while (recent.size() > maximumSize)
        recent.removeLast();

    SettingsStore settings;
    settings.setValue(QLatin1String(recentKey), recent);
    settings.sync();
}

///
/// \brief Removes a session file from the recent list.
/// \param path Session file path to forget.
///
void RecentSessionStore::remove(const QString &path)
{
    const QString canonical = normalize(path);
    QStringList recent = sessions();
    if (recent.removeAll(canonical) == 0)
        return;

    SettingsStore settings;
    settings.setValue(QLatin1String(recentKey), recent);
    settings.sync();
}
