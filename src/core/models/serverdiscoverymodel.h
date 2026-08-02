// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file serverdiscoverymodel.h
/// \brief Declares the tree model of discovered servers and their endpoints.
///

#pragma once

#include <QAbstractItemModel>
#include <QList>
#include <QString>

#include "opcua/opcuatypes.h"

///
/// \brief Two-level tree of the servers returned by FindServers and their endpoints.
///
/// Endpoints are fetched lazily: a server starts out NotLoaded and reports children so
/// the view offers an expand arrow. Expanding it is what triggers the GetEndpoints
/// request; while it runs, and when it fails, the server shows a single placeholder row.
///
class ServerDiscoveryModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    ///
    /// \brief Load state of one server's endpoint list.
    ///
    enum class LoadState {
        /// \brief Endpoints have not been requested yet.
        NotLoaded,
        /// \brief A GetEndpoints request is in flight.
        Loading,
        /// \brief Endpoints are available.
        Loaded,
        /// \brief The request failed or the server offers no usable discovery URL.
        Failed
    };
    Q_ENUM(LoadState)

    ///
    /// \brief Visible columns of the discovery tree.
    ///
    enum Column {
        PrimaryColumn = 0,
        TransportColumn,
        ColumnCount
    };

    ///
    /// \brief Custom roles exposed by the discovery model.
    ///
    enum Role {
        IsServerRole = Qt::UserRole,
        IsPlaceholderRole,
        SubtitleRole,
        PolicyRole,
        ModeRole,
        IconRole,
        StateRole,
        ServerRole,
        EndpointRole
    };

    ///
    /// \brief Constructs an empty discovery model.
    /// \param parent Owning QObject.
    ///
    explicit ServerDiscoveryModel(QObject *parent = nullptr);

    QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    bool hasChildren(const QModelIndex &parent = {}) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    ///
    /// \brief Replaces the discovered servers, dropping any loaded endpoints.
    /// \param servers Servers in the order the discovery server reported them.
    ///
    void setServers(const QList<ServerInfo> &servers);

    ///
    /// \brief Removes all servers.
    ///
    void clear();

    ///
    /// \brief Returns the number of discovered servers.
    /// \return Server count.
    ///
    int serverCount() const;

    ///
    /// \brief Returns the server at a row.
    /// \param row Server row.
    /// \return Server, or a default-constructed value when out of range.
    ///
    ServerInfo serverAt(int row) const;

    ///
    /// \brief Returns the endpoint load state of a server.
    /// \param row Server row.
    /// \return Load state, or NotLoaded when out of range.
    ///
    LoadState serverState(int row) const;

    ///
    /// \brief Marks a server as loading or failed, replacing its placeholder row.
    /// \param row Server row.
    /// \param state New load state.
    /// \param error Message shown for the Failed state.
    ///
    void setServerState(int row, LoadState state, const QString &error = {});

    ///
    /// \brief Stores a server's endpoints in ranked order and marks it loaded.
    /// \param row Server row.
    /// \param endpoints Endpoints discovered for the server.
    ///
    void setServerEndpoints(int row, const QList<EndpointInfo> &endpoints);

    ///
    /// \brief Returns the opc.tcp discovery URL a server should be queried at.
    /// \param row Server row.
    /// \return First opc.tcp discovery URL, or an empty string when none is advertised.
    ///
    QString discoveryUrl(int row) const;

    ///
    /// \brief Returns the server row an index belongs to.
    /// \param index Server or endpoint index.
    /// \return Server row, or -1 for an invalid index.
    ///
    int serverRowOf(const QModelIndex &index) const;

    ///
    /// \brief Returns the endpoint an index refers to.
    /// \param index Index to resolve.
    /// \return Endpoint, or a default-constructed value for non-endpoint indexes.
    ///
    EndpointInfo endpointAt(const QModelIndex &index) const;

    ///
    /// \brief Reports whether an index refers to an endpoint rather than a server
    ///        or a placeholder.
    /// \param index Index to test.
    /// \return True for endpoint indexes.
    ///
    bool isEndpoint(const QModelIndex &index) const;

private:
    struct ServerNode
    {
        ServerInfo info;
        QList<EndpointInfo> endpoints;
        LoadState state = LoadState::NotLoaded;
        QString error;
    };

    void replaceChildren(int row, LoadState state, const QList<EndpointInfo> &endpoints,
                         const QString &error);
    int childCount(const ServerNode &server) const;
    QString placeholderText(const ServerNode &server) const;

    QList<ServerNode> _servers;
};
