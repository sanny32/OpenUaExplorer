// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file eventswidget.cpp
/// \brief Implements the OPC UA events tab widget.
///

#include <QAbstractButton>
#include <QHeaderView>
#include <QLineEdit>

#include "appsettings.h"
#include "eventswidget.h"
#include "headerview.h"
#include "models/eventsmodel.h"
#include "opcua/attributeformatter.h"
#include "tableview.h"
#include "ui_eventswidget.h"

///
/// \brief Builds the events widget and its table view.
/// \param parent Parent widget.
///
EventsWidget::EventsWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::EventsWidget)
    , _eventsModel(new EventsModel(this))
{
    ui->setupUi(this);
    setupEventsView();
    configureToolbar();
}

///
/// \brief Destroys the widget and its generated UI.
///
EventsWidget::~EventsWidget()
{
    delete ui;
}

///
/// \brief Targets a node as the event source and enables subscribing.
/// \param nodeId Node to monitor for events.
/// \param displayName Human-readable name shown in the source field.
///
void EventsWidget::setEventSource(const QString &nodeId, const QString &displayName)
{
    _eventsNodeId = nodeId;
    _subscribed = false;
    ui->eventsNodeEdit->setText(displayName.isEmpty() ? nodeId : displayName);
    ui->eventsSubscribeButton->setEnabled(!nodeId.isEmpty());
    ui->eventsUnsubscribeButton->setEnabled(false);
}

///
/// \brief Requests event monitoring for the current source node.
///
void EventsWidget::requestEventMonitoring()
{
    if (_eventsNodeId.isEmpty() || _subscribed)
        return;
    emit eventSubscribeRequested(_eventsNodeId, 1000.0);
}

///
/// \brief Appends received events to the table.
/// \param events Events to display.
///
void EventsWidget::appendEvents(const QVector<OpcUaEvent> &events)
{
    QVector<EventItem> items;
    items.reserve(events.size());
    for (const OpcUaEvent &event : events) {
        EventItem item;
        item.time = OpcUaFormat::isoTimestampWithZone(event.time);
        item.severity = QString::number(event.severity);
        item.source = event.sourceName;
        item.message = event.message;
        item.eventType = event.eventType;
        items.append(item);
    }
    _eventsModel->addEvents(items);
}

///
/// \brief Updates the toolbar button states for a node's monitoring outcome.
/// \param nodeId Affected node.
/// \param subscribed Whether the node is now monitored for events.
///
void EventsWidget::setEventMonitoringState(const QString &nodeId, bool subscribed)
{
    if (nodeId != _eventsNodeId)
        return;
    _subscribed = subscribed;
    ui->eventsSubscribeButton->setEnabled(!subscribed && !_eventsNodeId.isEmpty());
    ui->eventsUnsubscribeButton->setEnabled(subscribed);
}

///
/// \brief Removes all displayed events.
///
void EventsWidget::clear()
{
    _eventsModel->setItems({});
}

///
/// \brief Persists the events table header state.
/// \param settings Settings store to write to.
///
void EventsWidget::saveViewState(AppSettings &settings) const
{
    settings.setViewState(ui->eventsTable->objectName(), ui->eventsTable->headerView()->saveLayout());
}

///
/// \brief Restores the events table header state.
/// \param settings Settings store to read from.
///
void EventsWidget::restoreViewState(AppSettings &settings)
{
    ui->eventsTable->headerView()->restoreLayout(settings.viewState(ui->eventsTable->objectName()));
}

///
/// \brief Binds and lays out the events table.
///
void EventsWidget::setupEventsView()
{
    ui->eventsTable->setModel(_eventsModel);
    ui->eventsTable->verticalHeader()->hide();

    auto *eventsHeader = ui->eventsTable->headerView();
    connect(eventsHeader, &HeaderView::sectionAlignmentChanged, this,
            [this](int logicalIndex, Qt::Alignment alignment) {
                _eventsModel->setColumnAlignment(logicalIndex, alignment | Qt::AlignVCenter);
            });

    eventsHeader->setStretchLastSection(false);
    eventsHeader->setSectionResizeMode(EventsModel::ColTime,      QHeaderView::Interactive);
    eventsHeader->setSectionResizeMode(EventsModel::ColSeverity,  QHeaderView::Fixed);
    eventsHeader->setSectionResizeMode(EventsModel::ColSource,    QHeaderView::Interactive);
    eventsHeader->setSectionResizeMode(EventsModel::ColMessage,   QHeaderView::Stretch);
    eventsHeader->setSectionResizeMode(EventsModel::ColEventType, QHeaderView::Interactive);

    eventsHeader->setSectionAlignment(EventsModel::ColSeverity, Qt::AlignCenter);

    ui->eventsTable->setColumnWidth(EventsModel::ColTime,      200);
    ui->eventsTable->setColumnWidth(EventsModel::ColSeverity,  70 );
    ui->eventsTable->setColumnWidth(EventsModel::ColSource,    140);
    ui->eventsTable->setColumnWidth(EventsModel::ColEventType, 180);
}

///
/// \brief Wires the toolbar buttons and their icons.
///
void EventsWidget::configureToolbar()
{
    ui->eventsSubscribeButton->setIcon(QStringLiteral("subscribe"));
    ui->eventsUnsubscribeButton->setIcon(QStringLiteral("unsubscribe"));
    ui->eventsClearButton->setIcon(QStringLiteral("trash"));

    connect(ui->eventsSubscribeButton, &QAbstractButton::clicked,
            this, &EventsWidget::requestEventMonitoring);
    connect(ui->eventsUnsubscribeButton, &QAbstractButton::clicked, this, [this]() {
        if (!_eventsNodeId.isEmpty())
            emit eventUnsubscribeRequested(_eventsNodeId);
    });
    connect(ui->eventsClearButton, &QAbstractButton::clicked, this, [this]() {
        _eventsModel->clear();
    });
}
