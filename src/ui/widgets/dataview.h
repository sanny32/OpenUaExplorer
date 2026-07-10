// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dataview.h
/// \brief Declares the tabbed data view hosting data, events, and history tabs.
///

#pragma once

#include <QWidget>

#include "appsettings.h"
#include "models/subscriptionitem.h"
#include "opcua/opcuatypes.h"

namespace Ui {
class DataView;
}

class DataAccessWidget;
class SubscriptionsDialog;
class SubscriptionsWidget;
class EventsWidget;
class EventsHistoryWidget;
class DataHistoryWidget;

///
/// \brief Tabbed container for data access, live events, and history widgets.
///
class DataView : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Pages hosted by the data view.
    ///
    enum Page {
        DataAccessPage = 0,
        EventsPage = 2,
        DataHistoryPage = 3,
        EventsHistoryPage = 4
    };

    ///
    /// \brief Builds the tabbed view and its hosted tab widgets.
    /// \param parent Parent widget.
    ///
    explicit DataView(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the view and its generated UI.
    ///
    ~DataView() override;

    ///
    /// \brief Returns the data access tab widget.
    /// \return Data access widget.
    ///
    DataAccessWidget *dataAccess() const;

    ///
    /// \brief Returns the subscriptions dialog widget.
    /// \return Subscriptions widget.
    ///
    SubscriptionsWidget *subscriptions() const;

    ///
    /// \brief Returns the events tab widget.
    /// \return Events widget.
    ///
    EventsWidget *events() const;

    ///
    /// \brief Returns the data history tab widget.
    /// \return Data history widget.
    ///
    DataHistoryWidget *dataHistory() const;

    ///
    /// \brief Returns the events history tab widget.
    /// \return Events history widget.
    ///
    EventsHistoryWidget *eventsHistory() const;

    ///
    /// \brief Switches the visible tab.
    /// \param page Page to show.
    ///
    void setCurrentPage(Page page);

    ///
    /// \brief Returns the currently visible tab index.
    /// \return Legacy page value of the active page.
    ///
    int currentPage() const;

    ///
    /// \brief Shows the subscriptions management dialog.
    ///
    void showSubscriptionsDialog();

    ///
    /// \brief Persists the header state of the hosted tab views.
    /// \param settings Settings store to write to.
    ///
    void saveViewState(AppSettings &settings) const;

    ///
    /// \brief Restores the header state of the hosted tab views.
    /// \param settings Settings store to read from.
    ///
    void restoreViewState(AppSettings &settings);

    ///
    /// \brief Adds or updates a node row and shows the Data Access page.
    /// \param details Variable node details.
    ///
    void addNode(const OpcUaNodeDetails &details);

    ///
    /// \brief Adds a node, assigns it to the Default subscription and shows the Data Access page.
    /// \param details Variable node details.
    /// \param subscription Subscription to assign.
    ///
    void addNodeWithDefaultSubscription(
        const OpcUaNodeDetails &details,
        const SubscriptionItem &subscription = SubscriptionItem());

    ///
    /// \brief Applies read results to the data rows.
    /// \param values Read results.
    ///
    void updateValues(const QVector<OpcUaDataValue> &values);

    ///
    /// \brief Shows data history samples in the Data History table.
    /// \param values Data history samples in time order.
    ///
    void setDataHistoryResults(const QVector<OpcUaHistoryValue> &values);

    ///
    /// \brief Targets a node on the Events page as the event source and shows the page.
    /// \param nodeId Node to monitor for events.
    /// \param displayName Human-readable name shown in the source field.
    /// \param displayPath Human-readable path shown in the source field.
    ///
    void beginEventMonitoring(const QString &nodeId, const QString &displayName,
                              const QString &displayPath = {});

    ///
    /// \brief Targets a node on the Events page and requests event monitoring.
    /// \param nodeId Node to monitor for events.
    /// \param displayName Human-readable name shown in the source field.
    /// \param displayPath Human-readable path shown in the source field.
    ///
    void requestEventMonitoringForNode(const QString &nodeId, const QString &displayName,
                                       const QString &displayPath = {});

    ///
    /// \brief Appends received events to the Events table.
    /// \param events Events to display.
    ///
    void appendEvents(const QVector<OpcUaEvent> &events);

    ///
    /// \brief Shows historical events in the Events History table.
    /// \param events Events to display.
    ///
    void setEventsHistoryResults(const QVector<OpcUaEvent> &events);

    ///
    /// \brief Targets a node on the Events History page and requests its event history.
    /// \param nodeId Node whose event history should be read.
    /// \param displayName Human-readable node name.
    /// \param displayPath Human-readable path shown in the node field.
    ///
    void requestEventsHistoryForNode(const QString &nodeId, const QString &displayName,
                                     const QString &displayPath = {});

    ///
    /// \brief Targets a node on the Data History page and requests its history for the current range.
    /// \param nodeId Node whose data history should be read.
    /// \param displayName Human-readable node name.
    /// \param displayPath Human-readable path shown in the node field.
    ///
    void requestDataHistoryForNode(const QString &nodeId, const QString &displayName,
                               const QString &displayPath = {});

    ///
    /// \brief Builds the default CSV export file name for the current data history query.
    /// \return Suggested CSV file name.
    ///
    QString suggestedDataHistoryCsvFileName() const;

    ///
    /// \brief Updates the subscription shown for a data-access node.
    /// \param nodeId Affected node.
    /// \param subscribed Whether the node belongs to the default subscription.
    ///
    void setNodeSubscribed(const QString &nodeId, bool subscribed);

    ///
    /// \brief Removes every node from the Data Access page, cancelling their monitoring.
    ///
    void clearDataAccessNodes();

    ///
    /// \brief Clears the data, subscriptions, live events, and history tabs.
    ///
    void clearRuntimeData();

public slots:
    ///
    /// \brief Applies the OPC UA timestamp display mode to the data and history tabs.
    /// \param mode Local time or UTC.
    ///
    void setTimestampMode(AppSettings::TimestampMode mode);

private:
    Ui::DataView *ui;
    SubscriptionsDialog *_subscriptionsDialog;
};
