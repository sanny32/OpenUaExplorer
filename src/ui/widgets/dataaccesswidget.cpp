// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file dataaccesswidget.cpp
/// \brief Implements the OPC UA data access tab widget.
///

#include <QAction>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QMenu>
#include <QMimeData>
#include <QPushButton>

#include "appicons.h"
#include "appsettings.h"
#include "dataaccesswidget.h"
#include "dialogs/messageboxdialog.h"
#include "dialogs/newsubscriptiondialog.h"
#include "fileexport.h"
#include "headerview.h"
#include "models/addressspacemimedata.h"
#include "models/dataaccessmodel.h"
#include "subscriptiondelegate.h"
#include "tableview.h"
#include "tableviewconfig.h"
#include "ui_dataaccesswidget.h"

namespace {

///
/// \brief Reads a dropped variable node from address-space MIME data.
/// \param mimeData MIME data to read.
/// \param node Destination for the decoded node.
/// \return True when the data contains a variable node with a NodeId.
///
bool decodeDroppedVariable(const QMimeData *mimeData, OpcUaNodeInfo *node)
{
    if (!AddressSpaceMime::decodeNode(mimeData, node))
        return false;
    return OpcUa::isVariable(node->nodeClass) && !node->nodeId.isEmpty();
}

} // namespace

///
/// \brief Builds the widget and its data view.
/// \param parent Parent widget.
///
DataAccessWidget::DataAccessWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DataAccessWidget)
    , _dataModel(new DataAccessModel(this))
{
    ui->setupUi(this);

    SubscriptionItem defaultSubscription;
    defaultSubscription.name = tr("Default");
    _subscriptions = {defaultSubscription};

    configureToolbar();
    setupDataView();
    rebuildSubscribeMenu();
}

///
/// \brief Destroys the widget and its generated UI.
///
DataAccessWidget::~DataAccessWidget()
{
    delete ui;
}

///
/// \brief Adds or updates a node row.
/// \param details Variable node details.
///
void DataAccessWidget::addNode(const OpcUaNodeDetails &details)
{
    _dataModel->addOrUpdate(details);
}

///
/// \brief Adds or updates a node row and assigns it to the Default subscription.
/// \param details Variable node details.
/// \param subscription Subscription to assign.
///
void DataAccessWidget::addNodeWithDefaultSubscription(
    const OpcUaNodeDetails &details,
    const SubscriptionItem &subscription)
{
    addNode(details);

    SubscriptionItem effectiveSubscription = subscription;
    if (effectiveSubscription.isDefault() && effectiveSubscription.name.isEmpty())
        effectiveSubscription = defaultSubscription();

    for (int row = 0; row < _dataModel->rowCount(); ++row) {
        if (_dataModel->itemAt(row).nodeId != details.nodeId)
            continue;
        _dataModel->setData(_dataModel->index(row, DataAccessModel::ColSubscription),
                            effectiveSubscription.name, Qt::EditRole);
        emit monitoringRequested(details.nodeId, effectiveSubscription.publishingInterval);
        return;
    }
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
                _dataModel->setData(index, defaultSubscription().name, Qt::EditRole);
        } else {
            _dataModel->setData(index, QString(), Qt::EditRole);
        }
        break;
    }
}

///
/// \brief Reports whether any data-access row is selected.
/// \return True when at least one row is selected.
///
bool DataAccessWidget::hasSelection() const
{
    return !ui->dataView->selectionModel()->selectedRows().isEmpty();
}

///
/// \brief Removes all data-access rows.
///
void DataAccessWidget::clear()
{
    _dataModel->clear();
}

///
/// \brief Reports whether the data-access table has any rows.
/// \return True when at least one node is listed.
///
bool DataAccessWidget::hasData() const
{
    return _dataModel->rowCount() > 0;
}

///
/// \brief Prompts for a file and exports the data-access rows as CSV.
///
void DataAccessWidget::exportToCsv()
{
    if (_dataModel->rowCount() == 0) {
        MessageBoxDialog::information(this, tr("Export Data"),
                                      tr("There are no data-access rows to export."));
        return;
    }

    FileExport::exportModelToCsv(this, tr("Export Data"), tr("data-access.csv"), *_dataModel);
}

