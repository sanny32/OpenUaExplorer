// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dataaccesscoordinator.cpp
/// \brief Implements the coordinator of the central data-access, events, history and trend area.
///

#include <QAction>
#include <QGuiApplication>

#include "addressspacemodule.h"
#include "application.h"
#include "appsettings.h"
#include "attributemodule.h"
#include "dataaccesscoordinator.h"
#include "dataaccessmodule.h"
#include "dialogs/messageboxdialog.h"
#include "dialogs/writevaluedialog.h"
#include "eventsmodule.h"
#include "features/selectioncontext.h"
#include "models/trendseries.h"
#include "opcua/opcuabackend.h"
#include "widgets/dataaccesswidget.h"
#include "widgets/datahistorywidget.h"
#include "widgets/dataview.h"
#include "widgets/eventshistorywidget.h"
#include "widgets/eventswidget.h"
#include "widgets/subscriptionswidget.h"
#include "widgets/trendpanelwidget.h"

namespace {

///
/// \brief Number of variables a dropped folder may add without asking.
///
constexpr int kFolderDropSilentLimit = 10;

///
/// \brief Hard cap on the variables one dropped folder may add.
///
constexpr int kFolderDropMaxNodes = 100;

} // namespace

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
DataAccessCoordinator::DataAccessCoordinator(DataView *dataView,
                                             TrendPanelWidget *trendPanel,
                                             DataAccessModule *dataAccess,
                                             EventsModule *events,
                                             AttributeModule *attributes,
                                             AddressSpaceModule *addressSpace,
                                             SelectionContext *selection,
                                             OpcUaBackend *backend,
                                             const DataAccessActions &actions,
                                             QWidget *dialogParent)
    : QObject(dialogParent)
    , _dataView(dataView)
    , _trendPanel(trendPanel)
    , _dataAccess(dataAccess)
    , _events(events)
    , _attributes(attributes)
    , _addressSpace(addressSpace)
    , _selection(selection)
    , _backend(backend)
    , _actions(actions)
    , _dialogParent(dialogParent)
{
    wireDataView();
    wireSelectionContext();
    wireModules();
    connect(_backend, &OpcUaBackend::stateChanged,
            this, &DataAccessCoordinator::onClientStateChanged);
}

///
/// \brief Removes any application cursor override owned by the coordinator.
///
DataAccessCoordinator::~DataAccessCoordinator()
{
    _pendingFolderDropNodeIds.clear();
    endFolderDropWait();
}

///
/// \brief Reads the currently selected node.
///
void DataAccessCoordinator::readSelected()
{
    const OpcUaNodeInfo selected = _selection->currentNode();
    if (!selected.nodeId.isEmpty())
        _attributes->read(selected.nodeId);
}

///
/// \brief Opens the write dialog for the selected node's current value.
///
void DataAccessCoordinator::writeSelected()
{
    if (_selectedNodeDetails.nodeId.isEmpty())
        return;
    showWriteDialog(_selectedNodeDetails.nodeId, _selectedNodeDetails.value,
                    _selectedNodeDetails.valueType, _selectedNodeDetails.dataTypeId,
                    OpcUa::isWritable(_selectedNodeDetails.userAccessLevel),
                    _selectedNodeDetails.enumEntries);
}

///
/// \brief Starts monitoring the selected variable and adds it to Data Access.
///
void DataAccessCoordinator::subscribeSelected()
{
    if (!OpcUa::isVariable(_selectedNodeDetails.nodeClass)
        || _selectedNodeDetails.nodeId.isEmpty()) {
        return;
    }
    _dataView->addNode(_selectedNodeDetails);
    _monitoringState.beginRequest(_selectedNodeDetails.nodeId);
    updateMonitoringActions();
    _dataAccess->subscribe(_selectedNodeDetails.nodeId);
}

///
/// \brief Stops monitoring the selected variable.
///
void DataAccessCoordinator::unsubscribeSelected()
{
    if (_selectedNodeDetails.nodeId.isEmpty()
        || !_monitoringState.isSubscribed(_selectedNodeDetails.nodeId)) {
        return;
    }
    _monitoringState.beginRequest(_selectedNodeDetails.nodeId);
    updateMonitoringActions();
    _dataAccess->unsubscribe(_selectedNodeDetails.nodeId);
}

///
/// \brief Adds the selected variable node to the data-access view.
///
void DataAccessCoordinator::addSelectedToView()
{
    if (OpcUa::isVariable(_selectedNodeDetails.nodeClass))
        _dataView->addNode(_selectedNodeDetails);
}

///
/// \brief Removes the selected data-access nodes from the data-access view.
///
void DataAccessCoordinator::removeSelectionFromView()
{
    _dataView->dataAccess()->removeSelectedNodes();
}

///
/// \brief Removes every node from the data-access view.
///
void DataAccessCoordinator::clearView()
{
    _dataView->clearDataAccessNodes();
}

///
/// \brief Unsubscribes the selected data-access nodes, leaving them in the view.
///
void DataAccessCoordinator::applyNoSubscription()
{
    _dataView->dataAccess()->applySubscriptionToSelection(QString());
}

