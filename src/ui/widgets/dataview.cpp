// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dataview.cpp
/// \brief Implements the tabbed data view container.
///

#include <QTabWidget>

#include "appsettings.h"
#include "dataaccesswidget.h"
#include "dataview.h"
#include "eventswidget.h"
#include "eventshistorywidget.h"
#include "datahistorywidget.h"
#include "subscriptionswidget.h"
#include "ui_dataview.h"

///
/// \brief Builds the tabbed view and its hosted tab widgets.
/// \param parent Parent widget.
///
DataView::DataView(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DataView)
{
    ui->setupUi(this);

    ui->mainTabs->setTabVisible(DataHistoryPage, OpcUa::isHistoryReadSupported());
    ui->mainTabs->setTabVisible(EventsHistoryPage, OpcUa::isHistoryReadSupported());

    connect(ui->subscriptionsWidget, &SubscriptionsWidget::subscriptionsChanged,
            ui->dataAccessWidget, &DataAccessWidget::setSubscriptions);
    connect(ui->subscriptionsWidget, &SubscriptionsWidget::subscriptionRenamed,
            ui->dataAccessWidget, &DataAccessWidget::applySubscriptionRename);
    connect(ui->subscriptionsWidget, &SubscriptionsWidget::subscriptionIntervalChanged,
            ui->dataAccessWidget, &DataAccessWidget::applySubscriptionInterval);
    connect(ui->subscriptionsWidget, &SubscriptionsWidget::subscriptionRemoved,
            ui->dataAccessWidget, &DataAccessWidget::applySubscriptionRemoval);

    ui->dataAccessWidget->setSubscriptions(ui->subscriptionsWidget->subscriptions());
}

///
/// \brief Destroys the view and its generated UI.
///
DataView::~DataView()
{
    delete ui;
}

///
/// \brief Returns the data access tab widget.
/// \return Data access widget.
///
DataAccessWidget *DataView::dataAccess() const
{
    return ui->dataAccessWidget;
}

///
/// \brief Returns the subscriptions tab widget.
/// \return Subscriptions widget.
///
SubscriptionsWidget *DataView::subscriptions() const
{
    return ui->subscriptionsWidget;
}

///
/// \brief Returns the events tab widget.
/// \return Events widget.
///
EventsWidget *DataView::events() const
{
    return ui->eventsWidget;
}

///
/// \brief Returns the data history tab widget.
/// \return Data history widget.
///
DataHistoryWidget *DataView::dataHistory() const
{
    return ui->dataHistoryWidget;
}

///
/// \brief Returns the events history tab widget.
/// \return Events history widget.
///
EventsHistoryWidget *DataView::eventsHistory() const
{
    return ui->eventsHistoryWidget;
}

///
/// \brief Switches the visible tab.
/// \param page Page to show.
///
void DataView::setCurrentPage(Page page)
{
    if ((page == DataHistoryPage || page == EventsHistoryPage) && !OpcUa::isHistoryReadSupported())
        page = DataAccessPage;
    ui->mainTabs->setCurrentIndex(static_cast<int>(page));
}

///
/// \brief Returns the currently visible tab index.
/// \return Index of the active page.
///
int DataView::currentPage() const
{
    return ui->mainTabs->currentIndex();
}

///
/// \brief Persists the header state of the hosted tab views.
/// \param settings Settings store to write to.
///
void DataView::saveViewState(AppSettings &settings) const
{
    ui->dataAccessWidget->saveViewState(settings);
    ui->subscriptionsWidget->saveViewState(settings);
    ui->eventsWidget->saveViewState(settings);
    ui->dataHistoryWidget->saveViewState(settings);
    ui->eventsHistoryWidget->saveViewState(settings);
}

///
/// \brief Restores the header state of the hosted tab views.
/// \param settings Settings store to read from.
///
void DataView::restoreViewState(AppSettings &settings)
{
    ui->dataAccessWidget->restoreViewState(settings);
    ui->subscriptionsWidget->restoreViewState(settings);
    ui->eventsWidget->restoreViewState(settings);
    ui->dataHistoryWidget->restoreViewState(settings);
    ui->eventsHistoryWidget->restoreViewState(settings);
}

///
/// \brief Adds or updates a node row and shows the Data Access page.
/// \param details Variable node details.
///
void DataView::addNode(const OpcUaNodeDetails &details)
{
    ui->dataAccessWidget->addNode(details);
    setCurrentPage(DataAccessPage);
}