///
/// \brief Returns the listed nodes paired with their subscription assignment.
/// \return NodeId and subscription-name pairs in row order.
///
QVector<QPair<QString, QString>> DataAccessWidget::monitoredNodes() const
{
    QVector<QPair<QString, QString>> nodes;
    nodes.reserve(_dataModel->rowCount());
    for (int row = 0; row < _dataModel->rowCount(); ++row) {
        const DataAccessItem item = _dataModel->itemAt(row);
        nodes.append({item.nodeId, item.subscriptionName});
    }
    return nodes;
}

///
/// \brief Persists the data view header state.
/// \param settings Settings store to write to.
///
void DataAccessWidget::saveViewState(AppSettings &settings) const
{
    settings.setViewState(ui->dataView->objectName(), ui->dataView->headerView()->saveLayout());
}

///
/// \brief Restores the data view header state.
/// \param settings Settings store to read from.
///
void DataAccessWidget::restoreViewState(AppSettings &settings)
{
    ui->dataView->headerView()->restoreLayout(settings.viewState(ui->dataView->objectName()));
}

///
/// \brief Applies the OPC UA timestamp display mode to the data-access table.
/// \param mode Local time or UTC.
///
void DataAccessWidget::setTimestampMode(AppSettings::TimestampMode mode)
{
    _dataModel->setTimestampMode(mode);
}

///
/// \brief Replaces the known subscriptions and rebuilds the subscribe menu.
/// \param subscriptions Current subscriptions.
///
void DataAccessWidget::setSubscriptions(const QVector<SubscriptionItem> &subscriptions)
{
    _subscriptions = subscriptions;
    if (_subscriptionDelegate) {
        _subscriptionDelegate->setSubscriptions(_subscriptions);
        ui->dataView->viewport()->update();
    }
    rebuildSubscribeMenu();
}

///
/// \brief Repoints data-access nodes from a renamed subscription to its new name.
/// \param oldName Previous subscription name.
/// \param newName New subscription name.
///
void DataAccessWidget::applySubscriptionRename(const QString &oldName, const QString &newName)
{
    for (int row = 0; row < _dataModel->rowCount(); ++row) {
        if (_dataModel->itemAt(row).subscriptionName != oldName)
            continue;
        _dataModel->setData(_dataModel->index(row, DataAccessModel::ColSubscription),
                            newName, Qt::EditRole);
    }
}

///
/// \brief Re-establishes monitoring at a new interval for a subscription's nodes.
/// \param name Subscription whose interval changed.
/// \param interval New publishing interval in milliseconds.
///
void DataAccessWidget::applySubscriptionInterval(const QString &name, double interval)
{
    for (int row = 0; row < _dataModel->rowCount(); ++row) {
        if (_dataModel->itemAt(row).subscriptionName != name)
            continue;
        emit monitoringRequested(_dataModel->itemAt(row).nodeId, interval);
    }
}

///
/// \brief Unassigns and stops monitoring nodes of a removed subscription.
/// \param name Subscription being removed.
///
void DataAccessWidget::applySubscriptionRemoval(const QString &name)
{
    for (int row = 0; row < _dataModel->rowCount(); ++row) {
        if (_dataModel->itemAt(row).subscriptionName != name)
            continue;
        const QString nodeId = _dataModel->itemAt(row).nodeId;
        _dataModel->setData(_dataModel->index(row, DataAccessModel::ColSubscription),
                            QString(), Qt::EditRole);
        emit monitoringCancelled(nodeId);
    }
}