///
/// \brief Assigns the selected data-access nodes to the built-in Default subscription.
///
void DataAccessCoordinator::applyDefaultSubscription()
{
    _dataView->dataAccess()->applySubscriptionToSelection(builtinSubscription(false).name);
}

///
/// \brief Assigns the selected data-access nodes to the built-in Fast subscription.
///
void DataAccessCoordinator::applyFastSubscription()
{
    _dataView->dataAccess()->applySubscriptionToSelection(builtinSubscription(true).name);
}

///
/// \brief Creates a new subscription and assigns the selected data-access nodes to it.
///
void DataAccessCoordinator::promptCustomSubscription()
{
    _dataView->dataAccess()->promptSubscriptionForSelection();
}

///
/// \brief Reads the data history of the selected variable node.
///
void DataAccessCoordinator::readDataHistoryForSelected()
{
    if (OpcUa::canReadHistory(_selectedNodeDetails))
        _dataView->requestDataHistoryForNode(
            _selectedNodeDetails.nodeId,
            _selectedNodeDetails.displayName,
            _selection->currentNode().displayPath);
}

///
/// \brief Reads the event history of the selected node.
///
void DataAccessCoordinator::readEventsHistoryForSelected()
{
    if (OpcUa::canReadEventHistory(_selectedNodeDetails))
        _dataView->requestEventsHistoryForNode(
            _selectedNodeDetails.nodeId,
            _selectedNodeDetails.displayName,
            _selection->currentNode().displayPath);
}

///
/// \brief Opens a data-view page and switches to it, or closes its tab.
/// \param page Page to open or close.
/// \param visible True to open the tab and make it current.
///
void DataAccessCoordinator::setPageVisible(DataView::Page page, bool visible)
{
    _dataView->setPageVisible(page, visible);
    if (visible && _dataView->isPageVisible(page))
        _dataView->setCurrentPage(page);
}

///
/// \brief Reopens every data-view tab the user closed.
///
void DataAccessCoordinator::showAllPages()
{
    _dataView->setClosedPages({});
}

///
/// \brief Shows the subscriptions management dialog.
///
void DataAccessCoordinator::showSubscriptionsDialog()
{
    _dataView->showSubscriptionsDialog();
}

///
/// \brief Loads the saved subscriptions into the subscriptions widget.
/// \param settings Settings store to read from.
///
void DataAccessCoordinator::loadSubscriptions(AppSettings &settings)
{
    _dataView->subscriptions()->loadSubscriptions(settings);
}

///
/// \brief Persists the open pages and the view element state of the central area.
/// \param settings Settings store to write to.
///
void DataAccessCoordinator::saveState(AppSettings &settings) const
{
    settings.setDataAccessPage(_dataView->currentPage());
    settings.setClosedDataAccessPages(_dataView->closedPages());
    _dataView->saveViewState(settings);
    _dataView->subscriptions()->saveSubscriptions(settings);
    _trendPanel->saveViewState(settings);
}

///
/// \brief Restores the open pages and the view element state of the central area.
/// \param settings Settings store to read from.
///
void DataAccessCoordinator::restoreState(AppSettings &settings)
{
    _dataView->setClosedPages(settings.closedDataAccessPages());
    // setCurrentPage() reopens a closed tab, so only activate a page that stayed open.
    const auto page = static_cast<DataView::Page>(settings.dataAccessPage());
    if (_dataView->isPageVisible(page))
        _dataView->setCurrentPage(page);
    _dataView->restoreViewState(settings);
    _trendPanel->restoreViewState(settings);
}

///
/// \brief Clears the runtime data and monitoring state after a disconnect.
///
void DataAccessCoordinator::clearRuntimeState()
{
    _dataView->clearRuntimeData();
    _trendPanel->clearRuntimeData();
    _monitoringState.clear();
    _pendingDataAccessNodeIds.clear();
    _pendingRestoreSubscriptions.clear();
    _pendingFolderDropNodeIds.clear();
    endFolderDropWait();
    _folderAddNodeIds.clear();
    _folderAddFailureCount = 0;
    updateMonitoringActions();
}

///
/// \brief Keeps the collected data on screen but drops the state tied to the connection.
///
/// The rows, charts and history stay so the user keeps the context of the lost session;
/// monitoring bookkeeping is reset because the server no longer holds any subscription.
/// \param offline True while the server connection is gone.
///
void DataAccessCoordinator::setOffline(bool offline)
{
    _dataView->setOffline(offline);
    if (!offline)
        return;

    _monitoringState.clear();
    _pendingDataAccessNodeIds.clear();
    _pendingRestoreSubscriptions.clear();
    _pendingFolderDropNodeIds.clear();
    endFolderDropWait();
    _folderAddNodeIds.clear();
    _folderAddFailureCount = 0;
    updateMonitoringActions();
}

