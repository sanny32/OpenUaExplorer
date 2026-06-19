// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file logwidget.cpp
/// \brief Implements the activity log widget.
///

#include <algorithm>
#include <atomic>
#include <QAbstractItemView>
#include <QClipboard>
#include <QComboBox>
#include <QDateTime>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QGuiApplication>
#include <QMenu>
#include <QMessageBox>
#include <QMetaObject>
#include <QPointer>
#include <QSignalBlocker>
#include <QTextStream>

#include "appicons.h"
#include "appsettings.h"
#include "headerview.h"
#include "loggingcategories.h"
#include "logwidget.h"
#include "themedaction.h"
#include "models/logmodel.h"
#include "tableview.h"
#include "ui_logwidget.h"
#include "utils.h"

namespace {
static LogWidget*       s_instance    = nullptr;
static QtMessageHandler s_prevHandler = nullptr;
static std::atomic<quint64> s_logEpoch { 0 };

///
/// \brief Derives the Source column label from a forwarded log category.
/// \param category Full logging category name.
/// \return Short source label, or an empty string for categories that are not forwarded.
///
static QString sourceForCategory(const QString &category)
{
    const QLatin1String appPrefix("ouaexp.");
    if (category.startsWith(appPrefix))
        return category.mid(appPrefix.size());
    if (category.startsWith(QLatin1String("qt.opcua")))
        return category.section(QLatin1Char('.'), -1);
    return QString();
}

///
/// \brief Qt message handler that forwards application and Qt OPC UA log messages to the active LogWidget.
/// \param type Qt message severity.
/// \param ctx Log context carrying the category.
/// \param msg Message text.
///
static void appMessageHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg)
{
    if (s_prevHandler) s_prevHandler(type, ctx, msg);
    if (!s_instance) return;

    const QString category = QString::fromLatin1(ctx.category ? ctx.category : "");
    const QString source = sourceForCategory(category);
    if (source.isEmpty()) return;

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

    ui->sourceCombo->setMinimumContentsLength(14);
    ui->sourceCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    ui->sourceCombo->view()->setTextElideMode(Qt::ElideNone);

    _searchIconAction = new ThemedAction(QStringLiteral("search"), QString(), this);
    ui->searchEdit->addAction(_searchIconAction, QLineEdit::LeadingPosition);

    _copyAction = new ThemedAction(QStringLiteral("copy"), tr("Copy"), this);
    _copyAction->setShortcut(QKeySequence::Copy);
    _copyAction->setShortcutContext(Qt::WidgetShortcut);
    connect(_copyAction, &QAction::triggered, this, &LogWidget::copySelection);
    ui->logTable->addAction(_copyAction);

    _copyAllAction = new ThemedAction(QStringLiteral("copy"), tr("Copy All"), this);
    connect(_copyAllAction, &QAction::triggered, this, &LogWidget::copyAll);
    ui->logTable->addAction(_copyAllAction);

    ui->logTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->logTable, &QWidget::customContextMenuRequested,
            this, &LogWidget::showContextMenu);

    refreshIcons();

    s_instance = this;
    s_prevHandler = qInstallMessageHandler(appMessageHandler);

    connect(ui->clearButton, &QPushButton::clicked, this, [this]() {
        s_logEpoch.fetch_add(1, std::memory_order_relaxed);
        _model->clear();
        _knownSources.clear();
        const QSignalBlocker blocker(ui->sourceCombo);
        ui->sourceCombo->clear();
        ui->sourceCombo->addItem(tr("All"));
        _model->setSourceFilter(QString());
    });

    connect(ui->exportButton, &QPushButton::clicked, this, [this]() {
        exportLog();
    });

    connect(ui->pauseButton, &QPushButton::toggled, this, [this](bool checked) {
        _paused = checked;
        ui->pauseButton->setIcon(checked ? QStringLiteral("resume") : QStringLiteral("pause"));
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

    connect(ui->sourceCombo, &QComboBox::currentTextChanged, this, [this](const QString &text) {
        _model->setSourceFilter(ui->sourceCombo->currentIndex() <= 0 ? QString() : text);
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
    if (_paused)
        return;
    registerSource(item.source);
    _model->addItem(item);
}

///
/// \brief Persists the log table header state.
/// \param settings Settings store to write to.
///
void LogWidget::saveViewState(AppSettings &settings) const
{
    settings.setViewState(ui->logTable->objectName(), ui->logTable->headerView()->saveLayout());
}

///
/// \brief Restores the log table header state.
/// \param settings Settings store to read from.
///
void LogWidget::restoreViewState(AppSettings &settings)
{
    ui->logTable->headerView()->restoreLayout(settings.viewState(ui->logTable->objectName()));
}

///
/// \brief Adds a newly seen source to the source filter combo, keeping the list sorted.
/// \param source Source label to register.
///
void LogWidget::registerSource(const QString &source)
{
    if (source.isEmpty() || _knownSources.contains(source))
        return;
    _knownSources.insert(source);

    int row = 1;
    while (row < ui->sourceCombo->count()
           && ui->sourceCombo->itemText(row).compare(source, Qt::CaseInsensitive) < 0)
        ++row;

    const QSignalBlocker blocker(ui->sourceCombo);
    ui->sourceCombo->insertItem(row, source);
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
    header->setSectionResizeMode(LogModel::ColTimestamp, QHeaderView::Interactive);
    header->setSectionResizeMode(LogModel::ColLevel,     QHeaderView::Interactive);
    header->setSectionResizeMode(LogModel::ColSource,    QHeaderView::Interactive);
    header->setSectionResizeMode(LogModel::ColMessage,   QHeaderView::Stretch);

    header->setSectionAlignment(LogModel::ColLevel,  Qt::AlignCenter);
    header->setSectionAlignment(LogModel::ColSource, Qt::AlignCenter);

    ui->logTable->setColumnWidth(LogModel::ColTimestamp, 95);
    ui->logTable->setColumnWidth(LogModel::ColLevel,     80);
    ui->logTable->setColumnWidth(LogModel::ColSource,    110);
}

///
/// \brief Reloads the search, pause, and clear icons for the active theme.
///
void LogWidget::refreshIcons()
{
    const bool paused = ui->pauseButton->isChecked();
    ui->pauseButton->setIcon(paused ? QStringLiteral("resume") : QStringLiteral("pause"));
    ui->clearButton->setIcon(QStringLiteral("trash"));
    ui->exportButton->setIcon(QStringLiteral("export"));
}

///
/// \brief Scrolls the log table to the latest entry.
///
void LogWidget::scrollToBottom()
{
    ui->logTable->scrollToBottom();
}

///
/// \brief Writes the currently visible log rows to a user-chosen text file.
///
/// Honours the active level, source and search filters, exporting exactly the
/// rows shown in the table.
///
void LogWidget::exportLog()
{
    const QString fileName = QFileDialog::getSaveFileName(
        this, tr("Export Log"),
        Utils::executableBaseName() + QStringLiteral(".log"),
        tr("Log files (*.log);;Text files (*.txt);;All files (*)"));
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Export Log"),
                             tr("Could not open the file for writing:\n%1")
                                 .arg(file.errorString()));
        return;
    }

    QTextStream stream(&file);
    const int rows = _model->rowCount();
    for (int row = 0; row < rows; ++row) {
        stream << _model->index(row, LogModel::ColTimestamp).data().toString() << '\t'
               << _model->index(row, LogModel::ColLevel).data().toString() << '\t'
               << _model->index(row, LogModel::ColSource).data().toString() << '\t'
               << _model->index(row, LogModel::ColMessage).data().toString() << '\n';
    }
}

///
/// \brief Copies the given log rows to the clipboard as tab-separated text.
/// \param rows Row indices to copy, in the order they should appear.
///
void LogWidget::copyRows(const QList<int> &rows)
{
    if (rows.isEmpty())
        return;

    QStringList lines;
    lines.reserve(rows.size());
    for (const int row : rows) {
        const QStringList cells = {
            _model->index(row, LogModel::ColTimestamp).data().toString(),
            _model->index(row, LogModel::ColLevel).data().toString(),
            _model->index(row, LogModel::ColSource).data().toString(),
            _model->index(row, LogModel::ColMessage).data().toString()
        };
        lines.append(cells.join(QLatin1Char('\t')));
    }

    QGuiApplication::clipboard()->setText(lines.join(QLatin1Char('\n')));
}

///
/// \brief Copies the selected log rows to the clipboard.
///
void LogWidget::copySelection()
{
    QList<int> rows;
    const QModelIndexList selected = ui->logTable->selectionModel()->selectedRows();
    rows.reserve(selected.size());
    for (const QModelIndex &index : selected)
        rows.append(index.row());
    std::sort(rows.begin(), rows.end());

    copyRows(rows);
}

///
/// \brief Copies every visible log row to the clipboard.
///
void LogWidget::copyAll()
{
    const int count = _model->rowCount();
    QList<int> rows;
    rows.reserve(count);
    for (int row = 0; row < count; ++row)
        rows.append(row);

    copyRows(rows);
}

///
/// \brief Shows the log table context menu at the requested position.
/// \param pos Position in the table viewport's coordinates.
///
void LogWidget::showContextMenu(const QPoint &pos)
{
    _copyAction->setEnabled(ui->logTable->selectionModel()->hasSelection());
    _copyAllAction->setEnabled(_model->rowCount() > 0);

    QMenu menu(this);
    menu.addAction(_copyAction);
    menu.addAction(_copyAllAction);
    menu.exec(ui->logTable->viewport()->mapToGlobal(pos));
}