///
/// \brief Handles address-space node drag/drop events on the data table.
/// \param watched Object receiving the event.
/// \param event Event to filter.
/// \return True when the event was consumed.
///
bool DataAccessWidget::eventFilter(QObject *watched, QEvent *event)
{
    const bool dataViewTarget = watched == ui->dataView || watched == ui->dataView->viewport();
    if (!dataViewTarget)
        return QWidget::eventFilter(watched, event);

    if (event->type() == QEvent::DragEnter || event->type() == QEvent::DragMove) {
        auto *dragEvent = static_cast<QDragMoveEvent *>(event);
        OpcUaNodeInfo node;
        if (decodeDroppedVariable(dragEvent->mimeData(), &node)) {
            dragEvent->setDropAction(Qt::CopyAction);
            dragEvent->accept();
            return true;
        }
        dragEvent->ignore();
        return false;
    }

    if (event->type() == QEvent::Drop) {
        auto *dropEvent = static_cast<QDropEvent *>(event);
        OpcUaNodeInfo node;
        if (decodeDroppedVariable(dropEvent->mimeData(), &node)) {
            emit nodeDropRequested(node.nodeId);
            dropEvent->setDropAction(Qt::CopyAction);
            dropEvent->accept();
            return true;
        }
        dropEvent->ignore();
        return false;
    }

    return QWidget::eventFilter(watched, event);
}

///
/// \brief Binds and lays out the data table, including column sizing and selection wiring.
///
void DataAccessWidget::setupDataView()
{
    ui->dataView->setModel(_dataModel);

    const auto emitNodeCount = [this] { emit nodeCountChanged(_dataModel->rowCount()); };
    connect(_dataModel, &QAbstractItemModel::rowsInserted, this, emitNodeCount);
    connect(_dataModel, &QAbstractItemModel::rowsRemoved, this, emitNodeCount);
    connect(_dataModel, &QAbstractItemModel::modelReset, this, emitNodeCount);

    _subscriptionDelegate = new SubscriptionDelegate(_subscriptions, ui->dataView);
    ui->dataView->setItemDelegateForColumn(DataAccessModel::ColSubscription, _subscriptionDelegate);
    connect(_subscriptionDelegate, &SubscriptionDelegate::subscriptionChanged, this,
            [this](const QModelIndex &index, const QString &subscriptionName) {
                const QString nodeId = _dataModel->itemAt(index.row()).nodeId;
                if (subscriptionName.isEmpty())
                    emit monitoringCancelled(nodeId);
                else
                    emit monitoringRequested(nodeId, intervalFor(subscriptionName));
            });
    connect(_subscriptionDelegate, &SubscriptionDelegate::newSubscriptionRequested, this,
            [this](const QModelIndex &index) {
                const QString nodeId = _dataModel->itemAt(index.row()).nodeId;
                QMetaObject::invokeMethod(this, [this, nodeId] {
                    promptNewSubscription(nodeId);
                }, Qt::QueuedConnection);
            });

    ui->dataView->setAcceptDrops(true);
    ui->dataView->viewport()->setAcceptDrops(true);
    ui->dataView->setDropIndicatorShown(true);
    ui->dataView->installEventFilter(this);
    ui->dataView->viewport()->installEventFilter(this);
    ui->dataView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->dataView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);
    ui->dataView->verticalHeader()->hide();
    ui->dataView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->dataView, &QWidget::customContextMenuRequested,
            this, &DataAccessWidget::showDataContextMenu);

    TableViewConfig::apply(ui->dataView,
        {
            {DataAccessModel::ColNumber, QHeaderView::Fixed, 36, Qt::AlignCenter},
            {DataAccessModel::ColNodeId, QHeaderView::Interactive, 220},
            {DataAccessModel::ColDisplayName, QHeaderView::Interactive, 120},
            {DataAccessModel::ColValue, QHeaderView::Interactive, 70, Qt::AlignCenter},
            {DataAccessModel::ColDataType, QHeaderView::Interactive, 82, Qt::AlignCenter},
            {DataAccessModel::ColTimestamp, QHeaderView::Interactive, 150, Qt::AlignCenter},
            {DataAccessModel::ColStatus, QHeaderView::Interactive, 86, Qt::AlignCenter},
            {DataAccessModel::ColSubscription, QHeaderView::Interactive, 100},
        },
        [this](int logicalIndex, Qt::Alignment alignment) {
            _dataModel->setColumnAlignment(logicalIndex, alignment);
        });

    connect(ui->dataView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this] {
        const int selectedCount = ui->dataView->selectionModel()->selectedRows().size();
        const bool hasSelection = selectedCount > 0;
        ui->removeButton->setEnabled(hasSelection);
        ui->readButton->setEnabled(hasSelection);
        ui->writeButton->setEnabled(selectedCount == 1);
        ui->subscribeButton->setEnabled(hasSelection);
        emit selectionChanged();
    });
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
    ui->removeButton->setEnabled(false);
    ui->readButton->setEnabled(false);
    ui->writeButton->setEnabled(false);
    ui->subscribeButton->setEnabled(false);

    connect(ui->addNodeButton, &QPushButton::clicked,
            this, &DataAccessWidget::addSelectedNodeRequested);
    connect(ui->removeButton, &QPushButton::clicked,
            this, &DataAccessWidget::removeSelectedNodes);
    connect(ui->readButton, &QPushButton::clicked,
            this, &DataAccessWidget::readSelectedNodes);
    connect(ui->writeButton, &QPushButton::clicked,
            this, &DataAccessWidget::writeSelectedNode);
}

