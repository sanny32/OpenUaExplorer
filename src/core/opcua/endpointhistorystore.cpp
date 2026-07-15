// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include "endpointhistorystore.h"
#include "settingsstore.h"

namespace {
constexpr auto lastEndpointUrlKey = "connectionDialog/lastEndpointUrl";
constexpr auto endpointUrlHistoryKey = "connectionDialog/endpointUrlHistory";
constexpr int maximumEndpointHistorySize = 10;
}

///
/// \brief Stores default endpoint URLs only when history settings do not exist yet.
/// \param endpointUrls Default URLs shown on the first application run.
///
void EndpointHistoryStore::seedIfUninitialized(const QStringList &endpointUrls) const
{
    SettingsStore settings;
    if (settings.contains(QLatin1String(endpointUrlHistoryKey))
        || settings.contains(QLatin1String(lastEndpointUrlKey))) {
        return;
    }

    QStringList result;
    for (const QString &endpointUrl : endpointUrls) {
        const QString normalized = endpointUrl.trimmed();
        if (!normalized.isEmpty() && !result.contains(normalized))
            result.append(normalized);
        if (result.size() == maximumEndpointHistorySize)
            break;
    }

    settings.setValue(QLatin1String(endpointUrlHistoryKey), result);
    settings.sync();
}

///
/// \brief Returns the endpoint URL history, with the last-used URL moved to the front.
/// \return Most-recent-first list of endpoint URLs.
///
QStringList EndpointHistoryStore::history() const
{
    SettingsStore settings;
    QStringList result =
        settings.value(QLatin1String(endpointUrlHistoryKey)).toStringList();
    const QString lastEndpoint =
        settings.value(QLatin1String(lastEndpointUrlKey)).toString().trimmed();
    if (!lastEndpoint.isEmpty()) {
        result.removeAll(lastEndpoint);
        result.prepend(lastEndpoint);
    }
    return result;
}

///
/// \brief Records an endpoint URL as most-recent, trimming the history to its cap.
/// \param endpointUrl URL to store; blank values are ignored.
///
void EndpointHistoryStore::save(const QString &endpointUrl) const
{
    const QString normalized = endpointUrl.trimmed();
    if (normalized.isEmpty())
        return;

    SettingsStore settings;
    QStringList result =
        settings.value(QLatin1String(endpointUrlHistoryKey)).toStringList();
    result.removeAll(normalized);
    result.prepend(normalized);
    while (result.size() > maximumEndpointHistorySize)
        result.removeLast();
    settings.setValue(QLatin1String(lastEndpointUrlKey), normalized);
    settings.setValue(QLatin1String(endpointUrlHistoryKey), result);
    settings.sync();
}

///
/// \brief Drops an endpoint URL from the history, clearing it as the last-used URL.
/// \param endpointUrl URL to forget; blank values are ignored.
///
void EndpointHistoryStore::remove(const QString &endpointUrl) const
{
    const QString normalized = endpointUrl.trimmed();
    if (normalized.isEmpty())
        return;

    SettingsStore settings;
    QStringList result =
        settings.value(QLatin1String(endpointUrlHistoryKey)).toStringList();
    result.removeAll(normalized);
    settings.setValue(QLatin1String(endpointUrlHistoryKey), result);

    const QString lastEndpoint =
        settings.value(QLatin1String(lastEndpointUrlKey)).toString().trimmed();
    if (lastEndpoint == normalized)
        settings.remove(QLatin1String(lastEndpointUrlKey));

    settings.sync();
}
