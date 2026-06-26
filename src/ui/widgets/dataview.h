// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dataview.h
/// \brief Declares the tabbed data view hosting the data, subscriptions, events and history tabs.
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
class SubscriptionsWidget;
class EventsWidget;
class HistoryWidget;

///
/// \brief Tabbed container for the data access, subscriptions, events and history widgets.
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
        SubscriptionsPage,
        EventsPage,
        HistoryPage
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
    /// \brief Returns the subscriptions tab widget.
    /// \return Subscriptions widget.
    ///
    SubscriptionsWidget *subscriptions() const;

    ///
    /// \brief Returns the events tab widget.
    /// \return Events widget.
    ///
    EventsWidget *events() const;

    ///
    /// \brief Returns the history tab widget.
    /// \return History widget.
    ///
    HistoryWidget *history() const;

    ///
    /// \brief Switches the visible tab.
    /// \param page Page to show.
    ///
    void setCurrentPage(Page page);

    ///
    /// \brief Returns the currently visible tab index.
    /// \return Index of the active page.
    ///
    int currentPage() const;

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
    /// \brief Shows history samples in the History table.
    /// \param values History samples in time order.
    ///
    void setHistoryResults(const QVector<OpcUaHistoryValue> &values);

    ///
    /// \brief Targets a node on the Events page as the event source and shows the page.
    /// \param nodeId Node to monitor for events.
    /// \param displayName Human-readable name shown in the source field.
    ///
    void beginEventMonitoring(const QString &nodeId, const QString &displayName);

    ///
    /// \brief Targets a node on the Events page and requests event monitoring.
    /// \param nodeId Node to monitor for events.
    /// \param displayName Human-readable name shown in the source field.
    ///
    void requestEventMonitoringForNode(const QString &nodeId, const QString &displayName);

    ///
    /// \brief Appends received events to the Events table.
    /// \param events Events to display.
    ///
    void appendEvents(const QVector<OpcUaEvent> &events);

    ///
    /// \brief Targets a node on the History page and requests its history for the current range.
    /// \param nodeId Node whose history should be read.
    /// \param displayName Human-readable name shown in the node field.
    ///
    void requestHistoryForNode(const QString &nodeId, const QString &displayName);

    ///
    /// \brief Builds the default CSV export file name for the current history query.
    /// \return Suggested CSV file name.
    ///
    QString suggestedHistoryCsvFileName() const;

    ///
    /// \brief Updates the subscription shown for a data-access node.
    /// \param nodeId Affected node.
    /// \param subscribed Whether the node belongs to the default subscription.
    ///
    void setNodeSubscribed(const QString &nodeId, bool subscribed);

    ///
    /// \brief Clears the data, subscriptions, events, and history tabs.
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
};
