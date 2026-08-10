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
#include <QDirIterator>
#include <QEvent>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QMenu>
#include <QPalette>
#include <QPushButton>
#include <QTranslator>

#include "appicons.h"
#include "appsettings.h"
#include "headerview.h"
#include "models/subscriptionsmodel.h"
#include "publishingintervaldelegate.h"
#include "subscriptionswidget.h"
#include "tableview.h"
#include "tableviewconfig.h"
#include "ui_subscriptionswidget.h"

namespace {

/// \brief Factory publishing interval of a built-in subscription, in milliseconds.
struct BuiltinDefault {
    int id;
    double interval;
};

/// \brief Factory values of the built-in subscriptions, in row order.
constexpr BuiltinDefault builtinDefaults[] = {
    {DefaultSubscriptionId, 1000.0},
    {FastSubscriptionId, 250.0},
    {SlowSubscriptionId, 5000.0}
};

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

///
/// \brief Returns the factory publishing interval of a built-in subscription.
/// \param id Built-in subscription identifier.
/// \return Factory interval in milliseconds, or 0 when the id is not built in.
///
double factoryInterval(int id)
{
    for (const BuiltinDefault &entry : builtinDefaults) {
        if (entry.id == id)
            return entry.interval;
    }
    return 0.0;
}

///
/// \brief Returns the untranslated factory name of a built-in subscription.
/// \param id Built-in subscription identifier.
/// \return Source string passed to tr(), or nullptr when the id is not built in.
///
const char *factorySourceName(int id)
{
    switch (id) {
    case DefaultSubscriptionId: return "Default";
    case FastSubscriptionId:    return "Fast";
    case SlowSubscriptionId:    return "Slow";
    default:                    return nullptr;
    }
}

///
/// \brief Reports whether a stored name is the factory name in any language the app ships.
/// \param id Built-in subscription identifier.
/// \param name Name read from the settings.
/// \return True when the name is a translation of the factory name rather than a user's choice.
///
bool isFactoryNameInAnyLanguage(int id, const QString &name)
{
    const char *source = factorySourceName(id);
    if (!source)
        return false;
    if (name == QString::fromUtf8(source))
        return true;

    static const QStringList catalogues = [] {
        QStringList files;
        QDirIterator iterator(QStringLiteral(":/translations"), {QStringLiteral("*.qm")},
                              QDir::Files);
        while (iterator.hasNext())
            files.append(iterator.next());
        return files;
    }();

    for (const QString &file : catalogues) {
        QTranslator translator;
        if (!translator.load(file))
            continue;
        if (name == translator.translate("SubscriptionsWidget", source))
            return true;
    }
    return false;
}

} // namespace

///
/// \brief Returns the translated factory name of a built-in subscription.
/// \param id Built-in subscription identifier.
/// \return Factory name, or an empty string when the id is not built in.
///
QString SubscriptionsWidget::factoryName(int id)
{
    switch (id) {
    case DefaultSubscriptionId: return tr("Default");
    case FastSubscriptionId:    return tr("Fast");
    case SlowSubscriptionId:    return tr("Slow");
    default:                    return QString();
    }
}

///
/// \brief Maps a stored subscription name onto the name of the current interface language.
/// \param name Subscription name read from settings or a saved session.
/// \return Current factory name when the stored one names a built-in subscription in any
///         shipped language, otherwise the name unchanged.
///
QString SubscriptionsWidget::canonicalName(const QString &name)
{
    if (name.isEmpty())
        return name;
    for (const BuiltinDefault &entry : builtinDefaults) {
        if (isFactoryNameInAnyLanguage(entry.id, name))
            return factoryName(entry.id);
    }
    return name;
}

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
    ui->restoreDefaultsButton->setIcon(QStringLiteral("refresh"));
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
    QVector<SubscriptionItem> items;
    items.reserve(int(std::size(builtinDefaults)));
    for (const BuiltinDefault &entry : builtinDefaults) {
        SubscriptionItem subscription;
        subscription.id = entry.id;
        subscription.name = factoryName(entry.id);
        subscription.publishingInterval = entry.interval;
        subscription.builtin = true;
        items.append(subscription);
    }
    _subscriptionsModel->setItems(items);
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
/// \brief Persists the user-created subscriptions and any edits to the built-in ones.
/// \param settings Settings store to write to.
///
void SubscriptionsWidget::saveSubscriptions(AppSettings &settings) const
{
    const QVector<SubscriptionItem> items = subscriptions();
    settings.setCustomSubscriptions(items);

    QVector<SubscriptionItem> overrides;
    for (const SubscriptionItem &item : items) {
        if (!item.isBuiltin())
            continue;
        if (!item.renamed && qFuzzyCompare(item.publishingInterval, factoryInterval(item.id)))
            continue;

        SubscriptionItem override = item;
        if (!item.renamed)
            override.name.clear();
        overrides.append(override);
    }
    settings.setBuiltinSubscriptionOverrides(overrides);
}