///
/// \brief Exports the currently visible central-area view to a file.
///
void DataAccessCoordinator::exportActiveView()
{
    switch (_dataView->currentPage()) {
    case DataView::EventsPage:
        _dataView->events()->exportEventsToCsv();
        break;
    case DataView::DataHistoryPage:
        _dataView->dataHistory()->exportDataHistoryToCsv();
        break;
    case DataView::EventsHistoryPage:
        _dataView->eventsHistory()->exportEventsHistoryToCsv();
        break;
    case DataView::DataAccessPage:
    default:
        _dataView->dataAccess()->exportToCsv();
        break;
    }
}

///
/// \brief Returns the user-created subscriptions for a saved session.
/// \return Non-built-in subscriptions in row order.
///
QVector<SubscriptionItem> DataAccessCoordinator::sessionSubscriptions() const
{
    QVector<SubscriptionItem> result;
    const QVector<SubscriptionItem> items = _dataView->subscriptions()->subscriptions();
    for (const SubscriptionItem &item : items) {
        if (!item.isBuiltin())
            result.append(item);
    }
    return result;
}

///
/// \brief Returns the listed data-access nodes with their session state.
/// \return Saved-node records in row order.
///
QVector<SessionNode> DataAccessCoordinator::monitoredNodes() const
{
    return _dataView->dataAccess()->monitoredNodes();
}

///
/// \brief Returns the charted trend node ids for a saved session.
/// \return Charted node ids.
///
QStringList DataAccessCoordinator::trendNodes() const
{
    return _trendPanel->chartedNodeIds();
}

///
/// \brief Returns the trend chart tabs with their settings for a saved session.
/// \return Trend tabs in tab order.
///
QVector<SessionTrendTab> DataAccessCoordinator::trendTabs() const
{
    return _trendPanel->captureTrendTabs();
}

///
/// \brief Restores monitored data-access nodes from a loaded session.
/// \param nodes Saved nodes to re-add and monitor.
///
void DataAccessCoordinator::restoreMonitoredNodes(const QVector<SessionNode> &nodes)
{
    DataAccessWidget *dataAccess = _dataView->dataAccess();
    dataAccess->restoreMonitoredNodes(nodes);
    for (const SessionNode &node : dataAccess->monitoredNodes()) {
        _pendingRestoreSubscriptions.insert(node.nodeId, node.subscriptionName);
        addNodeById(node.nodeId);
    }
}

///
/// \brief Restores charted trend nodes from a loaded session.
/// \param nodeIds Node ids to chart.
///
void DataAccessCoordinator::restoreTrendNodes(const QStringList &nodeIds)
{
    for (const QString &nodeId : nodeIds)
        _trendPanel->addNode(nodeId, QString());
}

///
/// \brief Restores trend chart tabs and their settings from a loaded session.
/// \param tabs Trend tabs to recreate.
///
void DataAccessCoordinator::restoreTrendTabs(const QVector<SessionTrendTab> &tabs)
{
    _trendPanel->restoreTrendTabs(tabs);
}

///
/// \brief Recreates user subscriptions from a loaded session.
/// \param subscriptions Subscriptions to add if absent.
///
void DataAccessCoordinator::restoreSubscriptions(const QVector<SubscriptionItem> &subscriptions)
{
    for (const SubscriptionItem &item : subscriptions)
        _dataView->subscriptions()->createSubscription(item.name, item.publishingInterval);
}

///
/// \brief Looks up a subscription by name, falling back to a plain named item.
/// \param name Subscription name.
/// \return Matching subscription, or a default-interval item carrying the name.
///
SubscriptionItem DataAccessCoordinator::subscriptionByName(const QString &name) const
{
    const QVector<SubscriptionItem> items = _dataView->subscriptions()->subscriptions();
    for (const SubscriptionItem &item : items) {
        if (item.name == name)
            return item;
    }
    SubscriptionItem fallback;
    fallback.name = name;
    return fallback;
}

///
/// \brief Applies raw attribute results that are not tied to the current selection.
/// \param details Read node details.
/// \param error Read error, if any.
///
void DataAccessCoordinator::onAttributeDetailsReady(const OpcUaNodeDetails &details,
                                                    const QString &error)
{
    const bool pending = _pendingDataAccessNodeIds.remove(details.nodeId);
    const bool isRestore = _pendingRestoreSubscriptions.contains(details.nodeId);
    const QString restoreSubscription = _pendingRestoreSubscriptions.take(details.nodeId);
    if (!error.isEmpty()) {
        finishFolderNode(details.nodeId, false);
        return;
    }

    if (!pending || !OpcUa::isVariable(details.nodeClass)) {
        if (pending)
            finishFolderNode(details.nodeId, false);
        return;
    }

    if (isRestore) {
        if (restoreSubscription.isEmpty()) {
            _dataView->addNode(details);
            finishFolderNode(details.nodeId, true);
        } else {
            _dataView->addNodeWithDefaultSubscription(details, subscriptionByName(restoreSubscription));
        }
        return;
    }

    _dataView->addNodeWithDefaultSubscription(details);
}

