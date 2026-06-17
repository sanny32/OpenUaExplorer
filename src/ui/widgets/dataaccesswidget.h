// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dataaccesswidget.h
/// \brief Declares the OPC UA data access widget.
///

#pragma once

#include <QModelIndex>
#include <QWidget>

#include "opcua/opcuatypes.h"

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
    /// \brief Adds or updates a node row and shows the Data Access page.
    /// \param details Variable node details.
    ///
    void addNode(const OpcUaNodeDetails &details);

    ///
    /// \brief Applies read results to the data rows.
    /// \param values Read results.
    ///
    void updateValues(const QVector<OpcUaDataValue> &values);

    ///
    /// \brief Clears the data, subscriptions, events, and history models.
    ///
    void clearRuntimeData();

signals:
    ///
    /// \brief Emitted when the user asks to add the selected node.
    ///
    void addSelectedNodeRequested();

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

private:
    void setupDataView();
    void setupSubscriptionsView();
    void setupEventsView();
    void setupHistoryView();
    void configureToolbar();
    void rebuildSubscribeMenu();
    void applySubscriptionToSelection(const QString &subscriptionName);
    QModelIndexList selectedDataRows() const;

    Ui::DataAccessWidget *ui;
    DataAccessModel      *_dataModel;
    SubscriptionsModel   *_subscriptionsModel;
    EventsModel          *_eventsModel;
    HistoryModel         *_historyModel;
};
