// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file eventswidget.cpp
/// \brief Implements the OPC UA events tab widget.
///

#include <QAbstractButton>
#include <QFileDialog>
#include <QHeaderView>
#include <QLineEdit>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTextStream>

#include "appsettings.h"
#include "eventswidget.h"
#include "headerview.h"
#include "models/eventsmodel.h"
#include "nodelineedit.h"
#include "formatters/attributeformatter.h"
#include "severitydelegate.h"
#include "tableview.h"
#include "ui_eventswidget.h"

namespace {

///
/// \brief Makes a string safe for use as one file-name segment.
/// \param value Segment text.
/// \param fallback Text used when the segment becomes empty.
/// \return File-name segment without filesystem separators or control characters.
///
QString fileNameSegment(QString value, const QString &fallback)
{
    value = value.trimmed();
    static const QRegularExpression invalidChars(QStringLiteral(R"([<>:"/\\|?*\x00-\x1f]+)"));
    value.replace(invalidChars, QStringLiteral("_"));
    value.replace(QRegularExpression(QStringLiteral(R"(\s+)")), QStringLiteral("_"));
    while (value.endsWith(QLatin1Char('.')) || value.endsWith(QLatin1Char(' ')))
        value.chop(1);
    return value.isEmpty() ? fallback : value;
}

} // namespace

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
    updateActionButtons();
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
/// \param displayPath Human-readable path shown in the source field.
///
void EventsWidget::setEventSource(const QString &nodeId, const QString &displayName,
                                  const QString &displayPath)
{
    _subscribed = false;
    _subscribedNodeId.clear();
    ui->eventsNodeEdit->setNode(nodeId, displayName, displayPath);
    ui->eventsSubscribeButton->setEnabled(!nodeId.isEmpty());
    ui->eventsUnsubscribeButton->setEnabled(false);
    updateActionButtons();
}

///
/// \brief Requests event monitoring for the current source node.
///
void EventsWidget::requestEventMonitoring()
{
    if (!ui->eventsNodeEdit->hasNode() || _subscribed)
        return;
    emit eventSubscribeRequested(ui->eventsNodeEdit->nodeId(), 1000.0);
}

///
/// \brief Builds the default CSV export file name for displayed events.
/// \return Suggested CSV file name.
///
QString EventsWidget::suggestedEventsCsvFileName() const
{
    const QString displayName = ui->eventsNodeEdit->nodeDisplayName();
    const QString displayPath = ui->eventsNodeEdit->nodeDisplayPath();
    const QString source = fileNameSegment(
        !displayName.isEmpty() ? displayName
        : (displayPath.isEmpty() ? ui->eventsNodeEdit->nodeId() : displayPath),
        QStringLiteral("events"));
    return QStringLiteral("events_%1.csv").arg(source);
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
    if (nodeId != ui->eventsNodeEdit->nodeId())
        return;
    _subscribed = subscribed;
    _subscribedNodeId = subscribed ? nodeId : QString();
    ui->eventsSubscribeButton->setEnabled(!subscribed && ui->eventsNodeEdit->hasNode());
    ui->eventsUnsubscribeButton->setEnabled(subscribed);
}

///
/// \brief Removes all displayed events.
///
void EventsWidget::clear()
{
    _eventsModel->setItems({});
    updateActionButtons();
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
    ui->eventsTable->setItemDelegateForColumn(EventsModel::ColSeverity,
                                              new SeverityDelegate(this));

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
    ui->eventsExportButton->setIcon(QStringLiteral("export"));
    ui->eventsClearButton->setIcon(QStringLiteral("trash"));

    connect(ui->eventsSubscribeButton, &QAbstractButton::clicked,
            this, &EventsWidget::requestEventMonitoring);
    connect(ui->eventsUnsubscribeButton, &QAbstractButton::clicked, this, [this]() {
        if (ui->eventsNodeEdit->hasNode())
            emit eventUnsubscribeRequested(ui->eventsNodeEdit->nodeId());
    });
    connect(ui->eventsClearButton, &QAbstractButton::clicked, this, [this]() {
        _eventsModel->clear();
    });
    connect(ui->eventsExportButton, &QAbstractButton::clicked,
            this, &EventsWidget::exportEventsToCsv);
    connect(ui->eventsNodeEdit, &NodeLineEdit::nodeCleared, this,
            &EventsWidget::clearEventSource);
    ui->eventsNodeEdit->setNodeAcceptor([](const OpcUaNodeInfo &node) {
        return OpcUa::canMonitorEvents(node);
    });
    connect(ui->eventsNodeEdit, &NodeLineEdit::nodeDropped, this,
            [this](const OpcUaNodeInfo &node) {
        if (_subscribed && _subscribedNodeId == node.nodeId)
            return;
        if (_subscribed)
            emit eventUnsubscribeRequested(_subscribedNodeId);
        _subscribed = false;
        _subscribedNodeId.clear();
        ui->eventsSubscribeButton->setEnabled(ui->eventsNodeEdit->hasNode());
        ui->eventsUnsubscribeButton->setEnabled(false);
        updateActionButtons();
    });
    connect(_eventsModel, &QAbstractItemModel::modelReset, this,
            &EventsWidget::updateActionButtons);
    connect(_eventsModel, &QAbstractItemModel::rowsInserted, this,
            &EventsWidget::updateActionButtons);
    connect(_eventsModel, &QAbstractItemModel::rowsRemoved, this,
            &EventsWidget::updateActionButtons);
}

///
/// \brief Enables toolbar actions that depend on displayed events.
///
void EventsWidget::updateActionButtons()
{
    const bool hasEvents = _eventsModel->rowCount() > 0;
    ui->eventsExportButton->setEnabled(hasEvents);
    ui->eventsClearButton->setEnabled(hasEvents);
}

///
/// \brief Clears the selected event source, stopping monitoring and removing displayed events.
///
void EventsWidget::clearEventSource()
{
    const QString nodeId = ui->eventsNodeEdit->nodeId();
    const bool wasSubscribed = _subscribed;

    _subscribed = false;
    _subscribedNodeId.clear();
    ui->eventsNodeEdit->clearNode();
    ui->eventsSubscribeButton->setEnabled(false);
    ui->eventsUnsubscribeButton->setEnabled(false);
    _eventsModel->clear();
    updateActionButtons();

    if (wasSubscribed && !nodeId.isEmpty())
        emit eventUnsubscribeRequested(nodeId);
}

///
/// \brief Saves the currently displayed events as CSV.
///
void EventsWidget::exportEventsToCsv()
{
    if (_eventsModel->rowCount() == 0)
        return;

    const QString fileName = QFileDialog::getSaveFileName(
        this, tr("Export Events"), suggestedEventsCsvFileName(),
        tr("CSV Files (*.csv);;All Files (*)"));
    if (fileName.isEmpty())
        return;

    QSaveFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Export Events"),
                             tr("Could not open '%1' for writing.").arg(fileName));
        return;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << _eventsModel->toCsv();
    if (!file.commit()) {
        QMessageBox::warning(this, tr("Export Events"),
                             tr("Could not save '%1'.").arg(fileName));
    }
}
