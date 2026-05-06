// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dataaccesswidget.cpp
/// \brief Implements the OPC UA data access widget.
///

#include <QHeaderView>
#include <QItemSelectionModel>
#include <QMenu>

#include "dataaccessmodel.h"
#include "dataaccesswidget.h"
#include "eventsmodel.h"
#include "headerview.h"
#include "historymodel.h"
#include "subscriptiondelegate.h"
#include "subscriptionsmodel.h"
#include "tableview.h"
#include "testdata.h"
#include "ui_dataaccesswidget.h"

///
/// \brief DataAccessWidget::DataAccessWidget
/// \param parent
///
DataAccessWidget::DataAccessWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DataAccessWidget)
    , _model(new DataAccessModel(this))
    , _subscriptionsModel(new SubscriptionsModel(this))
    , _eventsModel(new EventsModel(this))
    , _historyModel(new HistoryModel(this))
{
    ui->setupUi(this);
    configureToolbar();
    setupDataView();
    setupSubscriptionsView();
    setupEventsView();
    setupHistoryView();
    ui->dataView->setMinimumHeight(190);

    _subscriptionsModel->setItems(TestData::subscriptionItems());
    _eventsModel->setItems(TestData::eventItems());
    _historyModel->setItems(TestData::historyItems());
}

///
/// \brief DataAccessWidget::~DataAccessWidget
///
DataAccessWidget::~DataAccessWidget()
{
    delete ui;
}

///
/// \brief DataAccessWidget::setupDataView
///
void DataAccessWidget::setupDataView()
{
    ui->dataView->setModel(_model);
    ui->dataView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->dataView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);
    ui->dataView->verticalHeader()->hide();

    auto delegate = new SubscriptionDelegate(_model->subscriptionNames(), ui->dataView);
    ui->dataView->setItemDelegateForColumn(DataAccessModel::ColSubscription, delegate);

    auto *header = ui->dataView->headerView();
    connect(header, &HeaderView::sectionAlignmentChanged, this,
            [this](int logicalIndex, Qt::Alignment alignment) {
                _model->setColumnAlignment(logicalIndex, alignment | Qt::AlignVCenter);
            });

    header->setStretchLastSection(false);
    header->setSectionResizeMode(DataAccessModel::ColNumber,       QHeaderView::Fixed);
    header->setSectionResizeMode(DataAccessModel::ColNodeId,       QHeaderView::Stretch);
    header->setSectionResizeMode(DataAccessModel::ColDisplayName,  QHeaderView::Fixed);
    header->setSectionResizeMode(DataAccessModel::ColValue,        QHeaderView::Fixed);
    header->setSectionResizeMode(DataAccessModel::ColDataType,     QHeaderView::Fixed);
    header->setSectionResizeMode(DataAccessModel::ColTimestamp,    QHeaderView::Fixed);
    header->setSectionResizeMode(DataAccessModel::ColStatus,       QHeaderView::Fixed);
    header->setSectionResizeMode(DataAccessModel::ColSubscription, QHeaderView::Fixed);

    header->setSectionAlignment(DataAccessModel::ColNumber,     Qt::AlignCenter);
    header->setSectionAlignment(DataAccessModel::ColValue,      Qt::AlignCenter);
    header->setSectionAlignment(DataAccessModel::ColDataType,   Qt::AlignCenter);
    header->setSectionAlignment(DataAccessModel::ColTimestamp,  Qt::AlignCenter);
    header->setSectionAlignment(DataAccessModel::ColStatus,     Qt::AlignCenter);

    ui->dataView->setColumnWidth(DataAccessModel::ColNumber,       36 );
    ui->dataView->setColumnWidth(DataAccessModel::ColDisplayName,  120);
    ui->dataView->setColumnWidth(DataAccessModel::ColValue,        70 );
    ui->dataView->setColumnWidth(DataAccessModel::ColDataType,     82 );
    ui->dataView->setColumnWidth(DataAccessModel::ColTimestamp,    150);
    ui->dataView->setColumnWidth(DataAccessModel::ColStatus,       86 );
    ui->dataView->setColumnWidth(DataAccessModel::ColSubscription, 100);

    connect(ui->dataView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this] {
        ui->subscribeButton->setEnabled(
            !ui->dataView->selectionModel()->selectedRows().isEmpty());
    });
}

