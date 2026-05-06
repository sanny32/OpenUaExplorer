// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dataaccesswidget.h
/// \brief Declares the OPC UA data access widget.
///

#pragma once

#include <QWidget>

#include "itestdatapopulatable.h"

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
    explicit DataAccessWidget(QWidget *parent = nullptr);
    ~DataAccessWidget() override;

    void populateWithTestData() override;

private:
    void setupDataView();
    void setupSubscriptionsView();
    void setupEventsView();
    void setupHistoryView();
    void configureToolbar();
    void rebuildSubscribeMenu();
    void applySubscriptionToSelection(const QString &subscriptionName);

    Ui::DataAccessWidget *ui;
    DataAccessModel      *_dataModel;
    SubscriptionsModel   *_subscriptionsModel;
    EventsModel          *_eventsModel;
    HistoryModel         *_historyModel;
};