///
/// \brief Stores the read node details and enables the matching actions.
/// \param details Read node details.
/// \param error Read error, if any.
///
void DataAccessCoordinator::onDetailsReady(const OpcUaNodeDetails &details, const QString &error)
{
    if (!error.isEmpty())
        return;

    const bool variable = OpcUa::isVariable(details.nodeClass);
    _selectedNodeDetails = details;
    const bool writable = variable && OpcUa::isWritable(details.userAccessLevel);
    _actions.read->setEnabled(variable);
    _actions.readSelected->setEnabled(variable);
    _actions.write->setEnabled(writable);
    _actions.writeValue->setEnabled(writable);
    _actions.addToDataAccess->setEnabled(variable);
    _actions.readDataHistory->setEnabled(OpcUa::canReadHistory(details));
    _actions.readEventsHistory->setEnabled(OpcUa::canReadEventHistory(details));
    updateMonitoringActions();
}

///
/// \brief Clears selected-node state and disables selected-node actions.
///
void DataAccessCoordinator::onSelectionCleared()
{
    _selectedNodeDetails = {};
    _actions.read->setEnabled(false);
    _actions.readSelected->setEnabled(false);
    _actions.write->setEnabled(false);
    _actions.writeValue->setEnabled(false);
    _actions.addToDataAccess->setEnabled(false);
    _actions.readDataHistory->setEnabled(false);
    _actions.readEventsHistory->setEnabled(false);
    updateMonitoringActions();
}

///
/// \brief Pushes the latest data-access values into the view.
/// \param values Latest data access values.
/// \param error Read error, if any.
///
void DataAccessCoordinator::onDataValuesReady(const QVector<OpcUaDataValue> &values,
                                              const QString &error)
{
    if (error.isEmpty()) {
        _dataView->updateValues(values);
        _trendPanel->applyLiveValues(values);
    }
}

///
/// \brief Pushes raw data history samples into the view, or reports a read failure.
/// \param nodeId Node whose history was read.
/// \param values History samples in time order.
/// \param error Read error, if any.
///
void DataAccessCoordinator::onHistoryReady(const QString &nodeId,
                                           const QVector<OpcUaHistoryValue> &values,
                                           const QString &error)
{
    if (_trendPanel->consumeHistory(nodeId, error, values))
        return;
    if (error.isEmpty())
        _dataView->setDataHistoryResults(values);
    else
        MessageBoxDialog::warning(_dialogParent, tr("Data History Read Failed"), error,
                                  DialogButtonBox::Ok);
}

///
/// \brief Re-reads the node on success, or warns the user on failure.
/// \param nodeId Written node.
/// \param success Whether the write succeeded.
/// \param error Write error, if any.
///
void DataAccessCoordinator::onWriteFinished(const QString &nodeId, bool success,
                                            const QString &error)
{
    if (success) {
        _attributes->read(nodeId);
        _dataAccess->read({nodeId});
    } else {
        MessageBoxDialog::warning(_dialogParent, tr("Write Failed"), error,
                                  DialogButtonBox::Ok);
    }
}

///
/// \brief Applies the result of a subscribe or unsubscribe request to the UI.
/// \param nodeId Affected node.
/// \param subscribed True for subscribe and false for unsubscribe.
/// \param success Whether the request succeeded.
/// \param error Error description, empty on success.
///
void DataAccessCoordinator::onMonitoringFinished(const QString &nodeId, bool subscribed,
                                                 bool success, const QString &error)
{
    _monitoringState.finishRequest(nodeId, subscribed, success);
    const bool fromFolderDrop = _folderAddNodeIds.contains(nodeId);
    if (success) {
        _dataView->setNodeSubscribed(nodeId, subscribed);
        if (!subscribed)
            _dataView->setNodeRevisedInterval(nodeId, 0.0);
    } else if (!fromFolderDrop) {
        MessageBoxDialog::warning(_dialogParent,
                                  subscribed ? tr("Subscribe Failed") : tr("Unsubscribe Failed"),
                                  error,
                                  DialogButtonBox::Ok);
    }
    finishFolderNode(nodeId, success);
    updateMonitoringActions();
}

///
/// \brief Shows the monitoring parameters the server actually granted for a node.
/// \param nodeId Monitored node.
/// \param publishingInterval Publishing interval granted by the server, in milliseconds.
///
void DataAccessCoordinator::onMonitoringIntervalRevised(const QString &nodeId,
                                                        double publishingInterval)
{
    _dataView->setNodeRevisedInterval(nodeId, publishingInterval);
}

///
/// \brief Appends received events to the events view, ignoring delivery errors.
/// \param nodeId Monitored node that produced the events.
/// \param events Received events.
/// \param error Error description, empty on success.
///
void DataAccessCoordinator::onEventsReady(const QString &nodeId, const QVector<OpcUaEvent> &events,
                                          const QString &error)
{
    Q_UNUSED(nodeId)
    if (error.isEmpty())
        _dataView->appendEvents(events);
}

