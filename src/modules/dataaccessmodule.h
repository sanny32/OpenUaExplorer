// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dataaccessmodule.h
/// \brief Declares the module that exposes the data-access and monitoring API.
///

#pragma once

#include <QDateTime>
#include <QStringList>

#include "opcua/opcuatypes.h"
#include "servicemodule.h"

class OpcUaClientService;

///
/// \brief Provides the data-access API: read values and manage monitoring.
///
/// subscribe() is the single logged choke point for monitoring; callers (the data-access
/// widget and the MainWindow toolbar) invoke it directly. Streamed value updates that
/// arrive through valuesReady() are intentionally not logged.
///
class DataAccessModule : public ServiceModule
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs the data access module.
    /// \param parent Owning QObject.
    ///
    explicit DataAccessModule(QObject *parent = nullptr);

    QString name() const override;
    const QLoggingCategory &logCategory() const override;
    void initialize(ServiceContext &context) override;

public slots:
    ///
    /// \brief Reads the values of several nodes.
    /// \param nodeIds Nodes to read.
    ///
    void read(const QStringList &nodeIds);

    ///
    /// \brief Reads the raw history of a single node's Value over a time range.
    /// \param nodeId Node whose history is read.
    /// \param start Inclusive range start.
    /// \param end Inclusive range end.
    /// \param maxValues Maximum samples to return, or 0 for no limit.
    ///
    void readHistory(const QString &nodeId, const QDateTime &start, const QDateTime &end,
                     quint32 maxValues);

    ///
    /// \brief Enables monitoring for a node.
    /// \param nodeId Node to monitor.
    /// \param publishingInterval Publishing interval in milliseconds.
    ///
    void subscribe(const QString &nodeId, double publishingInterval = 1000.0);

    ///
    /// \brief Disables monitoring for a node.
    /// \param nodeId Node to stop monitoring.
    ///
    void unsubscribe(const QString &nodeId);

signals:
    ///
    /// \brief Emitted when read or monitored values arrive.
    /// \param values Latest values.
    /// \param error Read error, empty on success.
    ///
    void valuesReady(QVector<OpcUaDataValue> values, QString error);

    ///
    /// \brief Emitted when a raw history read finishes.
    /// \param nodeId Node whose history was read.
    /// \param values History samples in time order.
    /// \param error Read error, empty on success.
    ///
    void historyReady(QString nodeId, QVector<OpcUaHistoryValue> values, QString error);

    ///
    /// \brief Emitted when a monitoring request finishes.
    /// \param nodeId Affected node.
    /// \param subscribed True for subscribe and false for unsubscribe.
    /// \param success Whether the request succeeded.
    /// \param error Error description, empty on success.
    ///
    void monitoringFinished(QString nodeId, bool subscribed, bool success, QString error);

private:
    void handleValuesReady(const QVector<OpcUaDataValue> &values, const QString &error);
    void handleHistoryReady(const QString &nodeId, const QVector<OpcUaHistoryValue> &values,
                            const QString &error);
    void handleMonitoringFinished(const QString &nodeId, bool subscribed,
                                  bool success, const QString &error);

    OpcUaClientService *_clientService = nullptr;
};
