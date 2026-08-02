// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file serverdiscoverywidget.h
/// \brief Declares the discovered-server tree widget.
///

#pragma once

#include <QWidget>

#include "opcua/opcuatypes.h"

class QStackedWidget;
class QTreeView;
class ServerDiscoveryModel;

///
/// \brief Shows discovered OPC UA servers and their endpoints in a selectable tree.
///
/// Bundles the tree, its model, the row delegates and the "no servers found" state so
/// the find-servers dialog can treat server discovery as a single control. Endpoints
/// are not fetched here: expanding a server emits serverExpanded() and the owner is
/// expected to fill the branch in.
///
class ServerDiscoveryWidget : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the tree with its row delegates and the empty state.
    /// \param parent Owning widget.
    ///
    explicit ServerDiscoveryWidget(QWidget *parent = nullptr);

    ///
    /// \brief Shows the discovered servers, collapsed and without endpoints.
    /// \param servers Servers to display.
    ///
    void setServers(const QList<ServerInfo> &servers);

    ///
    /// \brief Removes all servers and returns to the empty state.
    ///
    void clear();

    ///
    /// \brief Returns the underlying model so the owner can drive the lazy loading.
    /// \return Discovery model.
    ///
    ServerDiscoveryModel *model() const;

    ///
    /// \brief Reports whether the current row is an endpoint.
    /// \return True when an endpoint is selected.
    ///
    bool hasEndpointSelection() const;

    ///
    /// \brief Returns the selected endpoint.
    /// \return Endpoint for the current row, or a default-constructed value.
    ///
    EndpointInfo currentEndpoint() const;

    ///
    /// \brief Selects the first endpoint of a server, if it has any.
    /// \param serverRow Server row.
    ///
    void selectFirstEndpoint(int serverRow);

signals:
    ///
    /// \brief Emitted whenever the current row changes.
    ///
    void currentEndpointChanged();

    ///
    /// \brief Emitted when a server row is expanded.
    /// \param serverRow Row of the expanded server.
    ///
    void serverExpanded(int serverRow);

    ///
    /// \brief Emitted when an endpoint row is double-clicked.
    ///
    void endpointActivated();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void setHoveredRow(const QModelIndex &index);
    void updateEmptyState();

    QStackedWidget *_pages;
    QTreeView *_view;
    ServerDiscoveryModel *_model;
};
