// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file recentconnectionstore.cpp
/// \brief Implements persistent storage for the most recent OPC UA connections.
///

#include <algorithm>

#include "recentconnectionstore.h"
#include "settingsstore.h"

namespace {
const char recentArrayKey[] = "opcua/recent";

///
/// \brief Reads one connection profile from the current QSettings array element.
/// \param settings Settings positioned on the array element.
/// \return The decoded profile.
///
ConnectionProfile readProfile(const QSettings &settings)
{
    ConnectionProfile profile;
    profile.id = settings.value(QStringLiteral("id")).toString();
    profile.name = settings.value(QStringLiteral("name")).toString();
    profile.sessionName = settings.value(QStringLiteral("sessionName")).toString();
    profile.backend = settings.value(QStringLiteral("backend"), QStringLiteral("open62541")).toString();
    profile.endpointUrl = settings.value(QStringLiteral("endpointUrl")).toString();
    profile.securityPolicy = settings.value(QStringLiteral("securityPolicy")).toString();
    profile.securityMode = settings.value(QStringLiteral("securityMode"), 1).toInt();
    profile.authentication = static_cast<ConnectionProfile::Authentication>(
        settings.value(QStringLiteral("authentication"), 0).toInt());
    profile.username = settings.value(QStringLiteral("username")).toString();
    profile.clientCertificateFile = settings.value(QStringLiteral("clientCertificateFile")).toString();
    profile.privateKeyFile = settings.value(QStringLiteral("privateKeyFile")).toString();
    profile.sessionTimeoutMs = settings.value(QStringLiteral("sessionTimeoutMs"), 600000).toInt();
    profile.connectTimeoutMs = settings.value(QStringLiteral("connectTimeoutMs"), 10000).toInt();
    profile.secureChannelLifetimeMs =
        settings.value(QStringLiteral("secureChannelLifetimeMs"), 600000).toInt();
    profile.endpointTimeoutMs = settings.value(QStringLiteral("endpointTimeoutMs"), 10000).toInt();
    profile.requestTimeoutMs = settings.value(QStringLiteral("requestTimeoutMs"), 15000).toInt();
    const qint64 lastUsedMs = settings.value(QStringLiteral("lastUsed"), 0).toLongLong();
    if (lastUsedMs > 0)
        profile.lastUsed = QDateTime::fromMSecsSinceEpoch(lastUsedMs);
    return profile;
}

///
/// \brief Writes one connection profile to the current QSettings array element.
/// \param settings Settings positioned on the array element.
/// \param profile Profile to encode.
///
void writeProfile(QSettings &settings, const ConnectionProfile &profile)
{
    settings.setValue(QStringLiteral("id"), profile.id);
    settings.setValue(QStringLiteral("name"), profile.name);
    settings.setValue(QStringLiteral("sessionName"), profile.sessionName);
    settings.setValue(QStringLiteral("backend"), profile.backend);
    settings.setValue(QStringLiteral("endpointUrl"), profile.endpointUrl);
    settings.setValue(QStringLiteral("securityPolicy"), profile.securityPolicy);
    settings.setValue(QStringLiteral("securityMode"), profile.securityMode);
    settings.setValue(QStringLiteral("authentication"), static_cast<int>(profile.authentication));
    settings.setValue(QStringLiteral("username"), profile.username);
    settings.setValue(QStringLiteral("clientCertificateFile"), profile.clientCertificateFile);
    settings.setValue(QStringLiteral("privateKeyFile"), profile.privateKeyFile);
    settings.setValue(QStringLiteral("sessionTimeoutMs"), profile.sessionTimeoutMs);
    settings.setValue(QStringLiteral("connectTimeoutMs"), profile.connectTimeoutMs);
    settings.setValue(QStringLiteral("secureChannelLifetimeMs"), profile.secureChannelLifetimeMs);
    settings.setValue(QStringLiteral("endpointTimeoutMs"), profile.endpointTimeoutMs);
    settings.setValue(QStringLiteral("requestTimeoutMs"), profile.requestTimeoutMs);
    settings.setValue(QStringLiteral("lastUsed"),
                      profile.lastUsed.isValid() ? profile.lastUsed.toMSecsSinceEpoch() : 0);
}
}

///
/// \brief Returns the recent connections, most-recent first.
/// \return Recent connection profiles.
///
QList<ConnectionProfile> RecentConnectionStore::connections() const
{
    SettingsStore settings;
    QList<ConnectionProfile> result;
    const int count = settings.beginReadArray(QLatin1String(recentArrayKey));
    for (int index = 0; index < count; ++index) {
        settings.setArrayIndex(index);
        result.append(readProfile(settings));
    }
    settings.endArray();
    return result;
}

///
/// \brief Records a connection as most-recent, replacing any with the same endpoint URL
///        and trimming the list to its cap.
/// \param profile Profile that was connected.
///
void RecentConnectionStore::record(const ConnectionProfile &profile)
{
    if (profile.endpointUrl.isEmpty())
        return;

    QList<ConnectionProfile> recent = connections();
    recent.erase(std::remove_if(recent.begin(), recent.end(),
                                [&profile](const ConnectionProfile &existing) {
                                    return existing.endpointUrl == profile.endpointUrl;
                                }),
                 recent.end());
    recent.prepend(profile);
    while (recent.size() > maximumSize)
        recent.removeLast();

    SettingsStore settings;
    settings.remove(QLatin1String(recentArrayKey));
    settings.beginWriteArray(QLatin1String(recentArrayKey), recent.size());
    for (int index = 0; index < recent.size(); ++index) {
        settings.setArrayIndex(index);
        writeProfile(settings, recent.at(index));
    }
    settings.endArray();
    settings.sync();
}
