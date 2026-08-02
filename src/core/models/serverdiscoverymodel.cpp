// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file serverdiscoverymodel.cpp
/// \brief Implements the tree model of discovered servers and their endpoints.
///

#include <QUrl>

#include "endpointranking.h"
#include "serverdiscoverymodel.h"

namespace {

/// \brief Scheme the application can connect with.
constexpr auto opcTcpScheme = "opc.tcp";

///
/// \brief Number of child rows a server shows for a given load state.
/// \param state Load state of the server.
/// \param endpointCount Number of endpoints already loaded.
/// \return Child row count; loading, failed and empty servers show one placeholder.
///
int childCountFor(ServerDiscoveryModel::LoadState state, int endpointCount)
{
    switch (state) {
    case ServerDiscoveryModel::LoadState::NotLoaded:
        return 0;
    case ServerDiscoveryModel::LoadState::Loaded:
        return endpointCount > 0 ? endpointCount : 1;
    case ServerDiscoveryModel::LoadState::Loading:
    case ServerDiscoveryModel::LoadState::Failed:
        break;
    }
    return 1;
}

}

///
/// \brief Constructs an empty discovery model.
/// \param parent Owning QObject.
///
ServerDiscoveryModel::ServerDiscoveryModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}

///
/// \brief Builds an index for a server row or one of its child rows.
/// \param row Row within the parent.
/// \param column Column index.
/// \param parent Parent index; invalid for server rows.
/// \return Index, or an invalid one when out of range.
///
QModelIndex ServerDiscoveryModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row < 0 || column < 0 || column >= ColumnCount)
        return {};

    if (!parent.isValid())
        return row < _servers.size() ? createIndex(row, column, quintptr(0)) : QModelIndex();

    if (parent.internalId() != 0 || parent.row() >= _servers.size())
        return {};
    if (row >= childCount(_servers.at(parent.row())))
        return {};
    return createIndex(row, column, quintptr(parent.row() + 1));
}

///
/// \brief Returns the server index a child row belongs to.
/// \param child Index whose parent is requested.
/// \return Parent index, or an invalid one for server rows.
///
QModelIndex ServerDiscoveryModel::parent(const QModelIndex &child) const
{
    if (!child.isValid() || child.internalId() == 0)
        return {};
    return createIndex(int(child.internalId()) - 1, PrimaryColumn, quintptr(0));
}

///
/// \brief Returns the number of servers, or a server's child rows.
/// \param parent Parent index.
/// \return Row count below the parent.
///
int ServerDiscoveryModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return _servers.size();
    if (parent.internalId() != 0 || parent.row() >= _servers.size())
        return 0;
    return childCount(_servers.at(parent.row()));
}

///
/// \brief Returns the fixed number of columns.
/// \param parent Parent index.
/// \return Column count.
///
int ServerDiscoveryModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return ColumnCount;
}

///
/// \brief Reports children for servers whose endpoints have not been requested yet.
/// \param parent Index to test.
/// \return True when the row can be expanded.
///
/// Endpoints load on expansion, so a NotLoaded server must claim children it cannot
/// list yet; otherwise the view would never offer the arrow that starts the request.
///
bool ServerDiscoveryModel::hasChildren(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return !_servers.isEmpty();
    if (parent.internalId() != 0 || parent.row() >= _servers.size())
        return false;
    const ServerNode &server = _servers.at(parent.row());
    return server.state == LoadState::NotLoaded || childCount(server) > 0;
}

///
/// \brief Returns the item flags, making placeholder rows unselectable.
/// \param index Index to describe.
/// \return Item flags for the row.
///
Qt::ItemFlags ServerDiscoveryModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    if (index.data(IsPlaceholderRole).toBool())
        return Qt::ItemIsEnabled;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

