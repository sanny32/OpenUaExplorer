// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include "endpointmodel.h"
#include "endpointranking.h"

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
    const QString policy = EndpointRanking::policyShortName(endpoint.securityPolicy);
    const bool recommended = index.row() == _recommendedRow;
    const bool secure = EndpointRanking::isSecure(endpoint);

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
        return EndpointRanking::modeIconName(endpoint.securityModeValue);
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
        return tr("Status");
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
    _endpoints = EndpointRanking::ranked(endpoints);
    _recommendedRow = EndpointRanking::recommendedRow(_endpoints);
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
