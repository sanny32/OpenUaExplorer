// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file logwidget.cpp
/// \brief Implements the activity log widget.
///

#include <atomic>
#include <QDateTime>
#include <QEvent>
#include <QMetaObject>
#include <QPointer>

#include "appicons.h"
#include "headerview.h"
#include "loggingcategories.h"
#include "logmodel.h"
#include "logwidget.h"
#include "tableview.h"
#include "ui_logwidget.h"

namespace {
static LogWidget*       s_instance    = nullptr;
static QtMessageHandler s_prevHandler = nullptr;
static std::atomic<quint64> s_logEpoch { 0 };

///
/// \brief Qt message handler that forwards "ouaexp.*" log messages to the active LogWidget.
/// \param type Qt message severity.
/// \param ctx Log context carrying the category.
/// \param msg Message text.
///
static void appMessageHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg)
{
    if (s_prevHandler) s_prevHandler(type, ctx, msg);
    const QLatin1String prefix("ouaexp.");
    const QLatin1String category(ctx.category ? ctx.category : "");
    if (!s_instance || !category.startsWith(prefix)) return;

    LogItem::Level level;
    switch (type) {
    case QtWarningMsg:  level = LogItem::Level::Warning; break;
    case QtCriticalMsg:
    case QtFatalMsg:    level = LogItem::Level::Error;   break;
    default:            level = LogItem::Level::Info;    break;
    }

    QString text = msg;
    if (text.length() >= 2 && text.startsWith('"') && text.endsWith('"'))
        text = text.mid(1, text.length() - 2);

    const QString source = QString(category).mid(prefix.size());

    LogItem item;
    item.timestamp = QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss.zzz"));
    item.level     = level;
    item.source    = source;
    item.message   = text;

    const quint64 epoch = s_logEpoch.load(std::memory_order_relaxed);
    QPointer<LogWidget> guard(s_instance);
    QMetaObject::invokeMethod(s_instance, [guard, item, epoch]() {
        if (guard && s_logEpoch.load(std::memory_order_relaxed) == epoch)
            guard->addItem(item);
    }, Qt::QueuedConnection);
}
} // namespace

///
/// \brief Builds the log widget, installs the message handler, and wires its controls.
/// \param parent Parent widget.
///
LogWidget::LogWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LogWidget)
    , _model(new LogModel(this))
{
    ui->setupUi(this);
    setupLogView();

    _searchIconAction = ui->searchEdit->addAction(QIcon(), QLineEdit::LeadingPosition);
    refreshIcons();

    s_instance = this;
    s_prevHandler = qInstallMessageHandler(appMessageHandler);

    connect(ui->clearButton, &QPushButton::clicked, this, [this]() {
        s_logEpoch.fetch_add(1, std::memory_order_relaxed);
        _model->clear();
    });

    connect(ui->pauseButton, &QPushButton::toggled, this, [this](bool checked) {
        _paused = checked;
        ui->pauseButton->setIcon(checked ? QStringLiteral("resume.svg") : QStringLiteral("pause.svg"));
        ui->pauseButton->setText(checked ? tr("Resume") : tr("Pause"));
    });

    connect(ui->levelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        switch (index) {
        case 0: _model->clearFilterLevel();                        break;
        case 1: _model->setFilterLevel(LogItem::Level::Info);     break;
        case 2: _model->setFilterLevel(LogItem::Level::Warning);  break;
        case 3: _model->setFilterLevel(LogItem::Level::Error);    break;
        }
    });

    connect(ui->searchEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        _model->setSearchFilter(text);
    });

    connect(_model, &LogModel::rowsInserted, this, [this]() {
        if (ui->autoScrollCheck->isChecked())
            scrollToBottom();
    });
}

///
/// \brief Restores the previous message handler and destroys the widget.
///
LogWidget::~LogWidget()
{
    qInstallMessageHandler(s_prevHandler);
    s_instance    = nullptr;
    s_prevHandler = nullptr;
    delete ui;
}

///
/// \brief Appends a log entry unless logging is paused.
/// \param item Log entry to add.
///
void LogWidget::addItem(const LogItem &item)
{
    if (!_paused)
        _model->addItem(item);
}

///
/// \brief Refreshes the themed icons when the palette changes.
/// \param event Change event being handled.
///
void LogWidget::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);

    if (event->type() == QEvent::PaletteChange
        || event->type() == QEvent::ApplicationPaletteChange) {
        refreshIcons();
    }
}

///
/// \brief Binds and lays out the log table.
///
void LogWidget::setupLogView()
{
    ui->logTable->setModel(_model);
    ui->logTable->verticalHeader()->hide();

    auto *header = ui->logTable->headerView();
    connect(header, &HeaderView::sectionAlignmentChanged, this,
            [this](int logicalIndex, Qt::Alignment alignment) {
                _model->setColumnAlignment(logicalIndex, alignment | Qt::AlignVCenter);
            });

    header->setStretchLastSection(false);
    header->setSectionResizeMode(LogModel::ColTimestamp, QHeaderView::Fixed);
    header->setSectionResizeMode(LogModel::ColLevel,     QHeaderView::Fixed);
    header->setSectionResizeMode(LogModel::ColSource,    QHeaderView::Fixed);
    header->setSectionResizeMode(LogModel::ColMessage,   QHeaderView::Stretch);

    header->setSectionAlignment(LogModel::ColLevel,  Qt::AlignCenter);
    header->setSectionAlignment(LogModel::ColSource, Qt::AlignCenter);

    ui->logTable->setColumnWidth(LogModel::ColTimestamp, 95);
    ui->logTable->setColumnWidth(LogModel::ColLevel,     80);
    ui->logTable->setColumnWidth(LogModel::ColSource,    60);
}

///
/// \brief Reloads the search, pause, and clear icons for the active theme.
///
void LogWidget::refreshIcons()
{
    const bool paused = ui->pauseButton->isChecked();
    _searchIconAction->setIcon(AppIcons::themed("search.svg"));
    ui->pauseButton->setIcon(paused ? QStringLiteral("resume.svg") : QStringLiteral("pause.svg"));
    ui->clearButton->setIcon(QStringLiteral("trash.svg"));
}

///
/// \brief Scrolls the log table to the latest entry.
///
void LogWidget::scrollToBottom()
{
    ui->logTable->scrollToBottom();
}