///
/// \brief Shows the data-access context menu mirroring the toolbar actions.
/// \param pos Cursor position in the data view's viewport coordinates.
///
void DataAccessWidget::showDataContextMenu(const QPoint &pos)
{
    const int selectedCount = ui->dataView->selectionModel()->selectedRows().size();
    const bool hasSelection = selectedCount > 0;

    QMenu menu(this);
    menu.addAction(AppIcons::themed(QStringLiteral("add")), tr("Add Node"),
                   this, &DataAccessWidget::addSelectedNodeRequested);

    QAction *removeAction = menu.addAction(AppIcons::themed(QStringLiteral("remove")), tr("Remove"),
                                           this, &DataAccessWidget::removeSelectedNodes);
    removeAction->setEnabled(hasSelection);

    QAction *removeAllAction = menu.addAction(AppIcons::themed(QStringLiteral("remove")), tr("Clear"),
                                              this, &DataAccessWidget::removeAllNodes);
    removeAllAction->setEnabled(_dataModel->rowCount() > 0);

    menu.addSeparator();

    QAction *readAction = menu.addAction(AppIcons::themed(QStringLiteral("read")), tr("Read"),
                                         this, &DataAccessWidget::readSelectedNodes);
    readAction->setEnabled(hasSelection);

    QAction *writeAction = menu.addAction(AppIcons::themed(QStringLiteral("write")), tr("Write"),
                                          this, &DataAccessWidget::writeSelectedNode);
    writeAction->setEnabled(selectedCount == 1);

    QMenu *subscribeMenu = menu.addMenu(AppIcons::themed(QStringLiteral("subscribe")), tr("Subscribe"));
    populateSubscribeMenu(subscribeMenu);
    subscribeMenu->menuAction()->setEnabled(hasSelection);

    menu.exec(ui->dataView->viewport()->mapToGlobal(pos));
}

///
/// \brief Removes the selected data-access nodes, cancelling monitoring for subscribed ones.
///
void DataAccessWidget::removeSelectedNodes()
{
    const QModelIndexList rows = selectedDataRows();
    for (const QModelIndex &idx : rows) {
        const DataAccessItem item = _dataModel->itemAt(idx.row());
        if (!item.subscriptionName.isEmpty())
            emit monitoringCancelled(item.nodeId);
    }
    _dataModel->removeRows(rows);
}

///
/// \brief Requests a read of the selected data-access nodes.
///
void DataAccessWidget::readSelectedNodes()
{
    emit readRequested(_dataModel->nodeIds(selectedDataRows()));
}

///
/// \brief Requests a value write for the single selected data-access node.
///
void DataAccessWidget::writeSelectedNode()
{
    const QModelIndexList rows = selectedDataRows();
    if (rows.size() != 1)
        return;
    const DataAccessItem item = _dataModel->itemAt(rows.first().row());
    emit writeRequested(item.nodeId, item.typedValue, item.valueType,
                        item.dataTypeId, OpcUa::isWritable(item.userAccessLevel));
}

///
/// \brief Removes every data-access node, cancelling monitoring for subscribed nodes.
///
void DataAccessWidget::removeAllNodes()
{
    for (int row = 0; row < _dataModel->rowCount(); ++row) {
        const DataAccessItem item = _dataModel->itemAt(row);
        if (!item.subscriptionName.isEmpty())
            emit monitoringCancelled(item.nodeId);
    }
    _dataModel->clear();
}