///
/// \brief Restores the subscriptions saved from the last run.
/// \param settings Settings store to read from.
///
void SubscriptionsWidget::loadSubscriptions(AppSettings &settings)
{
    const QVector<SubscriptionItem> overrides = settings.builtinSubscriptionOverrides();
    const bool legacyNames =
        settings.storedBuiltinSubscriptionSchema() < AppSettings::builtinSubscriptionSchema;
    for (const SubscriptionItem &override : overrides) {
        for (int row = 0; row < _subscriptionsModel->rowCount(); ++row) {
            const SubscriptionItem item = _subscriptionsModel->itemAt(row);
            if (!item.isBuiltin() || item.id != override.id)
                continue;
            if (!override.name.isEmpty()
                && !(legacyNames && isFactoryNameInAnyLanguage(override.id, override.name))) {
                _subscriptionsModel->setData(
                    _subscriptionsModel->index(row, SubscriptionsModel::ColName),
                    override.name, Qt::EditRole);
            }
            _subscriptionsModel->setData(
                _subscriptionsModel->index(row, SubscriptionsModel::ColPublishingInterval),
                override.publishingInterval, Qt::EditRole);
            break;
        }
    }

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
    ui->subscriptionsTable->verticalHeader()->hide();
    ui->subscriptionsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->subscriptionsTable->setEditTriggers(QAbstractItemView::DoubleClicked
                                            | QAbstractItemView::SelectedClicked);
    ui->subscriptionsTable->setItemDelegateForColumn(
        SubscriptionsModel::ColPublishingInterval, new PublishingIntervalDelegate(this));
    ui->subscriptionsTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->subscriptionsTable, &QWidget::customContextMenuRequested,
            this, &SubscriptionsWidget::showSubscriptionsContextMenu);

    TableViewConfig::apply(ui->subscriptionsTable,
        {
            {SubscriptionsModel::ColName, QHeaderView::Interactive, 120},
            {SubscriptionsModel::ColPublishingInterval, QHeaderView::Stretch},
        },
        [this](int logicalIndex, Qt::Alignment alignment) {
            _subscriptionsModel->setColumnAlignment(logicalIndex, alignment);
        });

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
    connect(ui->restoreDefaultsButton, &QPushButton::clicked,
            this, &SubscriptionsWidget::restoreBuiltinDefaults);
}

///
/// \brief Applies the shaded background that marks built-in subscriptions as permanent.
///
void SubscriptionsWidget::applyBuiltinDecoration()
{
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
    else if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        _subscriptionsModel->retranslate();
        retranslateBuiltinNames();
    }
}

///
/// \brief Renames the built-in subscriptions the user never renamed into the new language.
///
void SubscriptionsWidget::retranslateBuiltinNames()
{
    for (int row = 0; row < _subscriptionsModel->rowCount(); ++row) {
        const SubscriptionItem item = _subscriptionsModel->itemAt(row);
        if (!item.isBuiltin() || item.renamed)
            continue;
        _subscriptionsModel->setFactoryName(row, factoryName(item.id));
    }
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

    QAction *removeAllAction = menu.addAction(AppIcons::themed(QStringLiteral("remove")), tr("Clear"),
                                              this, &SubscriptionsWidget::removeAllSubscriptions);
    removeAllAction->setEnabled(hasRemovableSubscriptions());

    menu.addSeparator();
    menu.addAction(AppIcons::themed(QStringLiteral("refresh")), tr("Restore Defaults"),
                   this, &SubscriptionsWidget::restoreBuiltinDefaults);

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
    subscription.id = nextSubscriptionId();
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
    subscription.id = nextSubscriptionId();
    subscription.name = name;
    subscription.publishingInterval = interval;
    _subscriptionsModel->addSubscription(subscription);
}

///
/// \brief Returns an identifier that no existing subscription uses.
/// \return One past the highest identifier in use, never colliding with a built-in id.
///
int SubscriptionsWidget::nextSubscriptionId() const
{
    int highest = int(std::size(builtinDefaults)) - 1;
    for (int row = 0; row < _subscriptionsModel->rowCount(); ++row)
        highest = std::max(highest, _subscriptionsModel->itemAt(row).id);
    return highest + 1;
}

///
/// \brief Restores the factory name and publishing interval of every built-in subscription.
///
/// User-created subscriptions are left untouched. Values are written through the model so
/// renames and interval changes reach the data access rows already bound to these names.
///
void SubscriptionsWidget::restoreBuiltinDefaults()
{
    for (int row = 0; row < _subscriptionsModel->rowCount(); ++row) {
        const SubscriptionItem subscription = _subscriptionsModel->itemAt(row);
        if (!subscription.isBuiltin())
            continue;
        _subscriptionsModel->setFactoryName(row, factoryName(subscription.id));
        _subscriptionsModel->setData(
            _subscriptionsModel->index(row, SubscriptionsModel::ColPublishingInterval),
            factoryInterval(subscription.id), Qt::EditRole);
    }
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