///
/// \brief Pushes historical events into the view, or reports a read failure.
/// \param nodeId Node whose event history was read.
/// \param events Historical events in server order.
/// \param error Read error, if any.
///
void DataAccessCoordinator::onEventsHistoryReady(const QString &nodeId,
                                                 const QVector<OpcUaEvent> &events,
                                                 const QString &error)
{
    Q_UNUSED(nodeId)
    if (error.isEmpty())
        _dataView->setEventsHistoryResults(events);
    else
        MessageBoxDialog::warning(_dialogParent, tr("Events History Read Failed"), error,
                                  DialogButtonBox::Ok);
}

///
/// \brief Applies the result of an event subscribe or unsubscribe request to the UI.
/// \param nodeId Affected node.
/// \param subscribed True for subscribe and false for unsubscribe.
/// \param success Whether the request succeeded.
/// \param error Error description, empty on success.
///
void DataAccessCoordinator::onEventMonitoringFinished(const QString &nodeId, bool subscribed,
                                                      bool success, const QString &error)
{
    if (success) {
        _dataView->events()->setEventMonitoringState(nodeId, subscribed);
    } else {
        MessageBoxDialog::warning(_dialogParent,
                                  subscribed ? tr("Event Subscribe Failed")
                                             : tr("Event Unsubscribe Failed"),
                                  error,
                                  DialogButtonBox::Ok);
    }
}

///
/// \brief Refreshes the action enable-states for the new connection state.
/// \param state Current OPC UA client state.
///
void DataAccessCoordinator::onClientStateChanged(OpcUaConnectionState state)
{
    Q_UNUSED(state)
    updateMonitoringActions();
    updateSelectionActions();
}

///
/// \brief Enables the data-access clear/remove actions for the current row count.
/// \param count Current number of data-access rows.
///
void DataAccessCoordinator::onNodeCountChanged(int count)
{
    _actions.clearDataAccess->setEnabled(count > 0);
    updateSelectionActions();
}

///
/// \brief Forwards a feature's history-read request to the data view.
/// \param node Node whose history should be read.
///
void DataAccessCoordinator::onHistoryReadRequested(const OpcUaNodeInfo &node)
{
    _dataView->requestDataHistoryForNode(node.nodeId, node.displayName, node.displayPath);
}

///
/// \brief Forwards a feature's event-history request to the data view.
/// \param node Node whose event history should be read.
///
void DataAccessCoordinator::onEventsHistoryReadRequested(const OpcUaNodeInfo &node)
{
    _dataView->requestEventsHistoryForNode(node.nodeId, node.displayName, node.displayPath);
}

///
/// \brief Forwards a feature's event-monitoring request to the data view.
/// \param node Node to monitor for events.
///
void DataAccessCoordinator::onEventMonitorRequested(const OpcUaNodeInfo &node)
{
    _dataView->requestEventMonitoringForNode(node.nodeId, node.displayName, node.displayPath);
}

///
/// \brief Adds a feature-selected node to the trend panel when it can be charted.
/// \param node Variable node to chart.
///
void DataAccessCoordinator::onAddToTrendRequested(const OpcUaNodeInfo &node)
{
    if (!TrendSeries::isTrendable(node))
        return;
    const QString name = node.displayName.isEmpty() ? node.browseName : node.displayName;
    _trendPanel->addNode(node.nodeId, name, node.displayPath);
}

///
/// \brief Starts monitoring a feature-selected node, or a folder's direct variables.
/// \param node Variable node to subscribe, or a container node to expand first.
///
void DataAccessCoordinator::onSubscribeRequested(const OpcUaNodeInfo &node)
{
    if (OpcUa::isVariable(node.nodeClass)) {
        addNodeById(node.nodeId);
        return;
    }
    addFolderById(node.nodeId);
}

///
/// \brief Stops monitoring a feature-selected node.
/// \param node Variable node to unsubscribe.
///
void DataAccessCoordinator::onUnsubscribeRequested(const OpcUaNodeInfo &node)
{
    if (node.nodeId.isEmpty() || !_monitoringState.isSubscribed(node.nodeId))
        return;
    _monitoringState.beginRequest(node.nodeId);
    updateMonitoringActions();
    _dataAccess->unsubscribe(node.nodeId);
}

///
/// \brief Reads a node so it can be added to Data Access after its attributes arrive.
/// \param nodeId Node to add.
///
void DataAccessCoordinator::addNodeById(const QString &nodeId)
{
    if (nodeId.isEmpty())
        return;
    _pendingDataAccessNodeIds.insert(nodeId);
    _attributes->read(nodeId);
}

///
/// \brief Asks for the variables a dropped folder contributes.
/// \param nodeId Container node dropped onto or selected for Data Access.
///
/// A single browse lists the folder's direct variable children, unless
/// AppSettings::recursiveFolderDrop() asks for the whole subtree to be crawled.
///
void DataAccessCoordinator::addFolderById(const QString &nodeId)
{
    if (nodeId.isEmpty() || !_addressSpace)
        return;
    if (_pendingFolderDropNodeIds.contains(nodeId))
        return;
    if (_pendingFolderDropNodeIds.isEmpty()) {
        QGuiApplication::setOverrideCursor(Qt::WaitCursor);
        _folderDropCursorActive = true;
    }
    _pendingFolderDropNodeIds.insert(nodeId);
    if (AppSettings().recursiveFolderDrop())
        _addressSpace->collectSubtreeVariables(nodeId);
    else
        _addressSpace->browse(nodeId);
}

