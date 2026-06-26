// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file historywidget.cpp
/// \brief Implements the OPC UA history tab widget.
///

#include <QAbstractButton>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QHeaderView>
#include <QLineEdit>
#include <QMessageBox>
#include <QMimeData>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSpinBox>
#include <QTextStream>

#include "appsettings.h"
#include "headerview.h"
#include "historywidget.h"
#include "models/addressspacemimedata.h"
#include "models/historymodel.h"
#include "tableview.h"
#include "ui_historywidget.h"

namespace {

constexpr int historyDateTimeEditMinimumWidth = 190;

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

///
/// \brief Formats a date-time for compact file names.
/// \param value Date-time value.
/// \return File-name-safe date-time text.
///
QString fileNameDateTime(const QDateTime &value)
{
    return value.toString(QStringLiteral("yyyyMMdd_HHmmss"));
}

///
/// \brief Formats the selected node for the History toolbar field.
/// \param nodeId OPC UA NodeId.
/// \param displayName Human-readable node name or path.
/// \return Display name with NodeId when both values are distinct.
///
QString historyNodeText(const QString &nodeId, const QString &displayName)
{
    if (displayName.isEmpty() || displayName == nodeId)
        return nodeId;
    return QStringLiteral("%1 (%2)").arg(displayName, nodeId);
}

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
/// \brief Builds the history widget, its toolbar and table view.
/// \param parent Parent widget.
///
HistoryWidget::HistoryWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::HistoryWidget)
    , _historyModel(new HistoryModel(this))
{
    ui->setupUi(this);
    ui->historyReadButton->setIcon(QStringLiteral("read"));
    ui->historyExportButton->setIcon(QStringLiteral("export"));
    ui->historyClearButton->setIcon(QStringLiteral("trash"));
    setupHistoryView();
    updateActionButtons();
}

///
/// \brief Destroys the widget and its generated UI.
///
HistoryWidget::~HistoryWidget()
{
    delete ui;
}

///
/// \brief Shows history samples in the History table.
/// \param values History samples in time order.
///
void HistoryWidget::setHistoryResults(const QVector<OpcUaHistoryValue> &values)
{
    _historyModel->setItems(values);
}

///
/// \brief Targets a node and requests its history for the current range.
/// \param nodeId Node whose history should be read.
/// \param displayName Human-readable node name.
/// \param displayPath Human-readable path shown in the node field.
///
void HistoryWidget::requestHistoryForNode(const QString &nodeId, const QString &displayName,
                                          const QString &displayPath)
{
    if (!OpcUa::isHistoryReadSupported())
        return;
    if (nodeId.isEmpty())
        return;
    _historyNodeId = nodeId;
    _historyNodeDisplayName = displayName;
    _historyNodeDisplayPath = displayPath;
    const QString label = displayPath.isEmpty() ? displayName : displayPath;
    ui->historyNodeEdit->setText(historyNodeText(nodeId, label));
    ui->historyNodeEdit->setToolTip(nodeId);
    updateActionButtons();
    requestHistoryRead();
}

///
/// \brief Builds the default CSV export file name for the current history query.
/// \return Suggested CSV file name.
///
QString HistoryWidget::suggestedHistoryCsvFileName() const
{
    const QString tag = fileNameSegment(
        !_historyNodeDisplayName.isEmpty() ? _historyNodeDisplayName
        : (_historyNodeDisplayPath.isEmpty() ? _historyNodeId : _historyNodeDisplayPath),
        QStringLiteral("history"));
    QStringList parts = {
        tag,
        fileNameDateTime(ui->historyStartEdit->dateTime()),
        fileNameDateTime(ui->historyEndEdit->dateTime())
    };
    if (ui->historyMaxEdit->value() > ui->historyMaxEdit->minimum())
        parts.append(QStringLiteral("max%1").arg(ui->historyMaxEdit->value()));
    return parts.join(QLatin1Char('_')) + QStringLiteral(".csv");
}

///
/// \brief Clears the selected node and its displayed samples.
///
void HistoryWidget::clear()
{
    clearHistoryNode();
}

///
/// \brief Persists the history table header state.
/// \param settings Settings store to write to.
///
void HistoryWidget::saveViewState(AppSettings &settings) const
{
    settings.setViewState(ui->historyTable->objectName(), ui->historyTable->headerView()->saveLayout());
}

