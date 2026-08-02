// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include "endpointhistorystore.h"
#include "settingsstore.h"

namespace {
constexpr int maximumEndpointHistorySize = 10;
}

///
/// \brief Binds the store to a settings group.
/// \param group Settings group holding the history keys.
///
EndpointHistoryStore::EndpointHistoryStore(QString group)
    : _lastUrlKey(group + QStringLiteral("/lastEndpointUrl"))
    , _historyKey(group + QStringLiteral("/endpointUrlHistory"))
{
}

///
/// \brief Stores default endpoint URLs only when history settings do not exist yet.
/// \param endpointUrls Default URLs shown on the first application run.
///
void EndpointHistoryStore::seedIfUninitialized(const QStringList &endpointUrls) const
{
    SettingsStore settings;
    if (settings.contains(_historyKey) || settings.contains(_lastUrlKey))
        return;

    QStringList result;
    for (const QString &endpointUrl : endpointUrls) {
        const QString normalized = endpointUrl.trimmed();
        if (!normalized.isEmpty() && !result.contains(normalized))
            result.append(normalized);
        if (result.size() == maximumEndpointHistorySize)
            break;
    }

    settings.setValue(_historyKey, result);
    settings.sync();
}

///
/// \brief Returns the endpoint URL history, with the last-used URL moved to the front.
/// \return Most-recent-first list of endpoint URLs.
///
QStringList EndpointHistoryStore::history() const
{
    SettingsStore settings;
    QStringList result = settings.value(_historyKey).toStringList();
    const QString lastEndpoint = settings.value(_lastUrlKey).toString().trimmed();
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
    QStringList result = settings.value(_historyKey).toStringList();
    result.removeAll(normalized);
    result.prepend(normalized);
    while (result.size() > maximumEndpointHistorySize)
        result.removeLast();
    settings.setValue(_lastUrlKey, normalized);
    settings.setValue(_historyKey, result);
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
    QStringList result = settings.value(_historyKey).toStringList();
    result.removeAll(normalized);
    settings.setValue(_historyKey, result);

    if (settings.value(_lastUrlKey).toString().trimmed() == normalized)
        settings.remove(_lastUrlKey);

    settings.sync();
}