///
/// \brief Rebuilds the subscribe button's menu from the current subscriptions.
///
void DataAccessWidget::rebuildSubscribeMenu()
{
    QMenu *menu = new QMenu(ui->subscribeButton);
    populateSubscribeMenu(menu);
    ui->subscribeButton->setMenu(menu);
}

///
/// \brief Fills a menu with one action per subscription plus an Unsubscribe entry.
/// \param menu Menu to populate.
///
void DataAccessWidget::populateSubscribeMenu(QMenu *menu)
{
    const QStringList names = subscriptionNames();
    for (const QString &name : names) {
        menu->addAction(name, this, [this, name] {
            applySubscriptionToSelection(name);
        });
    }
    menu->addSeparator();
    menu->addAction(tr("Unsubscribe"), this, [this] {
        applySubscriptionToSelection(QString());
    });
}

///
/// \brief Assigns a subscription (or clears it) on every selected data row.
/// \param subscriptionName Subscription to assign, or empty to unsubscribe.
///
void DataAccessWidget::applySubscriptionToSelection(const QString &subscriptionName)
{
    const QModelIndexList rows = ui->dataView->selectionModel()->selectedRows();
    const double interval = intervalFor(subscriptionName);
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
/// \brief Prompts for a new subscription and assigns it to the selected data rows.
///
void DataAccessWidget::promptSubscriptionForSelection()
{
    if (!hasSelection())
        return;

    NewSubscriptionDialog dialog(subscriptionNames(), this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    const QString name = dialog.subscriptionName();
    emit subscriptionCreationRequested(name, dialog.publishingInterval());
    applySubscriptionToSelection(name);
}

///
/// \brief Prompts for a new subscription and assigns the edited node to it.
/// \param nodeId Node whose Subscription cell triggered the request.
///
void DataAccessWidget::promptNewSubscription(const QString &nodeId)
{
    int row = -1;
    for (int r = 0; r < _dataModel->rowCount(); ++r) {
        if (_dataModel->itemAt(r).nodeId == nodeId) {
            row = r;
            break;
        }
    }
    if (row < 0)
        return;

    const QModelIndex index = _dataModel->index(row, DataAccessModel::ColSubscription);
    const QString previous = _dataModel->itemAt(row).subscriptionName;
    _dataModel->setData(index, SubscriptionDelegate::createNewLabel(), Qt::EditRole);

    NewSubscriptionDialog dialog(subscriptionNames(), this);
    if (dialog.exec() != QDialog::Accepted) {
        _dataModel->setData(index, previous, Qt::EditRole);
        return;
    }

    const QString name = dialog.subscriptionName();
    const double interval = dialog.publishingInterval();
    emit subscriptionCreationRequested(name, interval);
    _dataModel->setData(index, name, Qt::EditRole);
    emit monitoringRequested(nodeId, interval);
}

///
/// \brief Returns the known subscription names in order.
/// \return Subscription names.
///
QStringList DataAccessWidget::subscriptionNames() const
{
    QStringList names;
    names.reserve(_subscriptions.size());
    for (const SubscriptionItem &item : _subscriptions)
        names.append(item.name);
    return names;
}

///
/// \brief Returns the publishing interval of a named subscription.
/// \param name Subscription name.
/// \return Publishing interval in milliseconds, or the default when not found.
///
double DataAccessWidget::intervalFor(const QString &name) const
{
    for (const SubscriptionItem &item : _subscriptions) {
        if (item.name == name)
            return item.publishingInterval;
    }
    return SubscriptionItem().publishingInterval;
}

///
/// \brief Returns the Default subscription, using a fallback label when needed.
/// \return Default subscription item.
///
SubscriptionItem DataAccessWidget::defaultSubscription() const
{
    for (SubscriptionItem item : _subscriptions) {
        if (!item.isDefault())
            continue;
        if (item.name.isEmpty())
            item.name = tr("Default");
        return item;
    }

    SubscriptionItem item;
    item.name = tr("Default");
    return item;
}

///
/// \brief Returns the currently selected data rows.
/// \return Selected Data Access rows.
///
QModelIndexList DataAccessWidget::selectedDataRows() const
{
    return ui->dataView->selectionModel()->selectedRows();
}