///
/// \brief Adds a node, assigns it to the Default subscription and shows the Data Access page.
/// \param details Variable node details.
/// \param subscription Subscription to assign.
///
void DataView::addNodeWithDefaultSubscription(
    const OpcUaNodeDetails &details,
    const SubscriptionItem &subscription)
{
    ui->dataAccessWidget->addNodeWithDefaultSubscription(details, subscription);
    setCurrentPage(DataAccessPage);
}

///
/// \brief Applies read results to the data rows.
/// \param values Read results.
///
void DataView::updateValues(const QVector<OpcUaDataValue> &values)
{
    ui->dataAccessWidget->updateValues(values);
}

///
/// \brief Shows data history samples in the Data History table.
/// \param values Data history samples in time order.
///
void DataView::setDataHistoryResults(const QVector<OpcUaHistoryValue> &values)
{
    ui->dataHistoryWidget->setDataHistoryResults(values);
}

///
/// \brief Targets a node on the Data History page and requests its history for the current range.
/// \param nodeId Node whose data history should be read.
/// \param displayName Human-readable node name.
/// \param displayPath Human-readable path shown in the node field.
///
void DataView::requestDataHistoryForNode(const QString &nodeId, const QString &displayName,
                                     const QString &displayPath)
{
    if (!OpcUa::isHistoryReadSupported())
        return;
    setCurrentPage(DataHistoryPage);
    ui->dataHistoryWidget->requestDataHistoryForNode(nodeId, displayName, displayPath);
}

///
/// \brief Targets a node on the Events page as the event source and shows the page.
/// \param nodeId Node to monitor for events.
/// \param displayName Human-readable name shown in the source field.
/// \param displayPath Human-readable path shown in the source field.
///
void DataView::beginEventMonitoring(const QString &nodeId, const QString &displayName,
                                    const QString &displayPath)
{
    ui->eventsWidget->setEventSource(nodeId, displayName, displayPath);
    setCurrentPage(EventsPage);
}

///
/// \brief Targets a node on the Events page and requests event monitoring.
/// \param nodeId Node to monitor for events.
/// \param displayName Human-readable name shown in the source field.
/// \param displayPath Human-readable path shown in the source field.
///
void DataView::requestEventMonitoringForNode(const QString &nodeId, const QString &displayName,
                                             const QString &displayPath)
{
    beginEventMonitoring(nodeId, displayName, displayPath);
    ui->eventsWidget->requestEventMonitoring();
}

///
/// \brief Appends received events to the Events table.
/// \param events Events to display.
///
void DataView::appendEvents(const QVector<OpcUaEvent> &events)
{
    ui->eventsWidget->appendEvents(events);
}

///
/// \brief Shows historical events in the Events History table.
/// \param events Events to display.
///
void DataView::setEventsHistoryResults(const QVector<OpcUaEvent> &events)
{
    ui->eventsHistoryWidget->setEventsHistoryResults(events);
}

///
/// \brief Targets a node on the Events History page and requests its event history.
/// \param nodeId Node whose event history should be read.
/// \param displayName Human-readable node name.
/// \param displayPath Human-readable path shown in the node field.
///
void DataView::requestEventsHistoryForNode(const QString &nodeId, const QString &displayName,
                                           const QString &displayPath)
{
    setCurrentPage(EventsHistoryPage);
    ui->eventsHistoryWidget->requestEventsHistoryForNode(nodeId, displayName, displayPath);
}

///
/// \brief Builds the default CSV export file name for the current data history query.
/// \return Suggested CSV file name.
///
QString DataView::suggestedDataHistoryCsvFileName() const
{
    return ui->dataHistoryWidget->suggestedDataHistoryCsvFileName();
}

///
/// \brief Updates the subscription shown for a data-access node.
/// \param nodeId Affected node.
/// \param subscribed Whether the node belongs to the default subscription.
///
void DataView::setNodeSubscribed(const QString &nodeId, bool subscribed)
{
    ui->dataAccessWidget->setNodeSubscribed(nodeId, subscribed);
}

///
/// \brief Clears the data, subscriptions, live events, and history tabs.
///
void DataView::clearRuntimeData()
{
    ui->dataAccessWidget->clear();
    ui->subscriptionsWidget->reset();
    ui->eventsWidget->clear();
    ui->dataHistoryWidget->clear();
    ui->eventsHistoryWidget->clear();
}

///
/// \brief Applies the OPC UA timestamp display mode to the data and history tabs.
/// \param mode Local time or UTC.
///
void DataView::setTimestampMode(AppSettings::TimestampMode mode)
{
    ui->dataAccessWidget->setTimestampMode(mode);
    ui->dataHistoryWidget->setTimestampMode(mode);
    ui->eventsHistoryWidget->setTimestampMode(mode);
}
