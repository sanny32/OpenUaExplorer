// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QList>
#include <QStringList>
#include <QVector>

#include <QOpcUaEndpointDescription>
#include <QOpcUaHistoryData>
#include <QOpcUaHistoryEvent>
#include <QOpcUaMonitoringParameters>
#include <QOpcUaReadResult>

#include "opcuatypes.h"

namespace QtOpcUaResultMapper {

/// \brief Keeps only endpoints whose security policy and transport scheme can be used.
QVector<QOpcUaEndpointDescription> endpointsWithSupportedPolicy(
    const QVector<QOpcUaEndpointDescription> &endpoints,
    const QStringList &supportedPolicies,
    const QString &scheme = QString());

/// \brief Fills browsed children with EventNotifier/Historizing values from a batch read.
void applyBrowseAttributeResults(QVector<OpcUaNodeInfo> *nodes,
                                 const QVector<QOpcUaReadResult> &results);

/// \brief Maps Qt attribute read results into transport-neutral data values.
QVector<OpcUaDataValue> dataValues(const QVector<QOpcUaReadResult> &results);

/// \brief Maps a Qt history result into transport-neutral history samples.
QVector<OpcUaHistoryValue> historyValues(const QOpcUaHistoryData &history);

/// \brief Builds the BaseEventType select clause shared by event reads and subscriptions.
QOpcUaMonitoringParameters::EventFilter baseEventFilter();

/// \brief Builds an OpcUaEvent from the select-clause field values of one notification.
OpcUaEvent eventFromFields(const QString &nodeId, const QVariantList &fields);

/// \brief Maps a Qt event-history result into transport-neutral events.
QVector<OpcUaEvent> historyEvents(const QOpcUaHistoryEvent &history);

} // namespace QtOpcUaResultMapper
