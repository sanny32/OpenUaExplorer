// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dataaccesswidget.cpp
/// \brief Implements the OPC UA data access widget.
///

#include <algorithm>

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
#include "publishingintervaldelegate.h"
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
    connect(_subscriptionsModel, &SubscriptionsModel::subscriptionRenamed,
            this, [this](const QString &oldName, const QString &newName) {
                renameSubscriptionAssignments(oldName, newName);
                rebuildSubscribeMenu();
            });
    connect(_subscriptionsModel, &SubscriptionsModel::subscriptionIntervalChanged,
            this, &DataAccessWidget::reapplySubscriptionInterval);
    resetSubscriptions();
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
/// \brief Updates the subscription shown for a data-access node.
/// \param nodeId Affected node.
/// \param subscribed Whether the node belongs to the default subscription.
///
void DataAccessWidget::setNodeSubscribed(const QString &nodeId, bool subscribed)
{
    for (int row = 0; row < _dataModel->rowCount(); ++row) {
        if (_dataModel->itemAt(row).nodeId != nodeId)
            continue;
        const QModelIndex index = _dataModel->index(row, DataAccessModel::ColSubscription);
        if (subscribed) {
            if (_dataModel->itemAt(row).subscriptionName.isEmpty())
                _dataModel->setData(index, QStringLiteral("Default"), Qt::EditRole);
        } else {
            _dataModel->setData(index, QString(), Qt::EditRole);
        }
        break;
    }
}

///
/// \brief Clears the data, subscriptions, events, and history models.
///
void DataAccessWidget::clearRuntimeData()
{
    _dataModel->clear();
    resetSubscriptions();
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
    header->setSectionResizeMode(DataAccessModel::ColNodeId,       QHeaderView::Interactive);
    header->setSectionResizeMode(DataAccessModel::ColDisplayName,  QHeaderView::Interactive);
    header->setSectionResizeMode(DataAccessModel::ColValue,        QHeaderView::Interactive);
    header->setSectionResizeMode(DataAccessModel::ColDataType,     QHeaderView::Interactive);
    header->setSectionResizeMode(DataAccessModel::ColTimestamp,    QHeaderView::Interactive);
    header->setSectionResizeMode(DataAccessModel::ColStatus,       QHeaderView::Interactive);
    header->setSectionResizeMode(DataAccessModel::ColSubscription, QHeaderView::Interactive);

    header->setSectionAlignment(DataAccessModel::ColNumber,     Qt::AlignCenter);
    header->setSectionAlignment(DataAccessModel::ColValue,      Qt::AlignCenter);
    header->setSectionAlignment(DataAccessModel::ColDataType,   Qt::AlignCenter);
    header->setSectionAlignment(DataAccessModel::ColTimestamp,  Qt::AlignCenter);
    header->setSectionAlignment(DataAccessModel::ColStatus,     Qt::AlignCenter);

    ui->dataView->setColumnWidth(DataAccessModel::ColNumber,       36 );
    ui->dataView->setColumnWidth(DataAccessModel::ColNodeId,       220);
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
    ui->subscriptionsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->subscriptionsTable->setEditTriggers(QAbstractItemView::DoubleClicked
                                            | QAbstractItemView::SelectedClicked);
    ui->subscriptionsTable->setItemDelegateForColumn(
        SubscriptionsModel::ColPublishingInterval, new PublishingIntervalDelegate(this));

    auto *subsHeader = ui->subscriptionsTable->headerView();
    connect(subsHeader, &HeaderView::sectionAlignmentChanged, this,
            [this](int logicalIndex, Qt::Alignment alignment) {
                _subscriptionsModel->setColumnAlignment(logicalIndex, alignment | Qt::AlignVCenter);
            });
    subsHeader->setSectionResizeMode(SubscriptionsModel::ColName,               QHeaderView::Interactive);
    subsHeader->setSectionResizeMode(SubscriptionsModel::ColPublishingInterval, QHeaderView::Stretch);
    ui->subscriptionsTable->setColumnWidth(SubscriptionsModel::ColName, 120);

    connect(ui->subscriptionsTable->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this] {
        const QModelIndexList rows = ui->subscriptionsTable->selectionModel()->selectedRows();
        const bool defaultSelected = rows.size() == 1
            && _subscriptionsModel->itemAt(rows.first().row()).name == QStringLiteral("Default");
        ui->removeSubscriptionButton->setEnabled(!rows.isEmpty() && !defaultSelected);
    });
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
    ui->addSubscriptionButton->setIcon(QStringLiteral("add"));
    ui->removeSubscriptionButton->setIcon(QStringLiteral("remove"));

    ui->subscribeButton->setPopupMode(QToolButton::InstantPopup);
    ui->subscribeButton->setEnabled(false);
    ui->removeSubscriptionButton->setEnabled(false);
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
    connect(ui->addSubscriptionButton, &QPushButton::clicked,
            this, &DataAccessWidget::addSubscription);
    connect(ui->removeSubscriptionButton, &QPushButton::clicked,
            this, &DataAccessWidget::removeSelectedSubscriptions);
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
    const double interval = _subscriptionsModel->intervalFor(subscriptionName);
    for (const QModelIndex &idx : rows) {
        const QString nodeId = _dataModel->itemAt(idx.row()).nodeId;
        _dataModel->setData(_dataModel->index(idx.row(), DataAccessModel::ColSubscription),
                        subscriptionName, Qt::EditRole);
        if (subscriptionName.isEmpty())
            emit monitoringCancelled(nodeId);
        else
            emit monitoringRequested(nodeId, interval);
    }
}

