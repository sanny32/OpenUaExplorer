// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file endpointranking.h
/// \brief Declares the shared endpoint security ranking and labelling helpers.
///

#pragma once

#include <QList>
#include <QString>

#include "opcua/opcuatypes.h"

///
/// \brief Security classification shared by every view that lists OPC UA endpoints.
///
namespace EndpointRanking {

///
/// \brief Returns the display name of a security policy, dropping any URI prefix.
/// \param securityPolicy Security policy URI or short name.
/// \return Short policy name, such as "Aes256_Sha256_RsaPss".
///
QString policyShortName(const QString &securityPolicy);

///
/// \brief Reports whether an endpoint uses signing/encryption with a real security policy.
/// \param endpoint Endpoint to classify.
/// \return True when the endpoint is considered secure.
///
bool isSecure(const EndpointInfo &endpoint);

///
/// \brief Scores an endpoint so stronger security ranks higher; insecure endpoints score -1.
/// \param endpoint Endpoint to score.
/// \return Ranking score, higher is better.
///
int rankScore(const EndpointInfo &endpoint);

///
/// \brief Picks the icon name representing a security mode.
/// \param securityModeValue Message security mode value.
/// \return Themed icon name for the mode.
///
QString modeIconName(int securityModeValue);

///
/// \brief Returns the row of the highest-ranked endpoint.
/// \param endpoints Endpoints to search.
/// \return Index of the recommended endpoint, or -1 when the list is empty.
///
int recommendedRow(const QList<EndpointInfo> &endpoints);

///
/// \brief Orders endpoints strongest first, keeping the server's order within a rank.
/// \param endpoints Endpoints to order.
/// \return Ranked copy of the endpoints.
///
QList<EndpointInfo> ranked(const QList<EndpointInfo> &endpoints);

} // namespace EndpointRanking
