// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dataaccesscoordinator.h
/// \brief Declares the coordinator of the central data-access, events, history and trend area.
///

#pragma once

#include <QObject>
#include <QSet>

#include "models/subscriptionitem.h"
#include "opcua/opcuatypes.h"

class AppSettings;
class AttributeModule;
class DataAccessModule;
class DataView;
class EventsModule;
class OpcUaClientService;
class QAction;
class SelectionContext;
class TrendPanelWidget;

///
/// \brief Menu and toolbar actions steered by the data-access coordinator.
///
struct DataAccessActions
{
    QAction *read = nullptr;
    QAction *readSelected = nullptr;
    QAction *write = nullptr;
    QAction *writeValue = nullptr;
    QAction *subscribe = nullptr;
    QAction *unsubscribe = nullptr;
    QAction *addToDataAccess = nullptr;
    QAction *removeFromDataAccess = nullptr;
    QAction *clearDataAccess = nullptr;
    QAction *setSubscriptionNone = nullptr;
    QAction *setSubscriptionDefault = nullptr;
    QAction *setSubscriptionFast = nullptr;
    QAction *setSubscriptionCustom = nullptr;
    QAction *readDataHistory = nullptr;
    QAction *readEventsHistory = nullptr;
};

///
/// \brief Coordinates the central-widget monitoring area with the data modules.
///
/// The data-access area is a central-widget monitoring controller, not a dock feature:
/// this class owns its module wiring, monitoring state, action enabling and persistence,
/// so the coupling stays discoverable in one place.
///
class DataAccessCoordinator : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the coordinator and wires the central area to the data modules.
    /// \param dataView Tabbed data view in the central widget.
    /// \param trendPanel Trend panel in the central widget.
    /// \param dataAccess Data-access module used for reads and monitoring.
    /// \param events Events module used for event monitoring and history.
    /// \param attributes Attribute module used for node reads and writes.
    /// \param selection Selection mediator shared with the UI features.
    /// \param clientService Client service queried for the connection state.
    /// \param actions Menu and toolbar actions steered by the coordinator.
    /// \param dialogParent Parent widget for dialogs; also the QObject owner.
    ///
    DataAccessCoordinator(DataView *dataView,
                          TrendPanelWidget *trendPanel,
                          DataAccessModule *dataAccess,
                          EventsModule *events,
                          AttributeModule *attributes,
                          SelectionContext *selection,
                          OpcUaClientService *clientService,
                          const DataAccessActions &actions,
                          QWidget *dialogParent);

    ///
    /// \brief Reads the currently selected node.
    ///
    void readSelected();

    ///
    /// \brief Opens the write dialog for the selected node's current value.
    ///
    void writeSelected();

    ///
    /// \brief Starts monitoring the selected variable and adds it to Data Access.
    ///
    void subscribeSelected();

    ///
    /// \brief Stops monitoring the selected variable.
    ///
    void unsubscribeSelected();

    ///
    /// \brief Adds the selected variable node to the data-access view.
    ///
    void addSelectedToView();

    ///
    /// \brief Removes the selected data-access nodes from the data-access view.
    ///
    void removeSelectionFromView();

    ///
    /// \brief Removes every node from the data-access view.
    ///
    void clearView();

    ///
    /// \brief Unsubscribes the selected data-access nodes, leaving them in the view.
    ///
    void applyNoSubscription();

    ///
    /// \brief Assigns the selected data-access nodes to the built-in Default subscription.
    ///
    void applyDefaultSubscription();

    ///
    /// \brief Assigns the selected data-access nodes to the built-in Fast subscription.
    ///
    void applyFastSubscription();

    ///
    /// \brief Creates a new subscription and assigns the selected data-access nodes to it.
    ///
    void promptCustomSubscription();

    ///
    /// \brief Reads the data history of the selected variable node.
    ///
    void readDataHistoryForSelected();

    ///
    /// \brief Reads the event history of the selected node.
    ///
    void readEventsHistoryForSelected();

    ///
    /// \brief Switches the data view to the Data Access page.
    ///
    void showDataAccessPage();

    ///
    /// \brief Switches the data view to the Events page.
    ///
    void showEventsPage();

    ///
    /// \brief Switches the data view to the Data History page when history is supported.
    ///
    void showDataHistoryPage();

    ///
    /// \brief Switches the data view to the Events History page when history is supported.
    ///
    void showEventsHistoryPage();

    ///
    /// \brief Shows the subscriptions management dialog.
    ///
    void showSubscriptionsDialog();

    ///
    /// \brief Loads the saved subscriptions into the subscriptions widget.
    /// \param settings Settings store to read from.
    ///
    void loadSubscriptions(AppSettings &settings);

    ///
    /// \brief Persists the visible page and the view element state of the central area.
    /// \param settings Settings store to write to.
    ///
    void saveState(AppSettings &settings) const;

    ///
    /// \brief Restores the visible page and the view element state of the central area.
    /// \param settings Settings store to read from.
    ///
    void restoreState(AppSettings &settings);

    ///
    /// \brief Clears the runtime data and monitoring state after a disconnect.
    ///
    void clearRuntimeState();

private:
    void onAttributeDetailsReady(const OpcUaNodeDetails &details, const QString &error);
    void onDetailsReady(const OpcUaNodeDetails &details, const QString &error);
    void onSelectionCleared();
    void onDataValuesReady(const QVector<OpcUaDataValue> &values, const QString &error);
    void onHistoryReady(const QString &nodeId, const QVector<OpcUaHistoryValue> &values,
                        const QString &error);
    void onWriteFinished(const QString &nodeId, bool success, const QString &error);
    void onMonitoringFinished(const QString &nodeId, bool subscribed,
                              bool success, const QString &error);
    void onEventsReady(const QString &nodeId, const QVector<OpcUaEvent> &events,
                       const QString &error);
    void onEventsHistoryReady(const QString &nodeId, const QVector<OpcUaEvent> &events,
                              const QString &error);
    void onEventMonitoringFinished(const QString &nodeId, bool subscribed,
                                   bool success, const QString &error);
    void onClientStateChanged(OpcUaConnectionState state);
    void onNodeCountChanged(int count);
    void onHistoryReadRequested(const OpcUaNodeInfo &node);
    void onEventsHistoryReadRequested(const OpcUaNodeInfo &node);
    void onEventMonitorRequested(const OpcUaNodeInfo &node);
    void onAddToTrendRequested(const OpcUaNodeInfo &node);
    void onSubscribeRequested(const OpcUaNodeInfo &node);
    void onUnsubscribeRequested(const OpcUaNodeInfo &node);
    void addNodeById(const QString &nodeId);
    void showWriteDialog(const QString &nodeId, const QVariant &value, int valueType,
                         const QString &dataTypeId, bool writable);
    void updateMonitoringActions();
    void updateSelectionActions();
    SubscriptionItem builtinSubscription(bool fast) const;
    void wireDataView();
    void wireSelectionContext();
    void wireModules();

    DataView *_dataView;
    TrendPanelWidget *_trendPanel;
    DataAccessModule *_dataAccess;
    EventsModule *_events;
    AttributeModule *_attributes;
    SelectionContext *_selection;
    OpcUaClientService *_clientService;
    DataAccessActions _actions;
    QWidget *_dialogParent;
    OpcUaNodeDetails _selectedNodeDetails;
    QSet<QString> _subscribedNodeIds;
    QSet<QString> _pendingMonitoringNodeIds;
    QSet<QString> _pendingDataAccessNodeIds;
};
