// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dataaccesswidget.cpp
/// \brief Implements the OPC UA data access widget.
///

#include <QHeaderView>
#include <QItemSelectionModel>
#include <QMenu>
#include <QPushButton>

#include "dataaccessmodel.h"
#include "dataaccesswidget.h"
#include "eventsmodel.h"
#include "headerview.h"
#include "historymodel.h"
#include "subscriptiondelegate.h"
#include "subscriptionsmodel.h"
#include "tableview.h"
#include "ui_dataaccesswidget.h"

///
/// \brief DataAccessWidget::DataAccessWidget
/// \param parent
///
DataAccessWidget::DataAccessWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DataAccessWidget)
    , _dataModel(new DataAccessModel(this))
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
    connect(_subscriptionsModel, &QAbstractItemModel::rowsInserted,
            this, &DataAccessWidget::rebuildSubscribeMenu);
    connect(_subscriptionsModel, &QAbstractItemModel::rowsRemoved,
            this, &DataAccessWidget::rebuildSubscribeMenu);
}

///
/// \brief DataAccessWidget::~DataAccessWidget
///
DataAccessWidget::~DataAccessWidget()
{
    delete ui;
}

///
/// \brief DataAccessWidget::setCurrentPage
/// \param page
///
void DataAccessWidget::setCurrentPage(Page page)
{
    ui->mainTabs->setCurrentIndex(static_cast<int>(page));
}

///
/// \brief DataAccessWidget::addNode
/// \param details Variable node details.
///
void DataAccessWidget::addNode(const OpcUaNodeDetails &details)
{
    _dataModel->addOrUpdate(details);
    setCurrentPage(DataAccessPage);
}

///
/// \brief DataAccessWidget::updateValues
/// \param values Read results.
///
void DataAccessWidget::updateValues(const QVector<OpcUaDataValue> &values)
{
    _dataModel->updateValues(values);
}

///
/// \brief DataAccessWidget::clearRuntimeData
///
void DataAccessWidget::clearRuntimeData()
{
    _dataModel->clear();
    _subscriptionsModel->clear();
    _eventsModel->setItems({});
    _historyModel->setItems({});
}

///
/// \brief DataAccessWidget::setupDataView
///
void DataAccessWidget::setupDataView()
{
    ui->dataView->setModel(_dataModel);
    ui->dataView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->dataView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);
    ui->dataView->verticalHeader()->hide();


    auto *header = ui->dataView->headerView();
    connect(header, &HeaderView::sectionAlignmentChanged, this,
            [this](int logicalIndex, Qt::Alignment alignment) {
                _dataModel->setColumnAlignment(logicalIndex, alignment | Qt::AlignVCenter);
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
    ui->addNodeButton->setIcon(QStringLiteral("add.svg"));
    ui->removeButton->setIcon(QStringLiteral("remove.svg"));
    ui->readButton->setIcon(QStringLiteral("read.svg"));
    ui->writeButton->setIcon(QStringLiteral("write.svg"));
    ui->subscribeButton->setIcon(QStringLiteral("subscribe.svg"));

    ui->subscribeButton->setPopupMode(QToolButton::InstantPopup);
    ui->subscribeButton->setEnabled(false);
    ui->mainTabs->setTabEnabled(SubscriptionsPage, false);
    ui->mainTabs->setTabEnabled(EventsPage, false);
    ui->mainTabs->setTabEnabled(HistoryPage, false);

    connect(ui->addNodeButton, &QPushButton::clicked,
            this, &DataAccessWidget::addSelectedNodeRequested);
    connect(ui->removeButton, &QPushButton::clicked, this, [this]() {
        _dataModel->removeRows(selectedDataRows());
    });
    connect(ui->readButton, &QPushButton::clicked, this, [this]() {
        emit readRequested(_dataModel->nodeIds(selectedDataRows()));
    });
    connect(ui->writeButton, &QPushButton::clicked, this, [this]() {
        const QModelIndexList rows = selectedDataRows();
        if (rows.size() != 1)
            return;
        const DataAccessItem item = _dataModel->itemAt(rows.first().row());
        emit writeRequested(item.nodeId, item.typedValue, item.valueType,
                            item.dataTypeId, OpcUa::isWritable(item.userAccessLevel));
    });
}

///
/// \brief DataAccessWidget::rebuildSubscribeMenu
///
void DataAccessWidget::rebuildSubscribeMenu()
{
    const QStringList names = _subscriptionsModel->names();

    auto delegate = new SubscriptionDelegate(names, ui->dataView);
    ui->dataView->setItemDelegateForColumn(DataAccessModel::ColSubscription, delegate);

    QMenu *menu = new QMenu(ui->subscribeButton);
    for (const QString &name : names) {
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
        _dataModel->setData(_dataModel->index(idx.row(), DataAccessModel::ColSubscription),
                        subscriptionName, Qt::EditRole);
    }
}

///
/// \brief DataAccessWidget::selectedDataRows
/// \return Selected Data Access rows.
///
QModelIndexList DataAccessWidget::selectedDataRows() const
{
    return ui->dataView->selectionModel()->selectedRows();
}
