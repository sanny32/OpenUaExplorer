// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include <algorithm>
#include "endpointmodel.h"

namespace {
///
/// \brief Picks the icon name representing a security mode.
/// \param securityMode Message security mode value.
/// \return Icon file name for the mode.
///
QString securityIconName(int securityMode)
{
    switch (securityMode) {
    case 2:
        return QStringLiteral("shield-check.svg");
    case 3:
        return QStringLiteral("lock.svg");
    default:
        return QStringLiteral("unlock.svg");
    }
}

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
}

///
/// \brief Constructs an empty endpoint model.
/// \param parent Owning QObject.
///
EndpointModel::EndpointModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

///
/// \brief Returns the number of endpoint rows.
/// \param parent Parent index; non-root parents have no rows.
/// \return Endpoint count for the root, otherwise 0.
///
int EndpointModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : _endpoints.size();
}

///
/// \brief Returns the fixed number of columns.
/// \param parent Parent index; non-root parents have no columns.
/// \return Column count for the root, otherwise 0.
///
int EndpointModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

///
/// \brief Reports whether an endpoint uses signing/encryption with a real security policy.
/// \param endpoint Endpoint to classify.
/// \return True when the endpoint is considered secure.
///
bool EndpointModel::isSecure(const EndpointInfo &endpoint) const
{
    const QString policy = endpoint.securityPolicy.section(QLatin1Char('#'), -1);
    return endpoint.securityModeValue > 1
        && policy.compare(QStringLiteral("None"), Qt::CaseInsensitive) != 0;
}

///
/// \brief Scores an endpoint so stronger security ranks higher; insecure endpoints score -1.
/// \param endpoint Endpoint to score.
/// \return Ranking score, higher is better.
///
int EndpointModel::rankScore(const EndpointInfo &endpoint) const
{
    if (!isSecure(endpoint))
        return -1;
    const QString policy = endpoint.securityPolicy.section(QLatin1Char('#'), -1);
    return policyStrength(policy) * 10 + endpoint.securityModeValue;
}

///
/// \brief Finds the row of the highest-ranked endpoint.
/// \return Index of the recommended endpoint, or -1 when empty.
///
int EndpointModel::recommendedRow() const
{
    int best = -1;
    int bestScore = -1;
    for (int row = 0; row < _endpoints.size(); ++row) {
        const int score = rankScore(_endpoints.at(row));
        if (score > bestScore) {
            bestScore = score;
            best = row;
        }
    }
    return best;
}

///
/// \brief Returns endpoint data for a cell and role, including status text, colour, and icon.
/// \param index Cell to query.
/// \param role Display or custom endpoint role.
/// \return Value for the role, or an invalid variant.
///
QVariant EndpointModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= _endpoints.size())
        return {};
    const EndpointInfo &endpoint = _endpoints.at(index.row());
    const QString policy = endpoint.securityPolicy.section(QLatin1Char('#'), -1);
    const bool recommended = index.row() == _recommendedRow;
    const bool secure = isSecure(endpoint);

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case PolicyColumn:
            return policy;
        case ModeColumn:
            return endpoint.securityMode;
        case StatusColumn:
            return recommended ? tr("Recommended")
                               : (secure ? tr("Good") : tr("Not secure"));
        default:
            return {};
        }
    case PolicyRole:
        return policy;
    case ModeRole:
        return endpoint.securityMode;
    case IconRole:
        return securityIconName(endpoint.securityModeValue);
    case StatusRole:
        return recommended ? tr("Recommended")
                           : (secure ? tr("Good") : tr("Not secure"));
    case StatusColorRole:
        if (recommended)
            return QColor(0x2e, 0x9e, 0x44);
        return secure ? QColor(0xc0, 0x7d, 0x00) : QColor(0xd1, 0x34, 0x38);
    case RecommendedRole:
        return recommended;
    case EndpointRole:
        return QVariant::fromValue(endpoint);
    default:
        return {};
    }
}

///
/// \brief Returns the horizontal header titles.
/// \param section Column index.
/// \param orientation Header orientation.
/// \param role Display role.
/// \return Column title, or the base implementation for other roles.
///
QVariant EndpointModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QAbstractTableModel::headerData(section, orientation, role);

    switch (section) {
    case PolicyColumn:
        return tr("Security Policy");
    case ModeColumn:
        return tr("Security Mode");
    case StatusColumn:
        return tr("Message Security / Status");
    default:
        return {};
    }
}

///
/// \brief Exposes the custom role names for QML/delegate access.
/// \return Role-id to name mapping.
///
QHash<int, QByteArray> EndpointModel::roleNames() const
{
    auto roles = QAbstractTableModel::roleNames();
    roles.insert(PolicyRole, "policy");
    roles.insert(ModeRole, "mode");
    roles.insert(IconRole, "icon");
    roles.insert(StatusRole, "status");
    roles.insert(StatusColorRole, "statusColor");
    roles.insert(RecommendedRole, "recommended");
    roles.insert(EndpointRole, "endpoint");
    return roles;
}

///
/// \brief Replaces the endpoints, sorting by rank and caching the recommended row.
/// \param endpoints Endpoints to display.
///
void EndpointModel::setEndpoints(const QList<EndpointInfo> &endpoints)
{
    beginResetModel();
    _endpoints = endpoints;
    std::stable_sort(_endpoints.begin(), _endpoints.end(),
                     [this](const EndpointInfo &a, const EndpointInfo &b) {
                         return rankScore(a) > rankScore(b);
                     });
    _recommendedRow = recommendedRow();
    endResetModel();
}

///
/// \brief Removes all endpoints.
///
void EndpointModel::clear()
{
    setEndpoints({});
}

///
/// \brief Returns the endpoint at a row.
/// \param row Row index.
/// \return Endpoint, or a default-constructed value when out of range.
///
EndpointInfo EndpointModel::endpointAt(int row) const
{
    return row >= 0 && row < _endpoints.size()
        ? _endpoints.at(row)
        : EndpointInfo{};
}

///
/// \brief Returns the current endpoints in ranked order.
/// \return The sorted endpoint list.
///
const QList<EndpointInfo> &EndpointModel::endpoints() const
{
    return _endpoints;
}
