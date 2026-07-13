// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file eventshistorywidget.cpp
/// \brief Implements the OPC UA events history tab widget.
///

#include <QAbstractButton>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QHeaderView>
#include <QSpinBox>
#include <QTimeZone>

#include "appsettings.h"
#include "eventshistorywidget.h"
#include "fileexport.h"
#include "headerview.h"
#include "models/eventsmodel.h"
#include "nodelineedit.h"
#include "utils.h"
#include "formatters/attributeformatter.h"
#include "severitydelegate.h"
#include "tableview.h"
#include "tableviewconfig.h"
#include "ui_eventshistorywidget.h"

namespace {

constexpr int eventsHistoryDateTimeEditMinimumWidth = 190;

///
/// \brief Formats a date-time's UTC offset as a "+HH:mm" zone indicator.
/// \param dateTime Date-time whose offset is rendered; honours DST for local times.
/// \return Signed zone offset, e.g. "+03:00".
///
QString utcOffsetSuffix(const QDateTime &dateTime)
{
    const int offsetSeconds = dateTime.offsetFromUtc();
    const QChar sign = offsetSeconds < 0 ? QLatin1Char('-') : QLatin1Char('+');
    const int absSeconds = offsetSeconds < 0 ? -offsetSeconds : offsetSeconds;
    return QStringLiteral("%1%2:%3")
        .arg(sign)
        .arg(absSeconds / 3600, 2, 10, QLatin1Char('0'))
        .arg(absSeconds % 3600 / 60, 2, 10, QLatin1Char('0'));
}

} // namespace

///
/// \brief Builds the events history widget, its toolbar and table view.
/// \param parent Parent widget.
///
EventsHistoryWidget::EventsHistoryWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::EventsHistoryWidget)
    , _eventsHistoryModel(new EventsModel(this))
{
    ui->setupUi(this);
    ui->eventsHistoryReadButton->setIcon(QStringLiteral("history"));
    ui->eventsHistoryExportButton->setIcon(QStringLiteral("export"));
    ui->eventsHistoryClearButton->setIcon(QStringLiteral("trash"));
    setupEventsHistoryView();
    updateActionButtons();
}

///
/// \brief Destroys the widget and its generated UI.
///
EventsHistoryWidget::~EventsHistoryWidget()
{
    delete ui;
}

///
/// \brief Shows historical events in the table.
/// \param events Events to display.
///
void EventsHistoryWidget::setEventsHistoryResults(const QVector<OpcUaEvent> &events)
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
    _eventsHistoryModel->setItems(items);
}

///
/// \brief Targets a node and requests its historical events for the current range.
/// \param nodeId Node whose event history should be read.
/// \param displayName Human-readable node name.
/// \param displayPath Human-readable path shown in the node field.
///
void EventsHistoryWidget::requestEventsHistoryForNode(const QString &nodeId,
                                                      const QString &displayName,
                                                      const QString &displayPath)
{
    if (nodeId.isEmpty())
        return;
    ui->eventsHistoryNodeEdit->setNode(nodeId, displayName, displayPath);
    updateActionButtons();
    requestEventsHistoryRead();
}

///
/// \brief Builds the default CSV export file name for the current event-history query.
/// \return Suggested CSV file name.
///
QString EventsHistoryWidget::suggestedEventsHistoryCsvFileName() const
{
    const QString displayName = ui->eventsHistoryNodeEdit->nodeDisplayName();
    const QString displayPath = ui->eventsHistoryNodeEdit->nodeDisplayPath();
    const QString tag = Utils::fileNameSegment(
        !displayName.isEmpty() ? displayName
        : (displayPath.isEmpty() ? ui->eventsHistoryNodeEdit->nodeId() : displayPath),
        QStringLiteral("events_history"));
    QStringList parts = {
        tag,
        Utils::fileNameDateTime(ui->eventsHistoryStartEdit->dateTime()),
        Utils::fileNameDateTime(ui->eventsHistoryEndEdit->dateTime())
    };
    if (ui->eventsHistoryMaxEdit->value() > ui->eventsHistoryMaxEdit->minimum())
        parts.append(QStringLiteral("max%1").arg(ui->eventsHistoryMaxEdit->value()));
    return parts.join(QLatin1Char('_')) + QStringLiteral(".csv");
}

///
/// \brief Clears the selected node and displayed events.
///
void EventsHistoryWidget::clear()
{
    clearEventsHistoryNode();
}

///
/// \brief Persists the events history table header state.
/// \param settings Settings store to write to.
///
void EventsHistoryWidget::saveViewState(AppSettings &settings) const
{
    settings.setViewState(ui->eventsHistoryTable->objectName(),
                          ui->eventsHistoryTable->headerView()->saveLayout());
}

///
/// \brief Restores the events history table header state.
/// \param settings Settings store to read from.
///
void EventsHistoryWidget::restoreViewState(AppSettings &settings)
{
    ui->eventsHistoryTable->headerView()->restoreLayout(
        settings.viewState(ui->eventsHistoryTable->objectName()));
}

///
/// \brief Aligns the event-history date pickers with the local/UTC timestamp mode.
/// \param mode Local time or UTC.
///
void EventsHistoryWidget::setTimestampMode(AppSettings::TimestampMode mode)
{
    applyEventsHistoryTimestampMode(mode);
}

