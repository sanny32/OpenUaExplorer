// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file connectionprofilestore.cpp
/// \brief Implements persistent storage for OPC UA connection profiles.
///

#include <algorithm>
#include <limits>

#include <QSettings>

#include "connectionprofilestore.h"

namespace {
const char profilesGroup[] = "opcua/profiles";
const char favoritesOrderKey[] = "opcua/favoritesOrder";
}

///
/// \brief Returns the saved connection profiles.
/// \return Saved connection profiles.
///
QList<ConnectionProfile> ConnectionProfileStore::profiles() const
{
    QSettings settings;
    QList<ConnectionProfile> result;
    settings.beginGroup(QLatin1String(profilesGroup));
    const QStringList ids = settings.childGroups();
    for (const QString &id : ids) {
        settings.beginGroup(id);
        ConnectionProfile profile;
        profile.id = id;
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
        profile.sessionTimeoutMs = settings.value(QStringLiteral("sessionTimeoutMs"), 60000).toInt();
        profile.connectTimeoutMs = settings.value(QStringLiteral("connectTimeoutMs"), 10000).toInt();
        profile.secureChannelLifetimeMs =
            settings.value(QStringLiteral("secureChannelLifetimeMs"), 600000).toInt();
        profile.endpointTimeoutMs = settings.value(QStringLiteral("endpointTimeoutMs"), 10000).toInt();
        profile.requestTimeoutMs = settings.value(QStringLiteral("requestTimeoutMs"), 15000).toInt();
        const qint64 lastUsedMs = settings.value(QStringLiteral("lastUsed"), 0).toLongLong();
        if (lastUsedMs > 0)
            profile.lastUsed = QDateTime::fromMSecsSinceEpoch(lastUsedMs);
        profile.saveProfile = true;
        settings.endGroup();
        result.append(profile);
    }
    settings.endGroup();

    // Apply the user-defined drag order. Ids absent from the order list (newly added
    // favourites) keep their relative position and sort after the ordered ones.
    const QStringList order = settings.value(QLatin1String(favoritesOrderKey)).toStringList();
    if (!order.isEmpty()) {
        std::stable_sort(result.begin(), result.end(),
                         [&order](const ConnectionProfile &lhs, const ConnectionProfile &rhs) {
            const int lhsRank = order.indexOf(lhs.id);
            const int rhsRank = order.indexOf(rhs.id);
            return (lhsRank < 0 ? std::numeric_limits<int>::max() : lhsRank)
                 < (rhsRank < 0 ? std::numeric_limits<int>::max() : rhsRank);
        });
    }
    return result;
}

///
/// \brief Persists a connection profile, replacing any with the same id.
/// \param profile Profile to persist.
/// \return True when the profile identifier is valid.
///
bool ConnectionProfileStore::save(const ConnectionProfile &profile)
{
    if (profile.id.isEmpty())
        return false;

    QSettings settings;
    settings.beginGroup(QLatin1String(profilesGroup));
    settings.beginGroup(profile.id);
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
    settings.endGroup();
    settings.endGroup();
    settings.sync();
    return settings.status() == QSettings::NoError;
}

///
/// \brief Removes a stored profile by id.
/// \param id Profile identifier.
/// \return True when the settings backend completed successfully.
///
bool ConnectionProfileStore::remove(const QString &id)
{
    QSettings settings;
    settings.beginGroup(QLatin1String(profilesGroup));
    settings.remove(id);
    settings.endGroup();

    QStringList order = settings.value(QLatin1String(favoritesOrderKey)).toStringList();
    if (order.removeAll(id) > 0)
        settings.setValue(QLatin1String(favoritesOrderKey), order);

    settings.sync();
    return settings.status() == QSettings::NoError;
}

///
/// \brief Persists the favourites display order as a list of profile identifiers.
/// \param orderedIds Profile identifiers in their desired order.
/// \return True when the settings backend completed successfully.
///
bool ConnectionProfileStore::setOrder(const QStringList &orderedIds)
{
    QSettings settings;
    settings.setValue(QLatin1String(favoritesOrderKey), orderedIds);
    settings.sync();
    return settings.status() == QSettings::NoError;
}
