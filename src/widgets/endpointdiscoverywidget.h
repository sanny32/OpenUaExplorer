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
    explicit EndpointDiscoveryWidget(QWidget *parent = nullptr);

    void setEndpoints(const QList<EndpointInfo> &endpoints);
    void clear();

    int endpointCount() const;
    int currentRow() const;
    bool hasSelection() const;
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
