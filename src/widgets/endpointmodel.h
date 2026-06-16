// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QAbstractTableModel>
#include <QColor>

#include "opcua/opcuatypes.h"

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

    enum Role {
        PolicyRole = Qt::UserRole,
        ModeRole,
        IconRole,
        StatusRole,
        StatusColorRole,
        RecommendedRole,
        EndpointRole
    };

    explicit EndpointModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setEndpoints(const QList<EndpointInfo> &endpoints);
    void clear();
    EndpointInfo endpointAt(int row) const;
    const QList<EndpointInfo> &endpoints() const;

private:
    bool isSecure(const EndpointInfo &endpoint) const;
    int recommendedRow() const;

    QList<EndpointInfo> _endpoints;
    int _recommendedRow = -1;
};
