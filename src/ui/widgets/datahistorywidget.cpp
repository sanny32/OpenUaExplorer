// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file datahistorywidget.cpp
/// \brief Implements the OPC UA data history tab widget.
///

#include <QAbstractButton>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QFileDialog>
#include <QHeaderView>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSpinBox>
#include <QTextStream>
#include <QTimeZone>

#include "appsettings.h"
#include "headerview.h"
#include "datahistorywidget.h"
#include "models/historymodel.h"
#include "nodelineedit.h"
#include "tableview.h"
#include "tableviewconfig.h"
#include "ui_datahistorywidget.h"

namespace {

constexpr int dataHistoryDateTimeEditMinimumWidth = 190;

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
/// \brief Builds the data history widget, its toolbar and table view.
/// \param parent Parent widget.
///
DataHistoryWidget::DataHistoryWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DataHistoryWidget)
    , _dataHistoryModel(new HistoryModel(this))
{
    ui->setupUi(this);
    ui->dataHistoryReadButton->setIcon(QStringLiteral("read"));
    ui->dataHistoryExportButton->setIcon(QStringLiteral("export"));
    ui->dataHistoryClearButton->setIcon(QStringLiteral("trash"));
    setupDataHistoryView();
    updateActionButtons();
}

///
/// \brief Destroys the widget and its generated UI.
///
DataHistoryWidget::~DataHistoryWidget()
{
    delete ui;
}

///
/// \brief Shows data history samples in the Data History table.
/// \param values Data history samples in time order.
///
void DataHistoryWidget::setDataHistoryResults(const QVector<OpcUaHistoryValue> &values)
{
    _dataHistoryModel->setItems(values);
}

///
/// \brief Targets a node and requests its data history for the current range.
/// \param nodeId Node whose data history should be read.
/// \param displayName Human-readable node name.
/// \param displayPath Human-readable path shown in the node field.
///
void DataHistoryWidget::requestDataHistoryForNode(const QString &nodeId, const QString &displayName,
                                          const QString &displayPath)
{
    if (!OpcUa::isHistoryReadSupported())
        return;
    if (nodeId.isEmpty())
        return;
    ui->dataHistoryNodeEdit->setNode(nodeId, displayName, displayPath);
    updateActionButtons();
    requestDataHistoryRead();
}

///
/// \brief Builds the default CSV export file name for the current data history query.
/// \return Suggested CSV file name.
///
QString DataHistoryWidget::suggestedDataHistoryCsvFileName() const
{
    const QString displayName = ui->dataHistoryNodeEdit->nodeDisplayName();
    const QString displayPath = ui->dataHistoryNodeEdit->nodeDisplayPath();
    const QString tag = fileNameSegment(
        !displayName.isEmpty() ? displayName
        : (displayPath.isEmpty() ? ui->dataHistoryNodeEdit->nodeId() : displayPath),
        QStringLiteral("history"));
    QStringList parts = {
        tag,
        fileNameDateTime(ui->dataHistoryStartEdit->dateTime()),
        fileNameDateTime(ui->dataHistoryEndEdit->dateTime())
    };
    if (ui->dataHistoryMaxEdit->value() > ui->dataHistoryMaxEdit->minimum())
        parts.append(QStringLiteral("max%1").arg(ui->dataHistoryMaxEdit->value()));
    return parts.join(QLatin1Char('_')) + QStringLiteral(".csv");
}

///
/// \brief Clears the selected node and its displayed samples.
///
void DataHistoryWidget::clear()
{
    clearDataHistoryNode();
}

///
/// \brief Persists the data history table header state.
/// \param settings Settings store to write to.
///
void DataHistoryWidget::saveViewState(AppSettings &settings) const
{
    settings.setViewState(ui->dataHistoryTable->objectName(), ui->dataHistoryTable->headerView()->saveLayout());
}

///
/// \brief Restores the data history table header state.
/// \param settings Settings store to read from.
///
void DataHistoryWidget::restoreViewState(AppSettings &settings)
{
    ui->dataHistoryTable->headerView()->restoreLayout(settings.viewState(ui->dataHistoryTable->objectName()));
}

///
/// \brief Aligns the data history date pickers with the local/UTC timestamp mode.
/// \param mode Local time or UTC.
///
void DataHistoryWidget::setTimestampMode(AppSettings::TimestampMode mode)
{
    _dataHistoryModel->setTimestampMode(mode);
    applyDataHistoryTimestampMode(mode);
}