///
/// \brief Restores the history table header state.
/// \param settings Settings store to read from.
///
void HistoryWidget::restoreViewState(AppSettings &settings)
{
    ui->historyTable->headerView()->restoreLayout(settings.viewState(ui->historyTable->objectName()));
}

///
/// \brief Aligns the history date pickers with the local/UTC timestamp mode.
/// \param mode Local time or UTC.
///
void HistoryWidget::setTimestampMode(AppSettings::TimestampMode mode)
{
    _historyModel->setTimestampMode(mode);
    applyHistoryTimestampMode(mode);
}

///
/// \brief Handles address-space node drag/drop events on the history node field.
/// \param watched Object receiving the event.
/// \param event Event to filter.
/// \return True when the event was consumed.
///
bool HistoryWidget::eventFilter(QObject *watched, QEvent *event)
{
    const bool historyNodeTarget = OpcUa::isHistoryReadSupported() && watched == ui->historyNodeEdit;
    if (!historyNodeTarget)
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
            _historyNodeId = node.nodeId;
            const QString label = node.displayName.isEmpty()
                ? (node.browseName.isEmpty() ? node.nodeId : node.browseName)
                : node.displayName;
            _historyNodeDisplayName = label;
            _historyNodeDisplayPath = node.displayPath;
            const QString fieldLabel = node.displayPath.isEmpty() ? label : node.displayPath;
            ui->historyNodeEdit->setText(historyNodeText(node.nodeId, fieldLabel));
            ui->historyNodeEdit->setToolTip(node.nodeId);
            updateActionButtons();
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
/// \brief Binds and lays out the history table and wires the toolbar.
///
void HistoryWidget::setupHistoryView()
{
    ui->historyTable->setModel(_historyModel);
    ui->historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->historyTable->verticalHeader()->hide();

    auto *historyHeader = ui->historyTable->headerView();
    connect(historyHeader, &HeaderView::sectionAlignmentChanged, this,
            [this](int logicalIndex, Qt::Alignment alignment) {
                _historyModel->setColumnAlignment(logicalIndex, alignment | Qt::AlignVCenter);
            });

    historyHeader->setStretchLastSection(false);
    historyHeader->setSectionResizeMode(HistoryModel::ColNumber,          QHeaderView::Fixed);
    historyHeader->setSectionResizeMode(HistoryModel::ColSourceTimestamp, QHeaderView::Interactive);
    historyHeader->setSectionResizeMode(HistoryModel::ColServerTimestamp, QHeaderView::Interactive);
    historyHeader->setSectionResizeMode(HistoryModel::ColValue,           QHeaderView::Stretch);
    historyHeader->setSectionResizeMode(HistoryModel::ColStatus,          QHeaderView::Interactive);

    historyHeader->setSectionAlignment(HistoryModel::ColNumber,          Qt::AlignCenter);
    historyHeader->setSectionAlignment(HistoryModel::ColSourceTimestamp, Qt::AlignCenter);
    historyHeader->setSectionAlignment(HistoryModel::ColServerTimestamp, Qt::AlignCenter);
    historyHeader->setSectionAlignment(HistoryModel::ColValue,           Qt::AlignCenter);
    historyHeader->setSectionAlignment(HistoryModel::ColStatus,          Qt::AlignCenter);

    ui->historyTable->setColumnWidth(HistoryModel::ColNumber,          48 );
    ui->historyTable->setColumnWidth(HistoryModel::ColSourceTimestamp, 200);
    ui->historyTable->setColumnWidth(HistoryModel::ColServerTimestamp, 200);
    ui->historyTable->setColumnWidth(HistoryModel::ColStatus,          90 );

    ui->historyNodeEdit->setAcceptDrops(true);
    ui->historyNodeEdit->installEventFilter(this);

    const QDateTime now = QDateTime::currentDateTime();
    ui->historyEndEdit->setDateTime(now);
    ui->historyStartEdit->setDateTime(now.addSecs(-3600));
    for (QDateTimeEdit *edit : {ui->historyStartEdit, ui->historyEndEdit}) {
        edit->setMinimumWidth(historyDateTimeEditMinimumWidth);
        connect(edit, &QDateTimeEdit::dateTimeChanged, this,
                [this, edit] { updateHistoryZoneSuffix(edit); });
    }
    applyHistoryTimestampMode(AppSettings().timestampMode());

    connect(ui->historyReadButton, &QAbstractButton::clicked,
            this, &HistoryWidget::requestHistoryRead);
    connect(ui->historyExportButton, &QAbstractButton::clicked,
            this, &HistoryWidget::exportHistoryToCsv);
    connect(ui->historyClearButton, &QAbstractButton::clicked,
            this, [this] { _historyModel->clear(); });
    connect(ui->historyNodeEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (text.isEmpty())
            clearHistoryNode();
    });
    connect(_historyModel, &QAbstractItemModel::modelReset, this,
            &HistoryWidget::updateActionButtons);
    connect(_historyModel, &QAbstractItemModel::rowsInserted, this,
            &HistoryWidget::updateActionButtons);
    connect(_historyModel, &QAbstractItemModel::rowsRemoved, this,
            &HistoryWidget::updateActionButtons);
}