///
/// \brief Restores the cursor after every pending folder browse or crawl has finished.
///
void DataAccessCoordinator::endFolderDropWait()
{
    if (!_folderDropCursorActive || !_pendingFolderDropNodeIds.isEmpty())
        return;
    QGuiApplication::restoreOverrideCursor();
    _folderDropCursorActive = false;
}

///
/// \brief Adds the direct variable children of a folder, within the add limits.
/// \param parentNodeId Browsed node.
/// \param children Browse result.
/// \param error Browse error, empty on success.
///
/// Only browse results for folders this coordinator asked about are handled; the same
/// signal also serves the address-space tree.
///
void DataAccessCoordinator::onFolderChildrenReady(const QString &parentNodeId,
                                                  const QVector<OpcUaNodeInfo> &children,
                                                  const QString &error)
{
    if (!_pendingFolderDropNodeIds.remove(parentNodeId))
        return;
    endFolderDropWait();
    offerFolderVariables(children, error);
}

///
/// \brief Adds the variables a subtree crawl found, within the add limits.
/// \param rootNodeId Node the crawl started from.
/// \param variables Variable nodes found, in breadth-first order.
/// \param error Crawl error, empty on success.
///
void DataAccessCoordinator::onFolderSubtreeVariablesReady(const QString &rootNodeId,
                                                          const QVector<OpcUaNodeInfo> &variables,
                                                          const QString &error)
{
    if (!_pendingFolderDropNodeIds.remove(rootNodeId))
        return;
    endFolderDropWait();
    offerFolderVariables(variables, error);
}

///
/// \brief Confirms and adds the variables a dropped folder contributed.
/// \param nodes Nodes the browse or the crawl reported.
/// \param error Browse or crawl error, empty on success.
///
void DataAccessCoordinator::offerFolderVariables(const QVector<OpcUaNodeInfo> &nodes,
                                                 const QString &error)
{
    if (!error.isEmpty()) {
        MessageBoxDialog::warning(_dialogParent, tr("Add Folder"), error, DialogButtonBox::Ok);
        return;
    }

    QVector<OpcUaNodeInfo> variables;
    QSet<QString> seenNodeIds;
    for (const OpcUaNodeInfo &child : nodes) {
        if (!OpcUa::isVariable(child.nodeClass) || child.nodeId.isEmpty())
            continue;
        if (seenNodeIds.contains(child.nodeId))
            continue;
        seenNodeIds.insert(child.nodeId);
        variables.append(child);
    }

    if (variables.isEmpty()) {
        MessageBoxDialog::information(_dialogParent, tr("Add Folder"),
                                      tr("This folder contains no variables to add."));
        return;
    }

    if (variables.size() > kFolderDropMaxNodes) {
        const DialogButtonBox::StandardButton answer = MessageBoxDialog::warning(
            _dialogParent, tr("Add Folder"),
            tr("This folder contains %1 variables, more than the limit of %2. "
               "Only the first %2 will be added.")
                .arg(variables.size()).arg(kFolderDropMaxNodes),
            DialogButtonBox::Ok | DialogButtonBox::Cancel, DialogButtonBox::Ok);
        if (answer != DialogButtonBox::Ok)
            return;
        variables.resize(kFolderDropMaxNodes);
    } else if (variables.size() > kFolderDropSilentLimit) {
        const DialogButtonBox::StandardButton answer = MessageBoxDialog::question(
            _dialogParent, tr("Add Folder"),
            tr("Add %n variable(s) to Data Access?", nullptr, variables.size()),
            DialogButtonBox::Yes | DialogButtonBox::No, DialogButtonBox::Yes);
        if (answer != DialogButtonBox::Yes)
            return;
    }

    addFolderVariables(variables);
}

///
/// \brief Shows the folder's variables at once and reads and subscribes them in the background.
/// \param variables Variable nodes to add.
///
void DataAccessCoordinator::addFolderVariables(const QVector<OpcUaNodeInfo> &variables)
{
    _dataView->dataAccess()->addPendingNodes(variables);
    for (const OpcUaNodeInfo &node : variables)
        _folderAddNodeIds.insert(node.nodeId);
    for (const OpcUaNodeInfo &node : variables)
        addNodeById(node.nodeId);
}

///
/// \brief Settles one row added by a folder drop and reports the batch's failures once.
/// \param nodeId Node that finished its read-and-subscribe chain.
/// \param success Whether the node was added successfully.
///
void DataAccessCoordinator::finishFolderNode(const QString &nodeId, bool success)
{
    _dataView->dataAccess()->clearNodePending(nodeId);
    if (!_folderAddNodeIds.remove(nodeId))
        return;
    if (!success)
        ++_folderAddFailureCount;
    if (!_folderAddNodeIds.isEmpty())
        return;

    const int failures = _folderAddFailureCount;
    _folderAddFailureCount = 0;
    if (failures > 0) {
        MessageBoxDialog::warning(
            _dialogParent, tr("Add Folder"),
            tr("%n variable(s) could not be added.", nullptr, failures),
            DialogButtonBox::Ok);
    }
}