///
/// \brief Resets the subscription list to the single built-in Default subscription.
///
void DataAccessWidget::resetSubscriptions()
{
    _subscriptionsModel->setItems({{QStringLiteral("Default"), 1000.0}});
}

///
/// \brief Adds a new subscription with a unique name and opens its name cell for editing.
///
void DataAccessWidget::addSubscription()
{
    QString name;
    for (int i = 1; ; ++i) {
        name = tr("Subscription %1").arg(i);
        if (!_subscriptionsModel->containsName(name))
            break;
    }

    const int row = _subscriptionsModel->addSubscription({name, 1000.0});
    const QModelIndex nameIndex = _subscriptionsModel->index(row, SubscriptionsModel::ColName);
    ui->mainTabs->setCurrentIndex(SubscriptionsPage);
    ui->subscriptionsTable->setCurrentIndex(nameIndex);
    ui->subscriptionsTable->edit(nameIndex);
}

///
/// \brief Removes the selected subscriptions, unassigning and unmonitoring their nodes.
///
void DataAccessWidget::removeSelectedSubscriptions()
{
    QModelIndexList rows = ui->subscriptionsTable->selectionModel()->selectedRows();
    std::sort(rows.begin(), rows.end(), [](const QModelIndex &a, const QModelIndex &b) {
        return a.row() > b.row();
    });
    for (const QModelIndex &idx : rows) {
        const QString name = _subscriptionsModel->itemAt(idx.row()).name;
        if (name == QStringLiteral("Default"))
            continue;
        for (int dataRow = 0; dataRow < _dataModel->rowCount(); ++dataRow) {
            if (_dataModel->itemAt(dataRow).subscriptionName != name)
                continue;
            const QString nodeId = _dataModel->itemAt(dataRow).nodeId;
            _dataModel->setData(_dataModel->index(dataRow, DataAccessModel::ColSubscription),
                                QString(), Qt::EditRole);
            emit monitoringCancelled(nodeId);
        }
        _subscriptionsModel->removeRow(idx.row());
    }
}

///
/// \brief Repoints data-access nodes from a renamed subscription to its new name.
/// \param oldName Previous subscription name.
/// \param newName New subscription name.
///
void DataAccessWidget::renameSubscriptionAssignments(const QString &oldName, const QString &newName)
{
    for (int row = 0; row < _dataModel->rowCount(); ++row) {
        if (_dataModel->itemAt(row).subscriptionName != oldName)
            continue;
        _dataModel->setData(_dataModel->index(row, DataAccessModel::ColSubscription),
                            newName, Qt::EditRole);
    }
}

///
/// \brief Re-establishes monitoring at a new interval for all nodes of a subscription.
/// \param name Subscription whose interval changed.
/// \param interval New publishing interval in milliseconds.
///
void DataAccessWidget::reapplySubscriptionInterval(const QString &name, double interval)
{
    for (int row = 0; row < _dataModel->rowCount(); ++row) {
        if (_dataModel->itemAt(row).subscriptionName != name)
            continue;
        emit monitoringRequested(_dataModel->itemAt(row).nodeId, interval);
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