///
/// \brief Returns server, endpoint, or placeholder data for a cell and role.
/// \param index Cell to query.
/// \param role Display or custom discovery role.
/// \return Value for the role, or an invalid variant.
///
QVariant ServerDiscoveryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    if (index.internalId() == 0) {
        if (index.row() >= _servers.size())
            return {};
        const ServerNode &server = _servers.at(index.row());
        const QString name = server.info.applicationName.isEmpty()
            ? server.info.applicationUri
            : server.info.applicationName;
        const QString subtitle = server.info.discoveryUrls.isEmpty()
            ? server.info.applicationUri
            : server.info.discoveryUrls.constFirst();

        switch (role) {
        case Qt::DisplayRole:
            return index.column() == PrimaryColumn ? name : QString();
        case Qt::ToolTipRole:
            return subtitle;
        case IsServerRole:
            return true;
        case IsPlaceholderRole:
            return false;
        case SubtitleRole:
            return subtitle;
        case IconRole:
            return QStringLiteral("server");
        case StateRole:
            return QVariant::fromValue(server.state);
        case ServerRole:
            return QVariant::fromValue(server.info);
        default:
            return {};
        }
    }

    const int serverRow = int(index.internalId()) - 1;
    if (serverRow < 0 || serverRow >= _servers.size())
        return {};
    const ServerNode &server = _servers.at(serverRow);

    if (server.state != LoadState::Loaded || server.endpoints.isEmpty()) {
        switch (role) {
        case Qt::DisplayRole:
            return index.column() == PrimaryColumn ? placeholderText(server) : QString();
        case IsServerRole:
            return false;
        case IsPlaceholderRole:
            return true;
        case StateRole:
            return QVariant::fromValue(server.state);
        default:
            return {};
        }
    }

    if (index.row() >= server.endpoints.size())
        return {};
    const EndpointInfo &endpoint = server.endpoints.at(index.row());
    const QString policy = EndpointRanking::policyShortName(endpoint.securityPolicy);

    switch (role) {
    case Qt::DisplayRole:
        if (index.column() == TransportColumn)
            return QUrl(endpoint.endpointUrl).scheme();
        return QStringLiteral("%1 — %2").arg(policy, endpoint.securityMode);
    case Qt::ToolTipRole:
        return endpoint.endpointUrl;
    case IsServerRole:
        return false;
    case IsPlaceholderRole:
        return false;
    case PolicyRole:
        return policy;
    case ModeRole:
        return endpoint.securityMode;
    case IconRole:
        return EndpointRanking::modeIconName(endpoint.securityModeValue);
    case EndpointRole:
        return QVariant::fromValue(endpoint);
    default:
        return {};
    }
}

///
/// \brief Exposes the custom role names for delegate access.
/// \return Role-id to name mapping.
///
QHash<int, QByteArray> ServerDiscoveryModel::roleNames() const
{
    auto roles = QAbstractItemModel::roleNames();
    roles.insert(IsServerRole, "isServer");
    roles.insert(IsPlaceholderRole, "isPlaceholder");
    roles.insert(SubtitleRole, "subtitle");
    roles.insert(PolicyRole, "policy");
    roles.insert(ModeRole, "mode");
    roles.insert(IconRole, "icon");
    roles.insert(StateRole, "state");
    roles.insert(ServerRole, "server");
    roles.insert(EndpointRole, "endpoint");
    return roles;
}

///
/// \brief Replaces the discovered servers, dropping any loaded endpoints.
/// \param servers Servers in the order the discovery server reported them.
///
void ServerDiscoveryModel::setServers(const QList<ServerInfo> &servers)
{
    beginResetModel();
    _servers.clear();
    _servers.reserve(servers.size());
    for (const ServerInfo &server : servers)
        _servers.append(ServerNode{server, {}, LoadState::NotLoaded, {}});
    endResetModel();
}

///
/// \brief Removes all servers.
///
void ServerDiscoveryModel::clear()
{
    setServers({});
}

///
/// \brief Returns the number of discovered servers.
/// \return Server count.
///
int ServerDiscoveryModel::serverCount() const
{
    return _servers.size();
}

///
/// \brief Returns the server at a row.
/// \param row Server row.
/// \return Server, or a default-constructed value when out of range.
///
ServerInfo ServerDiscoveryModel::serverAt(int row) const
{
    return row >= 0 && row < _servers.size() ? _servers.at(row).info : ServerInfo{};
}

///
/// \brief Returns the endpoint load state of a server.
/// \param row Server row.
/// \return Load state, or NotLoaded when out of range.
///
ServerDiscoveryModel::LoadState ServerDiscoveryModel::serverState(int row) const
{
    return row >= 0 && row < _servers.size() ? _servers.at(row).state : LoadState::NotLoaded;
}

///
/// \brief Marks a server as loading or failed, replacing its placeholder row.
/// \param row Server row.
/// \param state New load state.
/// \param error Message shown for the Failed state.
///
void ServerDiscoveryModel::setServerState(int row, LoadState state, const QString &error)
{
    replaceChildren(row, state, {}, error);
}

