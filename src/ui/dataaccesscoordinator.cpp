// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dataaccesscoordinator.cpp
/// \brief Implements the coordinator of the central data-access, events, history and trend area.
///

#include <QAction>
#include <QMessageBox>

#include "application.h"
#include "appsettings.h"
#include "attributemodule.h"
#include "dataaccesscoordinator.h"
#include "dataaccessmodule.h"
#include "dialogs/writevaluedialog.h"
#include "eventsmodule.h"
#include "features/selectioncontext.h"
#include "opcua/opcuaclientservice.h"
#include "widgets/dataaccesswidget.h"
#include "widgets/datahistorywidget.h"
#include "widgets/dataview.h"
#include "widgets/eventshistorywidget.h"
#include "widgets/eventswidget.h"
#include "widgets/subscriptionswidget.h"
#include "widgets/trendpanelwidget.h"

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
DataAccessCoordinator::DataAccessCoordinator(DataView *dataView,
                                             TrendPanelWidget *trendPanel,
                                             DataAccessModule *dataAccess,
                                             EventsModule *events,
                                             AttributeModule *attributes,
                                             SelectionContext *selection,
                                             OpcUaClientService *clientService,
                                             const DataAccessActions &actions,
                                             QWidget *dialogParent)
    : QObject(dialogParent)
    , _dataView(dataView)
    , _trendPanel(trendPanel)
    , _dataAccess(dataAccess)
    , _events(events)
    , _attributes(attributes)
    , _selection(selection)
    , _clientService(clientService)
    , _actions(actions)
    , _dialogParent(dialogParent)
{
    wireDataView();
    wireSelectionContext();
    wireModules();
    connect(_clientService, &OpcUaClientService::stateChanged,
            this, &DataAccessCoordinator::onClientStateChanged);
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
                    OpcUa::isWritable(_selectedNodeDetails.userAccessLevel));
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
    _pendingMonitoringNodeIds.insert(_selectedNodeDetails.nodeId);
    updateMonitoringActions();
    _dataAccess->subscribe(_selectedNodeDetails.nodeId);
}

