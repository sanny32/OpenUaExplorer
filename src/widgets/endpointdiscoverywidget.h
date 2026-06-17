// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file endpointdiscoverywidget.h
/// \brief Declares the discovered-endpoint table widget.
///

#pragma once

#include <QWidget>

#include "opcua/opcuatypes.h"

class QLabel;
class QTableView;
class EndpointModel;

///
/// \brief Shows discovered OPC UA endpoints in a selectable status table.
///
/// Bundles the "Discovered Endpoints" caption, the endpoint table, its model
/// and the cell delegates (radio selector, security text and status badge) so
/// the connection dialog can treat endpoint discovery as a single control.
///
class EndpointDiscoveryWidget : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the endpoint table with its custom row delegates and hover tracking.
    /// \param parent Owning widget.
    ///
    explicit EndpointDiscoveryWidget(QWidget *parent = nullptr);

    ///
    /// \brief Shows the discovered endpoints, updating the count and selecting the first row.
    /// \param endpoints Discovered endpoints to display.
    ///
    void setEndpoints(const QList<EndpointInfo> &endpoints);

    ///
    /// \brief Removes all endpoints.
    ///
    void clear();

    ///
    /// \brief Returns how many endpoints are shown.
    /// \return Number of discovered endpoints.
    ///
    int endpointCount() const;

    ///
    /// \brief Returns the selected row index.
    /// \return Selected endpoint row, or -1 when none is selected.
    ///
    int currentRow() const;

    ///
    /// \brief Reports whether a valid endpoint is selected.
    /// \return True when a valid endpoint row is selected.
    ///
    bool hasSelection() const;

    ///
    /// \brief Returns the selected endpoint.
    /// \return Endpoint for the selected row.
    ///
    EndpointInfo currentEndpoint() const;

signals:
    ///
    /// \brief Emitted whenever the selected endpoint row changes.
    ///
    void currentEndpointChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void setHoveredRow(int row);

    QLabel *_titleLabel;
    QTableView *_view;
    EndpointModel *_model;
};