///
/// \brief Stores a server's endpoints in ranked order and marks it loaded.
/// \param row Server row.
/// \param endpoints Endpoints discovered for the server.
///
void ServerDiscoveryModel::setServerEndpoints(int row, const QList<EndpointInfo> &endpoints)
{
    replaceChildren(row, LoadState::Loaded, EndpointRanking::ranked(endpoints), {});
}

///
/// \brief Returns the opc.tcp discovery URL a server should be queried at.
/// \param row Server row.
/// \return First opc.tcp discovery URL, or an empty string when none is advertised.
///
QString ServerDiscoveryModel::discoveryUrl(int row) const
{
    if (row < 0 || row >= _servers.size())
        return {};
    for (const QString &url : _servers.at(row).info.discoveryUrls) {
        if (QUrl(url).scheme().compare(QLatin1String(opcTcpScheme), Qt::CaseInsensitive) == 0)
            return url;
    }
    return {};
}

///
/// \brief Returns the server row an index belongs to.
/// \param index Server or endpoint index.
/// \return Server row, or -1 for an invalid index.
///
int ServerDiscoveryModel::serverRowOf(const QModelIndex &index) const
{
    if (!index.isValid())
        return -1;
    return index.internalId() == 0 ? index.row() : int(index.internalId()) - 1;
}

///
/// \brief Returns the endpoint an index refers to.
/// \param index Index to resolve.
/// \return Endpoint, or a default-constructed value for non-endpoint indexes.
///
EndpointInfo ServerDiscoveryModel::endpointAt(const QModelIndex &index) const
{
    if (!isEndpoint(index))
        return {};
    return _servers.at(int(index.internalId()) - 1).endpoints.at(index.row());
}

///
/// \brief Reports whether an index refers to an endpoint rather than a server
///        or a placeholder.
/// \param index Index to test.
/// \return True for endpoint indexes.
///
bool ServerDiscoveryModel::isEndpoint(const QModelIndex &index) const
{
    if (!index.isValid() || index.internalId() == 0)
        return false;
    const int serverRow = int(index.internalId()) - 1;
    if (serverRow < 0 || serverRow >= _servers.size())
        return false;
    const ServerNode &server = _servers.at(serverRow);
    return server.state == LoadState::Loaded && index.row() < server.endpoints.size();
}

///
/// \brief Swaps a server's child rows for the ones a new load state produces.
/// \param row Server row.
/// \param state New load state.
/// \param endpoints Endpoints to store, already ranked.
/// \param error Message shown for the Failed state.
///
void ServerDiscoveryModel::replaceChildren(int row, LoadState state,
                                           const QList<EndpointInfo> &endpoints,
                                           const QString &error)
{
    if (row < 0 || row >= _servers.size())
        return;

    ServerNode &server = _servers[row];
    const QModelIndex parentIndex = index(row, PrimaryColumn);
    const int before = childCount(server);
    if (before > 0) {
        beginRemoveRows(parentIndex, 0, before - 1);
        server.endpoints.clear();
        server.state = LoadState::NotLoaded;
        endRemoveRows();
    }

    const int after = childCountFor(state, endpoints.size());
    if (after > 0)
        beginInsertRows(parentIndex, 0, after - 1);
    server.state = state;
    server.endpoints = endpoints;
    server.error = error;
    if (after > 0)
        endInsertRows();

    emit dataChanged(parentIndex, index(row, ColumnCount - 1));
}

///
/// \brief Returns how many child rows a server shows.
/// \param server Server node.
/// \return Child row count.
///
int ServerDiscoveryModel::childCount(const ServerNode &server) const
{
    return childCountFor(server.state, server.endpoints.size());
}

///
/// \brief Returns the text of a server's single placeholder row.
/// \param server Server node.
/// \return Placeholder text for the server's load state.
///
QString ServerDiscoveryModel::placeholderText(const ServerNode &server) const
{
    switch (server.state) {
    case LoadState::Loading:
        return tr("Loading endpoints…");
    case LoadState::Failed:
        return server.error.isEmpty() ? tr("Endpoints could not be loaded.") : server.error;
    case LoadState::Loaded:
        return tr("No endpoints available.");
    case LoadState::NotLoaded:
        break;
    }
    return {};
}