///
/// \brief Binds and lays out the data history table and wires the toolbar.
///
void DataHistoryWidget::setupDataHistoryView()
{
    ui->dataHistoryTable->setModel(_dataHistoryModel);
    ui->dataHistoryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->dataHistoryTable->verticalHeader()->hide();

    TableViewConfig::apply(ui->dataHistoryTable,
        {
            {HistoryModel::ColNumber, QHeaderView::Fixed, 48, Qt::AlignCenter},
            {HistoryModel::ColSourceTimestamp, QHeaderView::Interactive, 200, Qt::AlignCenter},
            {HistoryModel::ColServerTimestamp, QHeaderView::Interactive, 200, Qt::AlignCenter},
            {HistoryModel::ColValue, QHeaderView::Stretch, -1, Qt::AlignCenter},
            {HistoryModel::ColStatus, QHeaderView::Interactive, 90, Qt::AlignCenter},
        },
        [this](int logicalIndex, Qt::Alignment alignment) {
            _dataHistoryModel->setColumnAlignment(logicalIndex, alignment);
        });

    ui->dataHistoryNodeEdit->setNodeAcceptor([](const OpcUaNodeInfo &node) {
        return OpcUa::canReadHistory(node);
    });
    connect(ui->dataHistoryNodeEdit, &NodeLineEdit::nodeDropped, this,
            [this] { updateActionButtons(); });

    const QDateTime now = QDateTime::currentDateTime();
    ui->dataHistoryEndEdit->setDateTime(now);
    ui->dataHistoryStartEdit->setDateTime(now.addSecs(-3600));
    for (QDateTimeEdit *edit : {ui->dataHistoryStartEdit, ui->dataHistoryEndEdit}) {
        edit->setMinimumWidth(dataHistoryDateTimeEditMinimumWidth);
        connect(edit, &QDateTimeEdit::dateTimeChanged, this,
                [this, edit] { updateDataHistoryZoneSuffix(edit); });
    }
    applyDataHistoryTimestampMode(AppSettings().timestampMode());

    connect(ui->dataHistoryReadButton, &QAbstractButton::clicked,
            this, &DataHistoryWidget::requestDataHistoryRead);
    connect(ui->dataHistoryExportButton, &QAbstractButton::clicked,
            this, &DataHistoryWidget::exportDataHistoryToCsv);
    connect(ui->dataHistoryClearButton, &QAbstractButton::clicked,
            this, [this] { _dataHistoryModel->clear(); });
    connect(ui->dataHistoryNodeEdit, &NodeLineEdit::nodeCleared, this,
            &DataHistoryWidget::clearDataHistoryNode);
    connect(_dataHistoryModel, &QAbstractItemModel::modelReset, this,
            &DataHistoryWidget::updateActionButtons);
    connect(_dataHistoryModel, &QAbstractItemModel::rowsInserted, this,
            &DataHistoryWidget::updateActionButtons);
    connect(_dataHistoryModel, &QAbstractItemModel::rowsRemoved, this,
            &DataHistoryWidget::updateActionButtons);
}

///
/// \brief Enables the toolbar buttons that depend on the current node and samples.
///
void DataHistoryWidget::updateActionButtons()
{
    ui->dataHistoryReadButton->setEnabled(OpcUa::isHistoryReadSupported()
                                      && ui->dataHistoryNodeEdit->hasNode());
    const bool hasSamples = _dataHistoryModel->rowCount() > 0;
    ui->dataHistoryExportButton->setEnabled(hasSamples);
    ui->dataHistoryClearButton->setEnabled(hasSamples);
}

///
/// \brief Clears the selected Data History node and its displayed samples.
///
void DataHistoryWidget::clearDataHistoryNode()
{
    ui->dataHistoryNodeEdit->clearNode();
    _dataHistoryModel->clear();
    updateActionButtons();
}

///
/// \brief Requests a data history read for the targeted node over the current range.
///
void DataHistoryWidget::requestDataHistoryRead()
{
    if (!OpcUa::isHistoryReadSupported())
        return;
    if (!ui->dataHistoryNodeEdit->hasNode())
        return;
    emit dataHistoryReadRequested(ui->dataHistoryNodeEdit->nodeId(), ui->dataHistoryStartEdit->dateTime(),
                              ui->dataHistoryEndEdit->dateTime(),
                              static_cast<quint32>(ui->dataHistoryMaxEdit->value()));
}

///
/// \brief Saves the currently displayed data history samples as CSV.
///
void DataHistoryWidget::exportDataHistoryToCsv()
{
    if (_dataHistoryModel->rowCount() == 0)
        return;

    const QString fileName = QFileDialog::getSaveFileName(
        this, tr("Export Data History"), suggestedDataHistoryCsvFileName(),
        tr("CSV Files (*.csv);;All Files (*)"));
    if (fileName.isEmpty())
        return;

    QSaveFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Export Data History"),
                             tr("Could not open '%1' for writing.").arg(fileName));
        return;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << _dataHistoryModel->toCsv();
    if (!file.commit()) {
        QMessageBox::warning(this, tr("Export Data History"),
                             tr("Could not save '%1'.").arg(fileName));
    }
}

///
/// \brief Aligns the data history date pickers with the local/UTC timestamp mode.
/// \param mode Local time or UTC.
///
void DataHistoryWidget::applyDataHistoryTimestampMode(AppSettings::TimestampMode mode)
{
    const bool utc = mode == AppSettings::TimestampMode::Utc;
    const QTimeZone zone = utc ? QTimeZone::UTC : QTimeZone::LocalTime;

    for (QDateTimeEdit *edit : {ui->dataHistoryStartEdit, ui->dataHistoryEndEdit}) {
        const QDateTime current = edit->dateTime();
        edit->setTimeZone(zone);
        edit->setDateTime(utc ? current.toUTC() : current.toLocalTime());
        updateDataHistoryZoneSuffix(edit);
    }
}

///
/// \brief Appends the field's time zone to its display format as a non-editable suffix.
/// \param edit Date-time edit to update; "Z" for UTC, otherwise the local offset.
///
void DataHistoryWidget::updateDataHistoryZoneSuffix(QDateTimeEdit *edit)
{
    const QString zone = edit->timeZone() == QTimeZone::UTC
        ? QStringLiteral("Z")
        : utcOffsetSuffix(edit->dateTime());
    edit->setDisplayFormat(QStringLiteral("yyyy-MM-dd HH:mm:ss '%1'").arg(zone));
}