///
/// \brief DataAccessWidget::setupSubscriptionsView
///
void DataAccessWidget::setupSubscriptionsView()
{
    ui->subscriptionsTable->setModel(_subscriptionsModel);
    ui->subscriptionsTable->verticalHeader()->hide();

    auto *subsHeader = ui->subscriptionsTable->headerView();
    connect(subsHeader, &HeaderView::sectionAlignmentChanged, this,
            [this](int logicalIndex, Qt::Alignment alignment) {
                _subscriptionsModel->setColumnAlignment(logicalIndex, alignment | Qt::AlignVCenter);
            });
    subsHeader->setSectionResizeMode(SubscriptionsModel::ColName,               QHeaderView::Fixed);
    subsHeader->setSectionResizeMode(SubscriptionsModel::ColPublishingInterval, QHeaderView::Stretch);
    ui->subscriptionsTable->setColumnWidth(SubscriptionsModel::ColName, 120);
}

///
/// \brief DataAccessWidget::setupEventsView
///
void DataAccessWidget::setupEventsView()
{
    ui->eventsTable->setModel(_eventsModel);
    ui->eventsTable->verticalHeader()->hide();

    auto *eventsHeader = ui->eventsTable->headerView();
    connect(eventsHeader, &HeaderView::sectionAlignmentChanged, this,
            [this](int logicalIndex, Qt::Alignment alignment) {
                _eventsModel->setColumnAlignment(logicalIndex, alignment | Qt::AlignVCenter);
            });
    eventsHeader->setSectionResizeMode(EventsModel::ColTime,    QHeaderView::Fixed);
    eventsHeader->setSectionResizeMode(EventsModel::ColMessage, QHeaderView::Stretch);
    ui->eventsTable->setColumnWidth(EventsModel::ColTime, 95);
}

///
/// \brief DataAccessWidget::setupHistoryView
///
void DataAccessWidget::setupHistoryView()
{
    ui->historyTable->setModel(_historyModel);
    ui->historyTable->verticalHeader()->hide();

    auto *historyHeader = ui->historyTable->headerView();
    connect(historyHeader, &HeaderView::sectionAlignmentChanged, this,
            [this](int logicalIndex, Qt::Alignment alignment) {
                _historyModel->setColumnAlignment(logicalIndex, alignment | Qt::AlignVCenter);
            });
    historyHeader->setSectionResizeMode(HistoryModel::ColNode,  QHeaderView::Fixed);
    historyHeader->setSectionResizeMode(HistoryModel::ColRange, QHeaderView::Stretch);
    ui->historyTable->setColumnWidth(HistoryModel::ColNode, 260);
}

///
/// \brief DataAccessWidget::configureToolbar
///
void DataAccessWidget::configureToolbar()
{
    ui->addNodeButton->setIcon("add.svg");
    ui->removeButton->setIcon("remove.svg");
    ui->readButton->setIcon("read.svg");
    ui->writeButton->setIcon("write.svg");
    ui->subscribeButton->setIcon("subscribe.svg");

    ui->subscribeButton->setPopupMode(QToolButton::InstantPopup);
    ui->subscribeButton->setEnabled(false);

    QMenu *menu = new QMenu(ui->subscribeButton);
    for (const QString &name : _model->subscriptionNames()) {
        menu->addAction(name, this, [this, name] {
            applySubscriptionToSelection(name);
        });
    }
    menu->addSeparator();
    menu->addAction(tr("Unsubscribe"), this, [this] {
        applySubscriptionToSelection(QString());
    });
    ui->subscribeButton->setMenu(menu);
}

///
/// \brief DataAccessWidget::applySubscriptionToSelection
/// \param subscriptionName
///
void DataAccessWidget::applySubscriptionToSelection(const QString &subscriptionName)
{
    const QModelIndexList rows = ui->dataView->selectionModel()->selectedRows();
    for (const QModelIndex &idx : rows) {
        _model->setData(_model->index(idx.row(), DataAccessModel::ColSubscription),
                        subscriptionName, Qt::EditRole);
    }
}