///
/// \brief Opens the write dialog and writes the entered value on accept.
/// \param nodeId Node to write.
/// \param value Current value.
/// \param valueType OPC UA value type.
/// \param dataTypeId DataType NodeId.
/// \param writable Whether the user may write.
/// \param enumEntries Named values of the DataType; empty unless it is an enumeration.
///
void DataAccessCoordinator::showWriteDialog(const QString &nodeId, const QVariant &value,
                                            int valueType, const QString &dataTypeId,
                                            bool writable,
                                            const OpcUaEnumEntries &enumEntries)
{
    WriteValueDialog dialog(_dialogParent);
    dialog.setEnumEntries(enumEntries);
    dialog.setValue(value, valueType, dataTypeId, writable);
    if (dialog.exec() == QDialog::Accepted)
        _attributes->write(nodeId, dialog.value(), dialog.valueType());
}

///
/// \brief Enables the monitoring actions for the selected variable's current state.
///
void DataAccessCoordinator::updateMonitoringActions()
{
    const bool connected = _backend->state() == OpcUaConnectionState::Connected;
    const bool variable = connected && OpcUa::isVariable(_selectedNodeDetails.nodeClass)
        && !_selectedNodeDetails.nodeId.isEmpty();
    const bool subscribed = variable
        && _monitoringState.isSubscribed(_selectedNodeDetails.nodeId);
    const bool pending = variable
        && _monitoringState.isPending(_selectedNodeDetails.nodeId);
    _actions.subscribe->setEnabled(variable && !subscribed && !pending);
    _actions.unsubscribe->setEnabled(subscribed && !pending);
}

///
/// \brief Enables the data-access selection actions (Remove and Set Subscription).
///
void DataAccessCoordinator::updateSelectionActions()
{
    const bool connected = _backend->state() == OpcUaConnectionState::Connected;
    const bool actionable = connected && _dataView->dataAccess()->hasSelection();
    _actions.removeFromDataAccess->setEnabled(actionable);
    _actions.setSubscriptionNone->setEnabled(actionable);
    _actions.setSubscriptionDefault->setEnabled(actionable);
    _actions.setSubscriptionFast->setEnabled(actionable);
    _actions.setSubscriptionCustom->setEnabled(actionable);
}

///
/// \brief Returns the built-in Default or Fast subscription from the current list.
/// \param fast True for the Fast subscription, false for the Default subscription.
/// \return Matching subscription, or a sensible fallback when it is missing.
///
SubscriptionItem DataAccessCoordinator::builtinSubscription(bool fast) const
{
    const int wantedId = fast ? FastSubscriptionId : DefaultSubscriptionId;

    const QVector<SubscriptionItem> items = _dataView->subscriptions()->subscriptions();
    for (const SubscriptionItem &item : items) {
        if (item.isBuiltin() && item.id == wantedId)
            return item;
    }

    SubscriptionItem fallback;
    fallback.builtin = true;
    fallback.id = wantedId;
    fallback.name = SubscriptionsWidget::factoryName(wantedId);
    if (fast)
        fallback.publishingInterval = 250.0;
    return fallback;
}

