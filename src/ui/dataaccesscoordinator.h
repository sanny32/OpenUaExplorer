// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dataaccesscoordinator.h
/// \brief Declares the coordinator of the central data-access, events, history and trend area.
///

#pragma once

#include <QHash>
#include <QObject>
#include <QPair>
#include <QSet>
#include <QVector>

#include "dataaccessmonitoringstate.h"
#include "models/subscriptionitem.h"
#include "opcua/opcuatypes.h"
#include "session/sessiondata.h"
#include "widgets/dataview.h"

class AddressSpaceModule;
class AppSettings;
class AttributeModule;
class DataAccessModule;
class EventsModule;
class OpcUaBackend;
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
    /// \param addressSpace Address-space module used to browse dropped folders.
    /// \param selection Selection mediator shared with the UI features.
    /// \param backend Backend queried for the connection state.
    /// \param actions Menu and toolbar actions steered by the coordinator.
    /// \param dialogParent Parent widget for dialogs; also the QObject owner.
    ///
    DataAccessCoordinator(DataView *dataView,
                          TrendPanelWidget *trendPanel,
                          DataAccessModule *dataAccess,
                          EventsModule *events,
                          AttributeModule *attributes,
                          AddressSpaceModule *addressSpace,
                          SelectionContext *selection,
                          OpcUaBackend *backend,
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
    /// \brief Opens a data-view page and switches to it, or closes its tab.
    /// \param page Page to open or close.
    /// \param visible True to open the tab and make it current.
    ///
    void setPageVisible(DataView::Page page, bool visible);

    ///
    /// \brief Reopens every data-view tab the user closed.
    ///
    void showAllPages();

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
    /// \brief Persists the open pages and the view element state of the central area.
    /// \param settings Settings store to write to.
    ///
    void saveState(AppSettings &settings) const;

    ///
    /// \brief Restores the open pages and the view element state of the central area.
    /// \param settings Settings store to read from.
    ///
    void restoreState(AppSettings &settings);

    ///
    /// \brief Clears the runtime data and monitoring state after a disconnect.
    ///
    void clearRuntimeState();

    ///
    /// \brief Keeps the collected data on screen but drops the state tied to the connection.
    /// \param offline True while the server connection is gone.
    ///
    void setOffline(bool offline);

    ///
    /// \brief Exports the currently visible central-area view to a file.
    ///
    void exportActiveView();

    ///
    /// \brief Returns the user-created subscriptions for a saved session.
    /// \return Non-built-in subscriptions in row order.
    ///
    QVector<SubscriptionItem> sessionSubscriptions() const;

    ///
    /// \brief Returns the listed data-access nodes with their session state.
    /// \return Saved-node records in row order.
    ///
    QVector<SessionNode> monitoredNodes() const;

    ///
    /// \brief Returns the charted trend node ids for a saved session.
    /// \return Charted node ids.
    ///
    QStringList trendNodes() const;

    ///
    /// \brief Returns the trend chart tabs with their settings for a saved session.
    /// \return Trend tabs in tab order.
    ///
    QVector<SessionTrendTab> trendTabs() const;

    ///
    /// \brief Restores monitored data-access nodes from a loaded session.
    /// \param nodes Saved nodes to re-add and monitor.
    ///
    void restoreMonitoredNodes(const QVector<SessionNode> &nodes);

    ///
    /// \brief Restores charted trend nodes from a loaded session.
    /// \param nodeIds Node ids to chart.
    ///
    void restoreTrendNodes(const QStringList &nodeIds);

    ///
    /// \brief Restores trend chart tabs and their settings from a loaded session.
    /// \param tabs Trend tabs to recreate.
    ///
    void restoreTrendTabs(const QVector<SessionTrendTab> &tabs);

    ///
    /// \brief Recreates user subscriptions from a loaded session.
    /// \param subscriptions Subscriptions to add if absent.
    ///
    void restoreSubscriptions(const QVector<SubscriptionItem> &subscriptions);

private:
    SubscriptionItem subscriptionByName(const QString &name) const;
    void onAttributeDetailsReady(const OpcUaNodeDetails &details, const QString &error);
    void onDetailsReady(const OpcUaNodeDetails &details, const QString &error);
    void onSelectionCleared();
    void onDataValuesReady(const QVector<OpcUaDataValue> &values, const QString &error);
    void onHistoryReady(const QString &nodeId, const QVector<OpcUaHistoryValue> &values,
                        const QString &error);
    void onWriteFinished(const QString &nodeId, bool success, const QString &error);
    void onMonitoringFinished(const QString &nodeId, bool subscribed,
                              bool success, const QString &error);
    void onMonitoringIntervalRevised(const QString &nodeId, double publishingInterval);
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
    void addFolderById(const QString &nodeId);
    void onFolderChildrenReady(const QString &parentNodeId,
                               const QVector<OpcUaNodeInfo> &children,
                               const QString &error);
    void onFolderSubtreeVariablesReady(const QString &rootNodeId,
                                       const QVector<OpcUaNodeInfo> &variables,
                                       const QString &error);
    void offerFolderVariables(const QVector<OpcUaNodeInfo> &nodes, const QString &error);
    void addFolderVariables(const QVector<OpcUaNodeInfo> &variables);
    void finishFolderNode(const QString &nodeId, bool success);
    void showWriteDialog(const QString &nodeId, const QVariant &value, int valueType,
                         const QString &dataTypeId, bool writable,
                         const OpcUaEnumEntries &enumEntries);
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
    AddressSpaceModule *_addressSpace;
    SelectionContext *_selection;
    OpcUaBackend *_backend;
    DataAccessActions _actions;
    QWidget *_dialogParent;
    OpcUaNodeDetails _selectedNodeDetails;
    DataAccessMonitoringState _monitoringState;
    QSet<QString> _pendingDataAccessNodeIds;
    QHash<QString, QString> _pendingRestoreSubscriptions;
    QSet<QString> _pendingFolderDropNodeIds;
    QSet<QString> _folderAddNodeIds;
    int _folderAddFailureCount = 0;
};
