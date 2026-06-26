// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dataaccesswidget.h
/// \brief Declares the OPC UA data access widget.
///

#pragma once

#include <QModelIndex>
#include <QWidget>

#include "appsettings.h"
#include "models/subscriptionitem.h"
#include "opcua/opcuatypes.h"

class QDateTimeEdit;
class QMenu;

namespace Ui {
class DataAccessWidget;
}

class DataAccessModel;
class SubscriptionsModel;
class EventsModel;
class HistoryModel;

///
/// \brief Widget for browsing data access items, subscriptions, events and history.
///
class DataAccessWidget : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Pages hosted by the data access widget.
    ///
    enum Page {
        DataAccessPage = 0,
        SubscriptionsPage,
        EventsPage,
        HistoryPage
    };

    ///
    /// \brief Builds the widget and its data, subscriptions, events, and history views.
    /// \param parent Parent widget.
    ///
    explicit DataAccessWidget(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the widget and its generated UI.
    ///
    ~DataAccessWidget() override;

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
    /// \brief Enables or disables the History page.
    /// \param available True when the server connection can serve history reads.
    ///
    void setHistoryAvailable(bool available);

    ///
    /// \brief Persists the header state of the data, subscriptions, events, and history views.
    /// \param settings Settings store to write to.
    ///
    void saveViewState(AppSettings &settings) const;

    ///
    /// \brief Restores the header state of the data, subscriptions, events, and history views.
    /// \param settings Settings store to read from.
    ///
    void restoreViewState(AppSettings &settings);

    ///
    /// \brief Adds or updates a node row and shows the Data Access page.
    /// \param details Variable node details.
    ///
    void addNode(const OpcUaNodeDetails &details);

    ///
    /// \brief Adds or updates a node row and assigns it to the Default subscription.
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
    /// \brief Clears the data, subscriptions, events, and history models.
    ///
    void clearRuntimeData();

public slots:
    ///
    /// \brief Applies the OPC UA timestamp display mode to the data-access table.
    /// \param mode Local time or UTC.
    ///
    void setTimestampMode(AppSettings::TimestampMode mode);

signals:
    ///
    /// \brief Emitted when the user asks to add the selected node.
    ///
    void addSelectedNodeRequested();

    ///
    /// \brief Emitted when an address-space node is dropped onto Data Access.
    /// \param nodeId Dropped OPC UA node.
    ///
    void nodeDropRequested(QString nodeId);

    ///
    /// \brief Emitted when the user requests a read of nodes.
    /// \param nodeIds Nodes to read.
    ///
    void readRequested(QStringList nodeIds);

    ///
    /// \brief Emitted when the user requests a raw history read for a node.
    /// \param nodeId Node whose history should be read.
    /// \param start Inclusive range start.
    /// \param end Inclusive range end.
    /// \param maxValues Maximum samples to return, or 0 for no limit.
    ///
    void historyReadRequested(QString nodeId, QDateTime start, QDateTime end, quint32 maxValues);

    ///
    /// \brief Emitted when the user requests a value write.
    /// \param nodeId Target node.
    /// \param currentValue Current value to seed the editor.
    /// \param valueType OPC UA value type.
    /// \param dataTypeId DataType NodeId.
    /// \param writable Whether the user may write.
    ///
    void writeRequested(QString nodeId, QVariant currentValue, int valueType,
                        QString dataTypeId, bool writable);

    ///
    /// \brief Emitted when a node should be monitored at a subscription's publishing interval.
    /// \param nodeId Node to monitor.
    /// \param publishingInterval Publishing interval in milliseconds.
    ///
    void monitoringRequested(QString nodeId, double publishingInterval);

    ///
    /// \brief Emitted when monitoring for a node should be cancelled.
    /// \param nodeId Node to stop monitoring.
    ///
    void monitoringCancelled(QString nodeId);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void setupDataView();
    void setupSubscriptionsView();
    void setupEventsView();
    void setupHistoryView();
    void clearHistoryNode();
    void requestHistoryRead();
    void exportHistoryToCsv();
    void applyHistoryTimestampMode(AppSettings::TimestampMode mode);
    void updateHistoryZoneSuffix(QDateTimeEdit *edit);
    void configureToolbar();
    void showDataContextMenu(const QPoint &pos);
    void removeSelectedNodes();
    void readSelectedNodes();
    void writeSelectedNode();
    void removeAllNodes();
    void rebuildSubscribeMenu();
    void populateSubscribeMenu(QMenu *menu);
    void applySubscriptionToSelection(const QString &subscriptionName);
    void resetSubscriptions();
    void showSubscriptionsContextMenu(const QPoint &pos);
    void addSubscription();
    void removeSelectedSubscriptions();
    void removeAllSubscriptions();
    void removeSubscriptionRow(int row);
    void renameSubscriptionAssignments(const QString &oldName, const QString &newName);
    void reapplySubscriptionInterval(const QString &name, double interval);
    QModelIndexList selectedDataRows() const;

    Ui::DataAccessWidget *ui;
    DataAccessModel      *_dataModel;
    SubscriptionsModel   *_subscriptionsModel;
    EventsModel          *_eventsModel;
    HistoryModel         *_historyModel;
    QString               _historyNodeId;
};
