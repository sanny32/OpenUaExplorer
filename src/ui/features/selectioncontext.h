// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file selectioncontext.h
/// \brief Declares the selected-node mediator shared by UI features.
///

#pragma once

#include <QObject>

#include "opcua/opcuatypes.h"

///
/// \brief Stores the current node selection and publishes matching node details.
///
class SelectionContext : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs an empty selection context.
    /// \param parent Owning QObject.
    ///
    explicit SelectionContext(QObject *parent = nullptr);

    ///
    /// \brief Returns the currently selected address-space node.
    /// \return Selected node, or an empty node when nothing is selected.
    ///
    OpcUaNodeInfo currentNode() const;

    ///
    /// \brief Returns the details for the selected node.
    /// \return Selected node details, or an empty value when none are loaded.
    ///
    OpcUaNodeDetails currentDetails() const;

public slots:
    ///
    /// \brief Selects a node and requests that listeners refresh their state.
    /// \param node Node selected by a feature.
    ///
    void selectNode(const OpcUaNodeInfo &node);

    ///
    /// \brief Publishes node details when they belong to the current selection.
    /// \param details Read node details.
    /// \param error Read error, empty on success.
    ///
    void setDetails(const OpcUaNodeDetails &details, const QString &error);

    ///
    /// \brief Clears the selected node and its details.
    ///
    void clear();

    ///
    /// \brief Requests a history read for a node selected from a feature.
    /// \param node Node whose history should be read.
    ///
    void requestHistory(const OpcUaNodeInfo &node);

    ///
    /// \brief Requests event monitoring for a node selected from a feature.
    /// \param node Node to monitor for events.
    ///
    void requestEventMonitor(const OpcUaNodeInfo &node);

    ///
    /// \brief Requests an event history read for a node selected from a feature.
    /// \param node Node whose event history should be read.
    ///
    void requestEventsHistory(const OpcUaNodeInfo &node);

    ///
    /// \brief Requests charting a node selected from a feature.
    /// \param node Variable node to chart.
    ///
    void requestAddToTrend(const OpcUaNodeInfo &node);

    ///
    /// \brief Requests monitoring a variable node selected from a feature.
    /// \param node Variable node to subscribe.
    ///
    void requestSubscribe(const OpcUaNodeInfo &node);

    ///
    /// \brief Requests stopping monitoring of a variable node selected from a feature.
    /// \param node Variable node to unsubscribe.
    ///
    void requestUnsubscribe(const OpcUaNodeInfo &node);

signals:
    ///
    /// \brief Emitted when a node becomes the current selection.
    /// \param node Selected node.
    ///
    void nodeSelected(OpcUaNodeInfo node);

    ///
    /// \brief Emitted when details for the selected node are available.
    /// \param details Selected node details.
    /// \param error Read error, empty on success.
    ///
    void detailsReady(OpcUaNodeDetails details, QString error);

    ///
    /// \brief Emitted after the selection has been cleared.
    ///
    void cleared();

    ///
    /// \brief Emitted when a feature requests a history read for a node.
    /// \param node Node whose history should be read.
    ///
    void historyReadRequested(OpcUaNodeInfo node);

    ///
    /// \brief Emitted when a feature requests event monitoring for a node.
    /// \param node Node to monitor for events.
    ///
    void eventMonitorRequested(OpcUaNodeInfo node);

    ///
    /// \brief Emitted when a feature requests an event history read for a node.
    /// \param node Node whose event history should be read.
    ///
    void eventsHistoryReadRequested(OpcUaNodeInfo node);

    ///
    /// \brief Emitted when a feature requests charting a node.
    /// \param node Variable node to chart.
    ///
    void addToTrendRequested(OpcUaNodeInfo node);

    ///
    /// \brief Emitted when a feature requests monitoring a variable node.
    /// \param node Variable node to subscribe.
    ///
    void subscribeRequested(OpcUaNodeInfo node);

    ///
    /// \brief Emitted when a feature requests stopping monitoring of a variable node.
    /// \param node Variable node to unsubscribe.
    ///
    void unsubscribeRequested(OpcUaNodeInfo node);

private:
    OpcUaNodeInfo _currentNode;
    OpcUaNodeDetails _currentDetails;
    QString _pendingHistoryNodeId;
    QString _pendingEventNodeId;
    QString _pendingEventsHistoryNodeId;
};
