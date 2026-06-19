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

#include "appsettings.h"
#include "dataaccesswidget.h"
#include "headerview.h"
#include "models/dataaccessmodel.h"
#include "models/eventsmodel.h"
#include "models/historymodel.h"
#include "models/subscriptionsmodel.h"
#include "subscriptiondelegate.h"
#include "tableview.h"
#include "ui_dataaccesswidget.h"

///
/// \brief Builds the widget and its data, subscriptions, events, and history views.
/// \param parent Parent widget.
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
/// \brief Destroys the widget and its generated UI.
///
DataAccessWidget::~DataAccessWidget()
{
    delete ui;
}

///
/// \brief Switches the visible tab.
/// \param page Page to show.
///
void DataAccessWidget::setCurrentPage(Page page)
{
    ui->mainTabs->setCurrentIndex(static_cast<int>(page));
}

///
/// \brief Returns the currently visible tab index.
/// \return Index of the active page.
///
int DataAccessWidget::currentPage() const
{
    return ui->mainTabs->currentIndex();
}

///
/// \brief Persists the header state of the data, subscriptions, events, and history views.
/// \param settings Settings store to write to.
///
void DataAccessWidget::saveViewState(AppSettings &settings) const
{
    settings.setViewState(ui->dataView->objectName(), ui->dataView->headerView()->saveLayout());
    settings.setViewState(ui->subscriptionsTable->objectName(),
                          ui->subscriptionsTable->headerView()->saveLayout());
    settings.setViewState(ui->eventsTable->objectName(), ui->eventsTable->headerView()->saveLayout());
    settings.setViewState(ui->historyTable->objectName(), ui->historyTable->headerView()->saveLayout());
}

///
/// \brief Restores the header state of the data, subscriptions, events, and history views.
/// \param settings Settings store to read from.
///
void DataAccessWidget::restoreViewState(AppSettings &settings)
{
    ui->dataView->headerView()->restoreLayout(settings.viewState(ui->dataView->objectName()));
    ui->subscriptionsTable->headerView()->restoreLayout(
        settings.viewState(ui->subscriptionsTable->objectName()));
    ui->eventsTable->headerView()->restoreLayout(settings.viewState(ui->eventsTable->objectName()));
    ui->historyTable->headerView()->restoreLayout(settings.viewState(ui->historyTable->objectName()));
}

///
/// \brief Adds or updates a node row and shows the Data Access page.
/// \param details Variable node details.
///
void DataAccessWidget::addNode(const OpcUaNodeDetails &details)
{
    _dataModel->addOrUpdate(details);
    setCurrentPage(DataAccessPage);
}

///
/// \brief Applies read results to the data rows.
/// \param values Read results.
///
void DataAccessWidget::updateValues(const QVector<OpcUaDataValue> &values)
{
    _dataModel->updateValues(values);
}

///
/// \brief Clears the data, subscriptions, events, and history models.
///
void DataAccessWidget::clearRuntimeData()
{
    _dataModel->clear();
    _subscriptionsModel->clear();
    _eventsModel->setItems({});
    _historyModel->setItems({});
}

///
/// \brief Binds and lays out the data table, including column sizing and selection wiring.
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
/// \brief Binds and lays out the subscriptions table.
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
/// \brief Binds and lays out the events table.
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
/// \brief Binds and lays out the history table.
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
/// \brief Sets toolbar icons, disables empty-state actions, and wires the toolbar buttons.
///
void DataAccessWidget::configureToolbar()
{
    ui->addNodeButton->setIcon(QStringLiteral("add"));
    ui->removeButton->setIcon(QStringLiteral("remove"));
    ui->readButton->setIcon(QStringLiteral("read"));
    ui->writeButton->setIcon(QStringLiteral("write"));
    ui->subscribeButton->setIcon(QStringLiteral("subscribe"));

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
/// \brief Rebuilds the subscription editor delegate and the subscribe button's menu.
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
/// \brief Assigns a subscription (or clears it) on every selected data row.
/// \param subscriptionName Subscription to assign, or empty to unsubscribe.
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
/// \brief Returns the currently selected data rows.
/// \return Selected Data Access rows.
///
QModelIndexList DataAccessWidget::selectedDataRows() const
{
    return ui->dataView->selectionModel()->selectedRows();
}
