// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file endpointranking.cpp
/// \brief Implements the shared endpoint security ranking and labelling helpers.
///

#include <algorithm>

#include "endpointranking.h"

namespace {

///
/// \brief Relative strength of a security policy, used to pick a recommendation.
/// \param policy Short security-policy name (suffix after '#').
/// \return Higher numbers denote stronger policies.
///
int policyStrength(const QString &policy)
{
    if (policy.compare(QStringLiteral("Aes256_Sha256_RsaPss"), Qt::CaseInsensitive) == 0)
        return 6;
    if (policy.compare(QStringLiteral("Aes128_Sha256_RsaOaep"), Qt::CaseInsensitive) == 0)
        return 5;
    if (policy.compare(QStringLiteral("Basic256Sha256"), Qt::CaseInsensitive) == 0)
        return 4;
    if (policy.compare(QStringLiteral("Basic256"), Qt::CaseInsensitive) == 0)
        return 3;
    if (policy.compare(QStringLiteral("Basic128Rsa15"), Qt::CaseInsensitive) == 0)
        return 2;
    if (policy.compare(QStringLiteral("None"), Qt::CaseInsensitive) == 0)
        return 0;
    return 1;
}

} // namespace

namespace EndpointRanking {

///
/// \brief Returns the display name of a security policy, dropping any URI prefix.
/// \param securityPolicy Security policy URI or short name.
/// \return Short policy name, such as "Aes256_Sha256_RsaPss".
///
QString policyShortName(const QString &securityPolicy)
{
    return securityPolicy.section(QLatin1Char('#'), -1);
}

///
/// \brief Reports whether an endpoint uses signing/encryption with a real security policy.
/// \param endpoint Endpoint to classify.
/// \return True when the endpoint is considered secure.
///
bool isSecure(const EndpointInfo &endpoint)
{
    return endpoint.securityModeValue > 1
        && policyShortName(endpoint.securityPolicy)
               .compare(QStringLiteral("None"), Qt::CaseInsensitive)
            != 0;
}

///
/// \brief Scores an endpoint so stronger security ranks higher; insecure endpoints score -1.
/// \param endpoint Endpoint to score.
/// \return Ranking score, higher is better.
///
int rankScore(const EndpointInfo &endpoint)
{
    if (!isSecure(endpoint))
        return -1;
    return policyStrength(policyShortName(endpoint.securityPolicy)) * 10
        + endpoint.securityModeValue;
}

///
/// \brief Picks the icon name representing a security mode.
/// \param securityModeValue Message security mode value.
/// \return Themed icon name for the mode.
///
QString modeIconName(int securityModeValue)
{
    switch (securityModeValue) {
    case 2:
        return QStringLiteral("shield-check");
    case 3:
        return QStringLiteral("lock");
    default:
        return QStringLiteral("unlock");
    }
}

///
/// \brief Returns the row of the highest-ranked endpoint.
/// \param endpoints Endpoints to search.
/// \return Index of the recommended endpoint, or -1 when the list is empty.
///
int recommendedRow(const QList<EndpointInfo> &endpoints)
{
    int best = -1;
    int bestScore = -1;
    for (int row = 0; row < endpoints.size(); ++row) {
        const int score = rankScore(endpoints.at(row));
        if (score > bestScore) {
            bestScore = score;
            best = row;
        }
    }
    return best;
}

///
/// \brief Orders endpoints strongest first, keeping the server's order within a rank.
/// \param endpoints Endpoints to order.
/// \return Ranked copy of the endpoints.
///
QList<EndpointInfo> ranked(const QList<EndpointInfo> &endpoints)
{
    QList<EndpointInfo> result = endpoints;
    std::stable_sort(result.begin(), result.end(),
                     [](const EndpointInfo &a, const EndpointInfo &b) {
                         return rankScore(a) > rankScore(b);
                     });
    return result;
}

} // namespace EndpointRanking
