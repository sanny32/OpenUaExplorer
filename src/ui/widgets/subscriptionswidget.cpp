// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file subscriptionswidget.cpp
/// \brief Implements the OPC UA subscriptions management widget.
///

#include <algorithm>

#include <QAction>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QMenu>
#include <QPushButton>

#include "appicons.h"
#include "appsettings.h"
#include "headerview.h"
#include "models/subscriptionsmodel.h"
#include "publishingintervaldelegate.h"
#include "subscriptionswidget.h"
#include "tableview.h"
#include "ui_subscriptionswidget.h"

///
/// \brief Builds the subscriptions widget and its table view.
/// \param parent Parent widget.
///
SubscriptionsWidget::SubscriptionsWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SubscriptionsWidget)
    , _subscriptionsModel(new SubscriptionsModel(this))
{
    ui->setupUi(this);
    ui->addSubscriptionButton->setIcon(QStringLiteral("add"));
    ui->removeSubscriptionButton->setIcon(QStringLiteral("remove"));
    ui->removeSubscriptionButton->setEnabled(false);
    setupSubscriptionsView();

    connect(_subscriptionsModel, &QAbstractItemModel::rowsInserted,
            this, &SubscriptionsWidget::emitSubscriptionsChanged);
    connect(_subscriptionsModel, &QAbstractItemModel::rowsRemoved,
            this, &SubscriptionsWidget::emitSubscriptionsChanged);
    connect(_subscriptionsModel, &QAbstractItemModel::modelReset,
            this, &SubscriptionsWidget::emitSubscriptionsChanged);
    connect(_subscriptionsModel, &SubscriptionsModel::subscriptionRenamed,
            this, [this](const QString &oldName, const QString &newName) {
                emit subscriptionRenamed(oldName, newName);
                emitSubscriptionsChanged();
            });
    connect(_subscriptionsModel, &SubscriptionsModel::subscriptionIntervalChanged,
            this, [this](const QString &name, double interval) {
                emit subscriptionIntervalChanged(name, interval);
                emitSubscriptionsChanged();
            });

    reset();
}

///
/// \brief Destroys the widget and its generated UI.
///
SubscriptionsWidget::~SubscriptionsWidget()
{
    delete ui;
}

///
/// \brief Resets the list to the single built-in Default subscription.
///
void SubscriptionsWidget::reset()
{
    SubscriptionItem subscription;
    subscription.name = tr("Default");
    _subscriptionsModel->setItems({subscription});
}

///
/// \brief Returns the current subscriptions as a snapshot.
/// \return Subscriptions in row order.
///
QVector<SubscriptionItem> SubscriptionsWidget::subscriptions() const
{
    QVector<SubscriptionItem> items;
    items.reserve(_subscriptionsModel->rowCount());
    for (int row = 0; row < _subscriptionsModel->rowCount(); ++row)
        items.append(_subscriptionsModel->itemAt(row));
    return items;
}

///
/// \brief Persists the subscriptions table header state.
/// \param settings Settings store to write to.
///
void SubscriptionsWidget::saveViewState(AppSettings &settings) const
{
    settings.setViewState(ui->subscriptionsTable->objectName(),
                          ui->subscriptionsTable->headerView()->saveLayout());
}

///
/// \brief Restores the subscriptions table header state.
/// \param settings Settings store to read from.
///
void SubscriptionsWidget::restoreViewState(AppSettings &settings)
{
    ui->subscriptionsTable->headerView()->restoreLayout(
        settings.viewState(ui->subscriptionsTable->objectName()));
}

