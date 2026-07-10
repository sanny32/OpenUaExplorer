// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QAbstractTableModel>
#include <QColor>

#include "opcua/opcuatypes.h"

///
/// \brief Table model for discovered OPC UA endpoints.
///
class EndpointModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    ///
    /// \brief Visible columns of the discovered-endpoint table.
    ///
    enum Column {
        SelectColumn = 0,
        PolicyColumn,
        ModeColumn,
        StatusColumn,
        ColumnCount
    };

    ///
    /// \brief Custom roles exposed by the endpoint model.
    ///
    enum Role {
        PolicyRole = Qt::UserRole,
        ModeRole,
        IconRole,
        StatusRole,
        StatusColorRole,
        RecommendedRole,
        EndpointRole
    };

    ///
    /// \brief Constructs an empty endpoint model.
    /// \param parent Owning QObject.
    ///
    explicit EndpointModel(QObject *parent = nullptr);

    ///
    /// \brief Returns the number of endpoint rows.
    /// \param parent Parent index; non-root parents have no rows.
    /// \return Endpoint count for the root, otherwise 0.
    ///
    int rowCount(const QModelIndex &parent = {}) const override;

    ///
    /// \brief Returns the fixed number of columns.
    /// \param parent Parent index; non-root parents have no columns.
    /// \return Column count for the root, otherwise 0.
    ///
    int columnCount(const QModelIndex &parent = {}) const override;

    ///
    /// \brief Returns endpoint data for a cell and role, including status text, colour, and icon.
    /// \param index Cell to query.
    /// \param role Display or custom endpoint role.
    /// \return Value for the role, or an invalid variant.
    ///
    QVariant data(const QModelIndex &index, int role) const override;

    ///
    /// \brief Returns the horizontal header titles.
    /// \param section Column index.
    /// \param orientation Header orientation.
    /// \param role Display role.
    /// \return Column title, or the base implementation for other roles.
    ///
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    ///
    /// \brief Exposes the custom role names for QML/delegate access.
    /// \return Role-id to name mapping.
    ///
    QHash<int, QByteArray> roleNames() const override;

    ///
    /// \brief Replaces the endpoints, sorting by rank and caching the recommended row.
    /// \param endpoints Endpoints to display.
    ///
    void setEndpoints(const QList<EndpointInfo> &endpoints);

    ///
    /// \brief Removes all endpoints.
    ///
    void clear();

    ///
    /// \brief Returns the endpoint at a row.
    /// \param row Row index.
    /// \return Endpoint, or a default-constructed value when out of range.
    ///
    EndpointInfo endpointAt(int row) const;

    ///
    /// \brief Returns the current endpoints in ranked order.
    /// \return The sorted endpoint list.
    ///
    const QList<EndpointInfo> &endpoints() const;

private:
    bool isSecure(const EndpointInfo &endpoint) const;
    int rankScore(const EndpointInfo &endpoint) const;
    int recommendedRow() const;

    QList<EndpointInfo> _endpoints;
    int _recommendedRow = -1;
};
