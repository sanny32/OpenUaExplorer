// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file subscriptionswidget.cpp
/// \brief Implements the OPC UA subscriptions management widget.
///

#include <algorithm>

#include <QAction>
#include <QBrush>
#include <QColor>
#include <QEvent>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QMenu>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QPushButton>

#include "appicons.h"
#include "appsettings.h"
#include "headerview.h"
#include "models/subscriptionsmodel.h"
#include "publishingintervaldelegate.h"
#include "subscriptionswidget.h"
#include "tableview.h"
#include "ui_subscriptionswidget.h"

namespace {

/// \brief Edge length of the built-in lock glyph in device-independent pixels.
constexpr int lockGlyphSize = 16;

/// \brief Transparent left padding baked before the lock glyph, in device-independent pixels.
constexpr int lockLeftPadding = 8;

/// \brief Normal (non-built-in) row canvas colours, matching the qlementine item background.
constexpr QRgb lightCanvas = 0xffffff;
constexpr QRgb darkCanvas = 0x1e1f24;

///
/// \brief Derives the faint shade that distinguishes built-in rows from the row canvas.
/// \param dark Whether the dark colour scheme is active.
/// \return A barely darker shade on light themes, a barely lighter one on dark themes.
///
QColor builtinShade(bool dark)
{
    return dark ? QColor(darkCanvas).lighter(118) : QColor(lightCanvas).darker(104);
}

} // namespace

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
    applyBuiltinDecoration();

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
/// \brief Resets the list to the built-in Default, Fast, and Slow subscriptions.
///
void SubscriptionsWidget::reset()
{
    SubscriptionItem defaultSubscription;
    defaultSubscription.name = tr("Default");
    defaultSubscription.builtin = true;

    SubscriptionItem fastSubscription;
    fastSubscription.name = tr("Fast");
    fastSubscription.publishingInterval = 250.0;
    fastSubscription.id = 1;
    fastSubscription.builtin = true;

    SubscriptionItem slowSubscription;
    slowSubscription.name = tr("Slow");
    slowSubscription.publishingInterval = 5000.0;
    slowSubscription.id = 2;
    slowSubscription.builtin = true;

    _subscriptionsModel->setItems({defaultSubscription, fastSubscription, slowSubscription});
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
/// \brief Persists the user-created subscriptions.
/// \param settings Settings store to write to.
///
void SubscriptionsWidget::saveSubscriptions(AppSettings &settings) const
{
    settings.setCustomSubscriptions(subscriptions());
}

///
/// \brief Restores the user-created subscriptions saved from the last session.
/// \param settings Settings store to read from.
///
void SubscriptionsWidget::loadSubscriptions(AppSettings &settings)
{
    const QVector<SubscriptionItem> stored = settings.customSubscriptions();
    for (const SubscriptionItem &item : stored)
        createSubscription(item.name, item.publishingInterval);
}

///
/// \brief Binds and lays out the subscriptions table.
///
void SubscriptionsWidget::setupSubscriptionsView()
{
    ui->subscriptionsTable->setModel(_subscriptionsModel);
    ui->subscriptionsTable->setIconSize(QSize(lockGlyphSize + lockLeftPadding, lockGlyphSize));
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
        const bool builtinSelected = rows.size() == 1
            && _subscriptionsModel->itemAt(rows.first().row()).isBuiltin();
        ui->removeSubscriptionButton->setEnabled(!rows.isEmpty() && !builtinSelected);
    });

    connect(ui->addSubscriptionButton, &QPushButton::clicked,
            this, &SubscriptionsWidget::addSubscription);
    connect(ui->removeSubscriptionButton, &QPushButton::clicked,
            this, &SubscriptionsWidget::removeSelectedSubscriptions);
}

///
/// \brief Applies the lock icon and shaded background used for built-in subscriptions.
///
void SubscriptionsWidget::applyBuiltinDecoration()
{
    const qreal dpr = devicePixelRatioF();
    QPixmap canvas(QSize(lockGlyphSize + lockLeftPadding, lockGlyphSize) * dpr);
    canvas.setDevicePixelRatio(dpr);
    canvas.fill(Qt::transparent);
    QPainter painter(&canvas);
    painter.drawPixmap(QPoint(lockLeftPadding, 0),
                       AppIcons::themed(QStringLiteral("lock"))
                           .pixmap(QSize(lockGlyphSize, lockGlyphSize), dpr));
    painter.end();
    _subscriptionsModel->setBuiltinIcon(QIcon(canvas));

    _subscriptionsModel->setBuiltinBackground(builtinShade(AppIcons::isDarkTheme()));
}

///
/// \brief Re-applies the built-in row styling when the palette changes.
/// \param event Change event being handled.
///
void SubscriptionsWidget::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::PaletteChange)
        applyBuiltinDecoration();
}

///
/// \brief Shows the subscriptions context menu mirroring the toolbar actions.
/// \param pos Cursor position in the subscriptions table's viewport coordinates.
///
void SubscriptionsWidget::showSubscriptionsContextMenu(const QPoint &pos)
{
    const QModelIndexList rows = ui->subscriptionsTable->selectionModel()->selectedRows();
    const bool builtinSelected = rows.size() == 1
        && _subscriptionsModel->itemAt(rows.first().row()).isBuiltin();

    QMenu menu(this);
    menu.addAction(AppIcons::themed(QStringLiteral("add")), tr("Add"),
                   this, &SubscriptionsWidget::addSubscription);

    QAction *removeAction = menu.addAction(AppIcons::themed(QStringLiteral("remove")), tr("Remove"),
                                           this, &SubscriptionsWidget::removeSelectedSubscriptions);
    removeAction->setEnabled(!rows.isEmpty() && !builtinSelected);

    QAction *removeAllAction = menu.addAction(AppIcons::themed(QStringLiteral("remove")), tr("Remove All"),
                                              this, &SubscriptionsWidget::removeAllSubscriptions);
    removeAllAction->setEnabled(hasRemovableSubscriptions());

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
/// \brief Adds a subscription with the given name and publishing interval.
/// \param name Subscription name; ignored when empty or already in use.
/// \param interval Publishing interval in milliseconds.
///
void SubscriptionsWidget::createSubscription(const QString &name, double interval)
{
    if (name.isEmpty() || _subscriptionsModel->containsName(name))
        return;

    SubscriptionItem subscription;
    subscription.id = _subscriptionsModel->rowCount();
    subscription.name = name;
    subscription.publishingInterval = interval;
    _subscriptionsModel->addSubscription(subscription);
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
    if (subscription.isBuiltin())
        return;
    emit subscriptionRemoved(subscription.name);
    _subscriptionsModel->removeRow(row);
}

///
/// \brief Reports whether any subscription can be removed.
/// \return True when at least one non-built-in subscription exists.
///
bool SubscriptionsWidget::hasRemovableSubscriptions() const
{
    for (int row = 0; row < _subscriptionsModel->rowCount(); ++row) {
        if (!_subscriptionsModel->itemAt(row).isBuiltin())
            return true;
    }
    return false;
}

///
/// \brief Emits the current subscriptions snapshot.
///
void SubscriptionsWidget::emitSubscriptionsChanged()
{
    emit subscriptionsChanged(subscriptions());
}