///
/// \brief Stops monitoring the selected variable.
///
void DataAccessCoordinator::unsubscribeSelected()
{
    if (_selectedNodeDetails.nodeId.isEmpty()
        || !_subscribedNodeIds.contains(_selectedNodeDetails.nodeId)) {
        return;
    }
    _pendingMonitoringNodeIds.insert(_selectedNodeDetails.nodeId);
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
/// \brief Switches the data view to the Data Access page.
///
void DataAccessCoordinator::showDataAccessPage()
{
    _dataView->setCurrentPage(DataView::DataAccessPage);
}

///
/// \brief Switches the data view to the Events page.
///
void DataAccessCoordinator::showEventsPage()
{
    _dataView->setCurrentPage(DataView::EventsPage);
}

///
/// \brief Switches the data view to the Data History page when history is supported.
///
void DataAccessCoordinator::showDataHistoryPage()
{
    if (!OpcUa::isHistoryReadSupported())
        return;
    _dataView->setCurrentPage(DataView::DataHistoryPage);
}

///
/// \brief Switches the data view to the Events History page when history is supported.
///
void DataAccessCoordinator::showEventsHistoryPage()
{
    if (!OpcUa::isHistoryReadSupported())
        return;
    _dataView->setCurrentPage(DataView::EventsHistoryPage);
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
/// \brief Persists the visible page and the view element state of the central area.
/// \param settings Settings store to write to.
///
void DataAccessCoordinator::saveState(AppSettings &settings) const
{
    settings.setDataAccessPage(_dataView->currentPage());
    _dataView->saveViewState(settings);
    _dataView->subscriptions()->saveSubscriptions(settings);
    _trendPanel->saveViewState(settings);
}

///
/// \brief Restores the visible page and the view element state of the central area.
/// \param settings Settings store to read from.
///
void DataAccessCoordinator::restoreState(AppSettings &settings)
{
    _dataView->setCurrentPage(static_cast<DataView::Page>(settings.dataAccessPage()));
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
    _subscribedNodeIds.clear();
    _pendingMonitoringNodeIds.clear();
    _pendingDataAccessNodeIds.clear();
    updateMonitoringActions();
}

///
/// \brief Applies raw attribute results that are not tied to the current selection.
/// \param details Read node details.
/// \param error Read error, if any.
///
void DataAccessCoordinator::onAttributeDetailsReady(const OpcUaNodeDetails &details,
                                                    const QString &error)
{
    if (!error.isEmpty())
        return;

    if (_pendingDataAccessNodeIds.remove(details.nodeId) && OpcUa::isVariable(details.nodeClass))
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
        QMessageBox::warning(_dialogParent, tr("Data History Read Failed"), error);
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
        QMessageBox::warning(_dialogParent, tr("Write Failed"), error);
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
    _pendingMonitoringNodeIds.remove(nodeId);
    if (success) {
        if (subscribed)
            _subscribedNodeIds.insert(nodeId);
        else
            _subscribedNodeIds.remove(nodeId);
        _dataView->setNodeSubscribed(nodeId, subscribed);
    } else {
        QMessageBox::warning(_dialogParent,
                             subscribed ? tr("Subscribe Failed") : tr("Unsubscribe Failed"),
                             error);
    }
    updateMonitoringActions();
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
        QMessageBox::warning(_dialogParent, tr("Events History Read Failed"), error);
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
        QMessageBox::warning(_dialogParent,
                             subscribed ? tr("Event Subscribe Failed")
                                        : tr("Event Unsubscribe Failed"),
                             error);
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
/// \brief Adds a feature-selected node to the trend panel.
/// \param node Variable node to chart.
///
void DataAccessCoordinator::onAddToTrendRequested(const OpcUaNodeInfo &node)
{
    const QString name = node.displayName.isEmpty() ? node.browseName : node.displayName;
    _trendPanel->addNode(node.nodeId, name, node.displayPath);
}

///
/// \brief Starts monitoring a feature-selected node.
/// \param node Variable node to subscribe.
///
void DataAccessCoordinator::onSubscribeRequested(const OpcUaNodeInfo &node)
{
    addNodeById(node.nodeId);
}

///
/// \brief Stops monitoring a feature-selected node.
/// \param node Variable node to unsubscribe.
///
void DataAccessCoordinator::onUnsubscribeRequested(const OpcUaNodeInfo &node)
{
    if (node.nodeId.isEmpty() || !_subscribedNodeIds.contains(node.nodeId))
        return;
    _pendingMonitoringNodeIds.insert(node.nodeId);
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
/// \brief Opens the write dialog and writes the entered value on accept.
/// \param nodeId Node to write.
/// \param value Current value.
/// \param valueType OPC UA value type.
/// \param dataTypeId DataType NodeId.
/// \param writable Whether the user may write.
///
void DataAccessCoordinator::showWriteDialog(const QString &nodeId, const QVariant &value,
                                            int valueType, const QString &dataTypeId,
                                            bool writable)
{
    WriteValueDialog dialog(_dialogParent);
    dialog.setValue(value, valueType, dataTypeId, writable);
    if (dialog.exec() == QDialog::Accepted)
        _attributes->write(nodeId, dialog.value(), dialog.valueType());
}

///
/// \brief Enables the monitoring actions for the selected variable's current state.
///
void DataAccessCoordinator::updateMonitoringActions()
{
    const bool connected = _clientService->state() == OpcUaConnectionState::Connected;
    const bool variable = connected && OpcUa::isVariable(_selectedNodeDetails.nodeClass)
        && !_selectedNodeDetails.nodeId.isEmpty();
    const bool subscribed = variable
        && _subscribedNodeIds.contains(_selectedNodeDetails.nodeId);
    const bool pending = variable
        && _pendingMonitoringNodeIds.contains(_selectedNodeDetails.nodeId);
    _actions.subscribe->setEnabled(variable && !subscribed && !pending);
    _actions.unsubscribe->setEnabled(subscribed && !pending);
}

///
/// \brief Enables the data-access selection actions (Remove and Set Subscription).
///
void DataAccessCoordinator::updateSelectionActions()
{
    const bool connected = _clientService->state() == OpcUaConnectionState::Connected;
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
    const QVector<SubscriptionItem> items = _dataView->subscriptions()->subscriptions();
    for (const SubscriptionItem &item : items) {
        const bool matches = fast ? (item.isBuiltin() && !item.isDefault()) : item.isDefault();
        if (matches)
            return item;
    }

    SubscriptionItem fallback;
    fallback.builtin = true;
    if (fast) {
        fallback.name = tr("Fast");
        fallback.publishingInterval = 250.0;
        fallback.id = 1;
    } else {
        fallback.name = tr("Default");
    }
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
    connect(_dataView->dataAccess(), &DataAccessWidget::writeRequested,
            this, &DataAccessCoordinator::showWriteDialog);
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
    connect(_events, &EventsModule::eventsReady,
            this, &DataAccessCoordinator::onEventsReady);
    connect(_events, &EventsModule::eventMonitoringFinished,
            this, &DataAccessCoordinator::onEventMonitoringFinished);
    if (OpcUa::isHistoryReadSupported()) {
        connect(_dataAccess, &DataAccessModule::historyReady,
                this, &DataAccessCoordinator::onHistoryReady);
        connect(_events, &EventsModule::eventsHistoryReady,
                this, &DataAccessCoordinator::onEventsHistoryReady);
    }
}