///
/// \brief Binds and lays out the events history table and wires the toolbar.
///
void EventsHistoryWidget::setupEventsHistoryView()
{
    ui->eventsHistoryTable->setModel(_eventsHistoryModel);
    ui->eventsHistoryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->eventsHistoryTable->verticalHeader()->hide();
    ui->eventsHistoryTable->setItemDelegateForColumn(EventsModel::ColSeverity,
                                                     new SeverityDelegate(this));

    TableViewConfig::apply(ui->eventsHistoryTable,
        {
            {EventsModel::ColTime, QHeaderView::Interactive, 200},
            {EventsModel::ColSeverity, QHeaderView::Fixed, 70, Qt::AlignCenter},
            {EventsModel::ColSource, QHeaderView::Interactive, 140},
            {EventsModel::ColMessage, QHeaderView::Stretch},
            {EventsModel::ColEventType, QHeaderView::Interactive, 180},
        },
        [this](int logicalIndex, Qt::Alignment alignment) {
            _eventsHistoryModel->setColumnAlignment(logicalIndex, alignment);
        });

    ui->eventsHistoryNodeEdit->setNodeAcceptor([](const OpcUaNodeInfo &node) {
        return OpcUa::canReadEventHistory(node);
    });
    connect(ui->eventsHistoryNodeEdit, &NodeLineEdit::nodeDropped, this,
            [this] { updateActionButtons(); });

    const QDateTime now = QDateTime::currentDateTime();
    ui->eventsHistoryEndEdit->setDateTime(now);
    ui->eventsHistoryStartEdit->setDateTime(now.addSecs(-3600));
    for (QDateTimeEdit *edit : {ui->eventsHistoryStartEdit, ui->eventsHistoryEndEdit}) {
        edit->setMinimumWidth(eventsHistoryDateTimeEditMinimumWidth);
        connect(edit, &QDateTimeEdit::dateTimeChanged, this,
                [this, edit] { updateEventsHistoryZoneSuffix(edit); });
    }
    applyEventsHistoryTimestampMode(AppSettings().timestampMode());

    connect(ui->eventsHistoryReadButton, &QAbstractButton::clicked,
            this, &EventsHistoryWidget::requestEventsHistoryRead);
    connect(ui->eventsHistoryExportButton, &QAbstractButton::clicked,
            this, &EventsHistoryWidget::exportEventsHistoryToCsv);
    connect(ui->eventsHistoryClearButton, &QAbstractButton::clicked,
            this, [this] { _eventsHistoryModel->clear(); });
    connect(ui->eventsHistoryNodeEdit, &NodeLineEdit::nodeCleared, this,
            &EventsHistoryWidget::clearEventsHistoryNode);
    connect(_eventsHistoryModel, &QAbstractItemModel::modelReset, this,
            &EventsHistoryWidget::updateActionButtons);
    connect(_eventsHistoryModel, &QAbstractItemModel::rowsInserted, this,
            &EventsHistoryWidget::updateActionButtons);
    connect(_eventsHistoryModel, &QAbstractItemModel::rowsRemoved, this,
            &EventsHistoryWidget::updateActionButtons);
}

///
/// \brief Enables toolbar buttons that depend on the current node and events.
///
void EventsHistoryWidget::updateActionButtons()
{
    ui->eventsHistoryReadButton->setEnabled(ui->eventsHistoryNodeEdit->hasNode());
    const bool hasEvents = _eventsHistoryModel->rowCount() > 0;
    ui->eventsHistoryExportButton->setEnabled(hasEvents);
    ui->eventsHistoryClearButton->setEnabled(hasEvents);
}

///
/// \brief Clears the selected event-history node and displayed events.
///
void EventsHistoryWidget::clearEventsHistoryNode()
{
    ui->eventsHistoryNodeEdit->clearNode();
    _eventsHistoryModel->clear();
    updateActionButtons();
}

///
/// \brief Requests event history for the targeted node over the current range.
///
void EventsHistoryWidget::requestEventsHistoryRead()
{
    if (!ui->eventsHistoryNodeEdit->hasNode())
        return;
    emit eventsHistoryReadRequested(
        ui->eventsHistoryNodeEdit->nodeId(), ui->eventsHistoryStartEdit->dateTime(),
        ui->eventsHistoryEndEdit->dateTime(),
        static_cast<quint32>(ui->eventsHistoryMaxEdit->value()));
}

///
/// \brief Saves the currently displayed historical events as CSV.
///
void EventsHistoryWidget::exportEventsHistoryToCsv()
{
    if (_eventsHistoryModel->rowCount() == 0)
        return;

    FileExport::exportModelToCsv(this, tr("Export Events History"),
                                 suggestedEventsHistoryCsvFileName(), *_eventsHistoryModel);
}

///
/// \brief Aligns the event-history date pickers with the local/UTC timestamp mode.
/// \param mode Local time or UTC.
///
void EventsHistoryWidget::applyEventsHistoryTimestampMode(AppSettings::TimestampMode mode)
{
    const bool utc = mode == AppSettings::TimestampMode::Utc;
    const QTimeZone zone = utc ? QTimeZone::UTC : QTimeZone::LocalTime;

    for (QDateTimeEdit *edit : {ui->eventsHistoryStartEdit, ui->eventsHistoryEndEdit}) {
        const QDateTime current = edit->dateTime();
        edit->setTimeZone(zone);
        edit->setDateTime(utc ? current.toUTC() : current.toLocalTime());
        updateEventsHistoryZoneSuffix(edit);
    }
}

///
/// \brief Appends the field's time zone to its display format as a non-editable suffix.
/// \param edit Date-time edit to update; "Z" for UTC, otherwise the local offset.
///
void EventsHistoryWidget::updateEventsHistoryZoneSuffix(QDateTimeEdit *edit)
{
    const QString zone = edit->timeZone() == QTimeZone::UTC
        ? QStringLiteral("Z")
        : utcOffsetSuffix(edit->dateTime());
    edit->setDisplayFormat(QStringLiteral("yyyy-MM-dd HH:mm:ss '%1'").arg(zone));
}