///
/// \brief Binds and lays out the subscriptions table.
///
void SubscriptionsWidget::setupSubscriptionsView()
{
    ui->subscriptionsTable->setModel(_subscriptionsModel);
    ui->subscriptionsTable->verticalHeader()->hide();
    ui->subscriptionsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->subscriptionsTable->setEditTriggers(QAbstractItemView::DoubleClicked
                                            | QAbstractItemView::SelectedClicked);
    ui->subscriptionsTable->setItemDelegateForColumn(
        SubscriptionsModel::ColPublishingInterval, new PublishingIntervalDelegate(this));
    ui->subscriptionsTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->subscriptionsTable, &QWidget::customContextMenuRequested,
            this, &SubscriptionsWidget::showSubscriptionsContextMenu);

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
            && _subscriptionsModel->itemAt(rows.first().row()).isDefault();
        ui->removeSubscriptionButton->setEnabled(!rows.isEmpty() && !defaultSelected);
    });

    connect(ui->addSubscriptionButton, &QPushButton::clicked,
            this, &SubscriptionsWidget::addSubscription);
    connect(ui->removeSubscriptionButton, &QPushButton::clicked,
            this, &SubscriptionsWidget::removeSelectedSubscriptions);
}

///
/// \brief Shows the subscriptions context menu mirroring the toolbar actions.
/// \param pos Cursor position in the subscriptions table's viewport coordinates.
///
void SubscriptionsWidget::showSubscriptionsContextMenu(const QPoint &pos)
{
    const QModelIndexList rows = ui->subscriptionsTable->selectionModel()->selectedRows();
    const bool defaultSelected = rows.size() == 1
        && _subscriptionsModel->itemAt(rows.first().row()).isDefault();

    QMenu menu(this);
    menu.addAction(AppIcons::themed(QStringLiteral("add")), tr("Add"),
                   this, &SubscriptionsWidget::addSubscription);

    QAction *removeAction = menu.addAction(AppIcons::themed(QStringLiteral("remove")), tr("Remove"),
                                           this, &SubscriptionsWidget::removeSelectedSubscriptions);
    removeAction->setEnabled(!rows.isEmpty() && !defaultSelected);

    QAction *removeAllAction = menu.addAction(AppIcons::themed(QStringLiteral("remove")), tr("Remove All"),
                                              this, &SubscriptionsWidget::removeAllSubscriptions);
    removeAllAction->setEnabled(_subscriptionsModel->rowCount() > 1);

    menu.exec(ui->subscriptionsTable->viewport()->mapToGlobal(pos));
}

///
/// \brief Adds a new subscription with a unique name and opens its name cell for editing.
///
void SubscriptionsWidget::addSubscription()
{
    QString name;
    for (int i = 1; ; ++i) {
        name = tr("Subscription %1").arg(i);
        if (!_subscriptionsModel->containsName(name))
            break;
    }

    SubscriptionItem subscription;
    subscription.id = _subscriptionsModel->rowCount();
    subscription.name = name;

    const int row = _subscriptionsModel->addSubscription(subscription);
    const QModelIndex nameIndex = _subscriptionsModel->index(row, SubscriptionsModel::ColName);
    ui->subscriptionsTable->setCurrentIndex(nameIndex);
    ui->subscriptionsTable->edit(nameIndex);
}

///
/// \brief Removes the selected subscriptions, unassigning and unmonitoring their nodes.
///
void SubscriptionsWidget::removeSelectedSubscriptions()
{
    QModelIndexList rows = ui->subscriptionsTable->selectionModel()->selectedRows();
    std::sort(rows.begin(), rows.end(), [](const QModelIndex &a, const QModelIndex &b) {
        return a.row() > b.row();
    });
    for (const QModelIndex &idx : rows)
        removeSubscriptionRow(idx.row());
}

///
/// \brief Removes every non-default subscription, unassigning and unmonitoring their nodes.
///
void SubscriptionsWidget::removeAllSubscriptions()
{
    for (int row = _subscriptionsModel->rowCount() - 1; row >= 0; --row)
        removeSubscriptionRow(row);
}

///
/// \brief Removes a single subscription row after announcing the removal.
/// \param row Subscription row to remove.
///
void SubscriptionsWidget::removeSubscriptionRow(int row)
{
    const SubscriptionItem subscription = _subscriptionsModel->itemAt(row);
    if (subscription.isDefault())
        return;
    emit subscriptionRemoved(subscription.name);
    _subscriptionsModel->removeRow(row);
}

///
/// \brief Emits the current subscriptions snapshot.
///
void SubscriptionsWidget::emitSubscriptionsChanged()
{
    emit subscriptionsChanged(subscriptions());
}
