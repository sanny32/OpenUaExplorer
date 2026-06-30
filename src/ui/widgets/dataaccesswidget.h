// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dataaccesswidget.h
/// \brief Declares the OPC UA data access tab widget.
///

#pragma once

#include <QModelIndex>
#include <QVector>
#include <QWidget>

#include "appsettings.h"
#include "models/subscriptionitem.h"
#include "opcua/opcuatypes.h"

class QMenu;

namespace Ui {
class DataAccessWidget;
}

class DataAccessModel;
class SubscriptionDelegate;

///
/// \brief Tab widget for browsing and monitoring data access items.
///
class DataAccessWidget : public QWidget
{
    Q_OBJECT

public:
    ///
    /// \brief Builds the widget and its data view.
    /// \param parent Parent widget.
    ///
    explicit DataAccessWidget(QWidget *parent = nullptr);

    ///
    /// \brief Destroys the widget and its generated UI.
    ///
    ~DataAccessWidget() override;

    ///
    /// \brief Adds or updates a node row.
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
    /// \brief Updates the subscription shown for a data-access node.
    /// \param nodeId Affected node.
    /// \param subscribed Whether the node belongs to the default subscription.
    ///
    void setNodeSubscribed(const QString &nodeId, bool subscribed);

    ///
    /// \brief Removes all data-access rows.
    ///
    void clear();

    ///
    /// \brief Persists the data view header state.
    /// \param settings Settings store to write to.
    ///
    void saveViewState(AppSettings &settings) const;

    ///
    /// \brief Restores the data view header state.
    /// \param settings Settings store to read from.
    ///
    void restoreViewState(AppSettings &settings);

public slots:
    ///
    /// \brief Applies the OPC UA timestamp display mode to the data-access table.
    /// \param mode Local time or UTC.
    ///
    void setTimestampMode(AppSettings::TimestampMode mode);

    ///
    /// \brief Replaces the known subscriptions and rebuilds the subscribe menu.
    /// \param subscriptions Current subscriptions.
    ///
    void setSubscriptions(const QVector<SubscriptionItem> &subscriptions);

    ///
    /// \brief Repoints data-access nodes from a renamed subscription to its new name.
    /// \param oldName Previous subscription name.
    /// \param newName New subscription name.
    ///
    void applySubscriptionRename(const QString &oldName, const QString &newName);

    ///
    /// \brief Re-establishes monitoring at a new interval for a subscription's nodes.
    /// \param name Subscription whose interval changed.
    /// \param interval New publishing interval in milliseconds.
    ///
    void applySubscriptionInterval(const QString &name, double interval);

    ///
    /// \brief Unassigns and stops monitoring nodes of a removed subscription.
    /// \param name Subscription being removed.
    ///
    void applySubscriptionRemoval(const QString &name);

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

    ///
    /// \brief Emitted when the user asks to create a new subscription from the Subscription column.
    /// \param name New subscription name.
    /// \param publishingInterval New subscription publishing interval in milliseconds.
    ///
    void subscriptionCreationRequested(QString name, double publishingInterval);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void setupDataView();
    void configureToolbar();
    void showDataContextMenu(const QPoint &pos);
    void removeSelectedNodes();
    void readSelectedNodes();
    void writeSelectedNode();
    void removeAllNodes();
    void rebuildSubscribeMenu();
    void populateSubscribeMenu(QMenu *menu);
    void applySubscriptionToSelection(const QString &subscriptionName);
    void promptNewSubscription(const QString &nodeId);
    QStringList subscriptionNames() const;
    double intervalFor(const QString &name) const;
    SubscriptionItem defaultSubscription() const;
    QModelIndexList selectedDataRows() const;

    Ui::DataAccessWidget      *ui;
    DataAccessModel           *_dataModel;
    SubscriptionDelegate      *_subscriptionDelegate = nullptr;
    QVector<SubscriptionItem>  _subscriptions;
};
