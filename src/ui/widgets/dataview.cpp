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
#include "historywidget.h"
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

    ui->mainTabs->setTabVisible(HistoryPage, OpcUa::isHistoryReadSupported());

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
/// \brief Returns the history tab widget.
/// \return History widget.
///
HistoryWidget *DataView::history() const
{
    return ui->historyWidget;
}

///
/// \brief Switches the visible tab.
/// \param page Page to show.
///
void DataView::setCurrentPage(Page page)
{
    if (page == HistoryPage && !OpcUa::isHistoryReadSupported())
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
    ui->historyWidget->saveViewState(settings);
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
    ui->historyWidget->restoreViewState(settings);
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
/// \brief Shows history samples in the History table.
/// \param values History samples in time order.
///
void DataView::setHistoryResults(const QVector<OpcUaHistoryValue> &values)
{
    ui->historyWidget->setHistoryResults(values);
}

///
/// \brief Targets a node on the History page and requests its history for the current range.
/// \param nodeId Node whose history should be read.
/// \param displayName Human-readable node name.
/// \param displayPath Human-readable path shown in the node field.
///
void DataView::requestHistoryForNode(const QString &nodeId, const QString &displayName,
                                     const QString &displayPath)
{
    if (!OpcUa::isHistoryReadSupported())
        return;
    setCurrentPage(HistoryPage);
    ui->historyWidget->requestHistoryForNode(nodeId, displayName, displayPath);
}

///
/// \brief Targets a node on the Events page as the event source and shows the page.
/// \param nodeId Node to monitor for events.
/// \param displayName Human-readable name shown in the source field.
///
void DataView::beginEventMonitoring(const QString &nodeId, const QString &displayName)
{
    ui->eventsWidget->setEventSource(nodeId, displayName);
    setCurrentPage(EventsPage);
}

///
/// \brief Targets a node on the Events page and requests event monitoring.
/// \param nodeId Node to monitor for events.
/// \param displayName Human-readable name shown in the source field.
///
void DataView::requestEventMonitoringForNode(const QString &nodeId, const QString &displayName)
{
    beginEventMonitoring(nodeId, displayName);
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
/// \brief Builds the default CSV export file name for the current history query.
/// \return Suggested CSV file name.
///
QString DataView::suggestedHistoryCsvFileName() const
{
    return ui->historyWidget->suggestedHistoryCsvFileName();
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
/// \brief Clears the data, subscriptions, events, and history tabs.
///
void DataView::clearRuntimeData()
{
    ui->dataAccessWidget->clear();
    ui->subscriptionsWidget->reset();
    ui->eventsWidget->clear();
    ui->historyWidget->clear();
}

///
/// \brief Applies the OPC UA timestamp display mode to the data and history tabs.
/// \param mode Local time or UTC.
///
void DataView::setTimestampMode(AppSettings::TimestampMode mode)
{
    ui->dataAccessWidget->setTimestampMode(mode);
    ui->historyWidget->setTimestampMode(mode);
}
