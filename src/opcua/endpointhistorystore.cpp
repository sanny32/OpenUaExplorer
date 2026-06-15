// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include <QSettings>

#include "endpointhistorystore.h"

namespace {
constexpr auto lastEndpointUrlKey = "connectionDialog/lastEndpointUrl";
constexpr auto endpointUrlHistoryKey = "connectionDialog/endpointUrlHistory";
constexpr int maximumEndpointHistorySize = 10;
}

QStringList EndpointHistoryStore::history() const
{
    QSettings settings;
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

void EndpointHistoryStore::save(const QString &endpointUrl) const
{
    const QString normalized = endpointUrl.trimmed();
    if (normalized.isEmpty())
        return;

    QSettings settings;
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
