#pragma once

#include <QWidget>

namespace Ui {
class DataAccessWidget;
}

class DataAccessModel;
class SubscriptionsModel;
class EventsModel;
class HistoryModel;

class DataAccessWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DataAccessWidget(QWidget *parent = nullptr);
    ~DataAccessWidget() override;

private:
    void setupDataView();
    void setupSubscriptionsView();
    void setupEventsView();
    void setupHistoryView();
    void configureToolbar();
    void applySubscriptionToSelection(const QString &subscriptionName);

    Ui::DataAccessWidget *ui;
    DataAccessModel      *_model;
    SubscriptionsModel   *_subscriptionsModel;
    EventsModel          *_eventsModel;
    HistoryModel         *_historyModel;
};
