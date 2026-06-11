// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dataaccesswidget.h
/// \brief Declares the OPC UA data access widget.
///

#pragma once

#include <QModelIndex>
#include <QWidget>

#include "itestdatapopulatable.h"
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
class DataAccessWidget : public QWidget, public ITestDataPopulatable
{
    Q_OBJECT

public:
    enum Page {
        DataAccessPage = 0,
        SubscriptionsPage,
        EventsPage,
        HistoryPage
    };

    explicit DataAccessWidget(QWidget *parent = nullptr);
    ~DataAccessWidget() override;

    void setCurrentPage(Page page);
    void populateWithTestData() override;
    void addNode(const OpcUaNodeDetails &details);
    void updateValues(const QVector<OpcUaDataValue> &values);
    void clearRuntimeData();

signals:
    void addSelectedNodeRequested();
    void readRequested(QStringList nodeIds);
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
