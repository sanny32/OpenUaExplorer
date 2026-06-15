// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QAbstractListModel>

#include "opcua/opcuatypes.h"

class EndpointModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        PolicyRole = Qt::UserRole,
        ModeRole,
        IconRole,
        EndpointRole
    };

    explicit EndpointModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setEndpoints(const QList<EndpointInfo> &endpoints);
    void clear();
    EndpointInfo endpointAt(int row) const;
    const QList<EndpointInfo> &endpoints() const;

private:
    QList<EndpointInfo> _endpoints;
};