///
/// \brief Connects the data view's widgets to the coordinator and the modules.
///
void DataAccessCoordinator::wireDataView()
{
    connect(_dataView->dataAccess(), &DataAccessWidget::addSelectedNodeRequested,
            this, &DataAccessCoordinator::addSelectedToView);
    connect(_dataView->dataAccess(), &DataAccessWidget::nodeDropRequested,
            this, &DataAccessCoordinator::addNodeById);
    connect(_dataView->dataAccess(), &DataAccessWidget::folderDropRequested,
            this, &DataAccessCoordinator::addFolderById);
    connect(_dataView->dataAccess(), &DataAccessWidget::writeRequested,
            this, &DataAccessCoordinator::showWriteDialog);
    connect(_dataView->dataAccess(), &DataAccessWidget::valueWriteRequested,
            _attributes, &AttributeModule::write);
    connect(_dataView->dataAccess(), &DataAccessWidget::readRequested,
            _dataAccess, &DataAccessModule::read);
    connect(_dataView->dataAccess(), &DataAccessWidget::monitoringRequested,
            _dataAccess, &DataAccessModule::subscribe);
    connect(_dataView->dataAccess(), &DataAccessWidget::monitoringCancelled,
            _dataAccess, &DataAccessModule::unsubscribe);
    connect(_dataView->dataAccess(), &DataAccessWidget::nodeCountChanged,
            this, &DataAccessCoordinator::onNodeCountChanged);
    connect(_dataView->dataAccess(), &DataAccessWidget::selectionChanged,
            this, &DataAccessCoordinator::updateSelectionActions);
    connect(_dataView->events(), &EventsWidget::eventSubscribeRequested,
            _events, &EventsModule::subscribeEvents);
    connect(_dataView->events(), &EventsWidget::eventUnsubscribeRequested,
            _events, &EventsModule::unsubscribeEvents);
    if (OpcUa::isHistoryReadSupported()) {
        connect(_dataView->dataHistory(), &DataHistoryWidget::dataHistoryReadRequested,
                _dataAccess, &DataAccessModule::readHistory);
        connect(_dataView->eventsHistory(), &EventsHistoryWidget::eventsHistoryReadRequested,
                _events, &EventsModule::readHistory);
        connect(_trendPanel, &TrendPanelWidget::historyReadRequested,
                _dataAccess, &DataAccessModule::readHistory);
    }
    connect(_trendPanel, &TrendPanelWidget::subscribeRequested,
            _dataAccess, &DataAccessModule::subscribe);
    connect(_trendPanel, &TrendPanelWidget::unsubscribeRequested,
            _dataAccess, &DataAccessModule::unsubscribe);

    SubscriptionsWidget *subscriptions = _dataView->subscriptions();
    _trendPanel->setSubscriptions(subscriptions->subscriptions());
    connect(subscriptions, &SubscriptionsWidget::subscriptionsChanged,
            _trendPanel, &TrendPanelWidget::setSubscriptions);
    connect(subscriptions, &SubscriptionsWidget::subscriptionRenamed,
            _trendPanel, &TrendPanelWidget::applySubscriptionRename);
    connect(_trendPanel, &TrendPanelWidget::subscriptionCreationRequested,
            subscriptions, &SubscriptionsWidget::createSubscription);

    connect(theApp(), &Application::timestampModeChanged,
            _dataView, &DataView::setTimestampMode);
    connect(theApp(), &Application::timestampModeChanged,
            _trendPanel, &TrendPanelWidget::setTimestampMode);
    connect(theApp(), &Application::highlightValueChangesChanged,
            _dataView, &DataView::setHighlightValueChanges);
}

///
/// \brief Connects the selection mediator's requests to the central area.
///
void DataAccessCoordinator::wireSelectionContext()
{
    connect(_selection, &SelectionContext::detailsReady,
            this, &DataAccessCoordinator::onDetailsReady);
    connect(_selection, &SelectionContext::cleared,
            this, &DataAccessCoordinator::onSelectionCleared);
    connect(_selection, &SelectionContext::eventMonitorRequested,
            this, &DataAccessCoordinator::onEventMonitorRequested);
    connect(_selection, &SelectionContext::addToTrendRequested,
            this, &DataAccessCoordinator::onAddToTrendRequested);
    connect(_selection, &SelectionContext::subscribeRequested,
            this, &DataAccessCoordinator::onSubscribeRequested);
    connect(_selection, &SelectionContext::unsubscribeRequested,
            this, &DataAccessCoordinator::onUnsubscribeRequested);
    if (OpcUa::isHistoryReadSupported()) {
        connect(_selection, &SelectionContext::historyReadRequested,
                this, &DataAccessCoordinator::onHistoryReadRequested);
        connect(_selection, &SelectionContext::eventsHistoryReadRequested,
                this, &DataAccessCoordinator::onEventsHistoryReadRequested);
    }
}

///
/// \brief Connects the data modules' results to the central area.
///
void DataAccessCoordinator::wireModules()
{
    connect(_attributes, &AttributeModule::attributesReady,
            this, &DataAccessCoordinator::onAttributeDetailsReady);
    connect(_attributes, &AttributeModule::writeFinished,
            this, &DataAccessCoordinator::onWriteFinished);
    connect(_dataAccess, &DataAccessModule::valuesReady,
            this, &DataAccessCoordinator::onDataValuesReady);
    connect(_dataAccess, &DataAccessModule::monitoringFinished,
            this, &DataAccessCoordinator::onMonitoringFinished);
    connect(_dataAccess, &DataAccessModule::monitoringIntervalRevised,
            this, &DataAccessCoordinator::onMonitoringIntervalRevised);
    connect(_events, &EventsModule::eventsReady,
            this, &DataAccessCoordinator::onEventsReady);
    connect(_events, &EventsModule::eventMonitoringFinished,
            this, &DataAccessCoordinator::onEventMonitoringFinished);
    if (_addressSpace) {
        connect(_addressSpace, &AddressSpaceModule::childrenReady,
                this, &DataAccessCoordinator::onFolderChildrenReady);
        connect(_addressSpace, &AddressSpaceModule::subtreeVariablesReady,
                this, &DataAccessCoordinator::onFolderSubtreeVariablesReady);
    }
    if (OpcUa::isHistoryReadSupported()) {
        connect(_dataAccess, &DataAccessModule::historyReady,
                this, &DataAccessCoordinator::onHistoryReady);
        connect(_events, &EventsModule::eventsHistoryReady,
                this, &DataAccessCoordinator::onEventsHistoryReady);
    }
}
