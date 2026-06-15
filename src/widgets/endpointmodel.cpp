// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#include "endpointmodel.h"

namespace {
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
}

EndpointModel::EndpointModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int EndpointModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : _endpoints.size();
}

QVariant EndpointModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= _endpoints.size())
        return {};
    const EndpointInfo &endpoint = _endpoints.at(index.row());
    const QString policy = endpoint.securityPolicy.section(QLatin1Char('#'), -1);
    switch (role) {
    case Qt::DisplayRole:
    case PolicyRole:
        return policy;
    case Qt::ToolTipRole:
        return endpoint.endpointUrl;
    case ModeRole:
        return endpoint.securityMode;
    case IconRole:
        return securityIconName(endpoint.securityModeValue);
    case EndpointRole:
        return QVariant::fromValue(endpoint);
    default:
        return {};
    }
}

QHash<int, QByteArray> EndpointModel::roleNames() const
{
    auto roles = QAbstractListModel::roleNames();
    roles.insert(PolicyRole, "policy");
    roles.insert(ModeRole, "mode");
    roles.insert(IconRole, "icon");
    roles.insert(EndpointRole, "endpoint");
    return roles;
}

void EndpointModel::setEndpoints(const QList<EndpointInfo> &endpoints)
{
    beginResetModel();
    _endpoints = endpoints;
    endResetModel();
}

void EndpointModel::clear()
{
    setEndpoints({});
}

EndpointInfo EndpointModel::endpointAt(int row) const
{
    return row >= 0 && row < _endpoints.size()
        ? _endpoints.at(row)
        : EndpointInfo{};
}

const QList<EndpointInfo> &EndpointModel::endpoints() const
{
    return _endpoints;
}