///
/// \brief Enables the toolbar buttons that depend on the current node and samples.
///
void HistoryWidget::updateActionButtons()
{
    ui->historyReadButton->setEnabled(OpcUa::isHistoryReadSupported() && !_historyNodeId.isEmpty());
    const bool hasSamples = _historyModel->rowCount() > 0;
    ui->historyExportButton->setEnabled(hasSamples);
    ui->historyClearButton->setEnabled(hasSamples);
}

///
/// \brief Clears the selected History node and its displayed samples.
///
void HistoryWidget::clearHistoryNode()
{
    _historyNodeId.clear();
    _historyNodeDisplayName.clear();
    _historyNodeDisplayPath.clear();
    ui->historyNodeEdit->clear();
    ui->historyNodeEdit->setToolTip(QString());
    _historyModel->clear();
    updateActionButtons();
}

///
/// \brief Requests a history read for the targeted node over the current range.
///
void HistoryWidget::requestHistoryRead()
{
    if (!OpcUa::isHistoryReadSupported())
        return;
    if (_historyNodeId.isEmpty())
        return;
    emit historyReadRequested(_historyNodeId, ui->historyStartEdit->dateTime(),
                              ui->historyEndEdit->dateTime(),
                              static_cast<quint32>(ui->historyMaxEdit->value()));
}

///
/// \brief Saves the currently displayed history samples as CSV.
///
void HistoryWidget::exportHistoryToCsv()
{
    if (_historyModel->rowCount() == 0)
        return;

    const QString fileName = QFileDialog::getSaveFileName(
        this, tr("Export History"), suggestedHistoryCsvFileName(),
        tr("CSV Files (*.csv);;All Files (*)"));
    if (fileName.isEmpty())
        return;

    QSaveFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Export History"),
                             tr("Could not open '%1' for writing.").arg(fileName));
        return;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << _historyModel->toCsv();
    if (!file.commit()) {
        QMessageBox::warning(this, tr("Export History"),
                             tr("Could not save '%1'.").arg(fileName));
    }
}

///
/// \brief Aligns the history date pickers with the local/UTC timestamp mode.
/// \param mode Local time or UTC.
///
void HistoryWidget::applyHistoryTimestampMode(AppSettings::TimestampMode mode)
{
    const bool utc = mode == AppSettings::TimestampMode::Utc;
    const Qt::TimeSpec spec = utc ? Qt::UTC : Qt::LocalTime;

    for (QDateTimeEdit *edit : {ui->historyStartEdit, ui->historyEndEdit}) {
        const QDateTime current = edit->dateTime();
        edit->setTimeSpec(spec);
        edit->setDateTime(utc ? current.toUTC() : current.toLocalTime());
        updateHistoryZoneSuffix(edit);
    }
}

///
/// \brief Appends the field's time zone to its display format as a non-editable suffix.
/// \param edit Date-time edit to update; "Z" for UTC, otherwise the local offset.
///
void HistoryWidget::updateHistoryZoneSuffix(QDateTimeEdit *edit)
{
    const QString zone = edit->timeSpec() == Qt::UTC
        ? QStringLiteral("Z")
        : utcOffsetSuffix(edit->dateTime());
    edit->setDisplayFormat(QStringLiteral("yyyy-MM-dd HH:mm:ss '%1'").arg(zone));
}
