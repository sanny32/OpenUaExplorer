// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file namespaceinspectordialog.cpp
/// \brief Implements the namespace inspector dialog.
///

#include <QFont>
#include <QFontMetrics>
#include <QHeaderView>
#include <QPainter>
#include <QPixmap>
#include <QShowEvent>
#include <QStandardItemModel>

#include "appcolors.h"
#include "fileexport.h"
#include "messageboxdialog.h"
#include "namespaceinspectordialog.h"
#include "opcua/opcuabackend.h"
#include "ui_namespaceinspectordialog.h"
#include "widgets/headerview.h"

namespace {

enum Column {
    ColumnIndex = 0,
    ColumnUri,
    ColumnNodes,
    ColumnKind,
    ColumnCount
};

constexpr int NamespaceIndexRole = Qt::UserRole + 1;

///
/// \brief Builds a small filled circle used as the Kind indicator.
/// \param color Circle fill colour.
/// \return Circle pixmap sized for a table cell.
///
QPixmap statusDot(const QColor &color)
{
    const int size = 12;
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawEllipse(1, 1, size - 2, size - 2);
    return pixmap;
}

} // namespace

///
/// \brief Builds the dialog and wires it to the backend.
/// \param service OPC UA backend used to read namespaces and crawl node counts.
/// \param cache Per-connection cache reused across dialog instances, or nullptr to disable caching.
/// \param parent Parent widget.
///
NamespaceInspectorDialog::NamespaceInspectorDialog(OpcUaBackend *service,
                                                   NamespaceInspectorCache *cache,
                                                   QWidget *parent)
    : AppBaseDialog(parent)
    , ui(new Ui::NamespaceInspectorDialog)
    , _service(service)
    , _cache(cache)
    , _model(new QStandardItemModel(this))
{
    ui->setupUi(this);

    _model->setColumnCount(ColumnCount);
    _model->setHorizontalHeaderLabels({ QStringLiteral("ns"), tr("URI"), tr("Nodes"), tr("Namespace") });
    ui->namespaceTable->setModel(_model);

    QHeaderView *header = ui->namespaceTable->horizontalHeader();
    header->setSectionResizeMode(ColumnIndex, QHeaderView::Interactive);
    header->setSectionResizeMode(ColumnUri, QHeaderView::Stretch);
    header->setSectionResizeMode(ColumnNodes, QHeaderView::Interactive);
    header->setSectionResizeMode(ColumnKind, QHeaderView::Interactive);

    QFont boldFont = ui->namespaceTable->font();
    boldFont.setBold(true);
    const QFontMetrics boldMetrics(boldFont);
    const int padding = 28;
    header->resizeSection(ColumnIndex, boldMetrics.horizontalAdvance(QStringLiteral("ns")) + padding);
    header->resizeSection(ColumnNodes,
        qMax(boldMetrics.horizontalAdvance(tr("Nodes")),
             boldMetrics.horizontalAdvance(QStringLiteral("99999"))) + padding);
    const int kindWidth = qMax(boldMetrics.horizontalAdvance(tr("Namespace")),
        qMax(boldMetrics.horizontalAdvance(tr("System")),
            qMax(boldMetrics.horizontalAdvance(tr("User")),
                 boldMetrics.horizontalAdvance(tr("User-defined")))));
    header->resizeSection(ColumnKind, kindWidth + padding + 20);
    if (HeaderView *headerView = ui->namespaceTable->headerView())
        headerView->setSectionAlignment(ColumnIndex, Qt::AlignCenter);

    ui->closeButton->setColors(
        { AppColors::accent(), AppColors::accentHover(), AppColors::accentPressed() });

    connect(ui->refreshButton, &QPushButton::clicked,
            this, &NamespaceInspectorDialog::refresh);
    connect(ui->exportButton, &QPushButton::clicked,
            this, &NamespaceInspectorDialog::exportNamespaces);
    connect(ui->closeButton, &QPushButton::clicked,
            this, &QDialog::accept);
    connect(ui->showSystemCheckBox, &QCheckBox::toggled,
            this, &NamespaceInspectorDialog::rebuildTable);
    connect(ui->namespaceTable->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, &NamespaceInspectorDialog::updateSelectedPanel);

    connect(_service, &OpcUaBackend::namespacesReady,
            this, &NamespaceInspectorDialog::handleNamespaces);
    connect(_service, &OpcUaBackend::namespaceStatisticsProgress,
            this, &NamespaceInspectorDialog::handleStatisticsProgress);
    connect(_service, &OpcUaBackend::namespaceStatisticsReady,
            this, &NamespaceInspectorDialog::handleStatistics);

    updateSelectedPanel();
}

///
/// \brief Destroys the dialog and its generated UI.
///
NamespaceInspectorDialog::~NamespaceInspectorDialog()
{
    delete ui;
}

///
/// \brief Triggers the first namespace load when the dialog is first shown.
/// \param event Show event.
///
void NamespaceInspectorDialog::showEvent(QShowEvent *event)
{
    AppBaseDialog::showEvent(event);
    if (!_loadedOnce && !event->spontaneous()) {
        _loadedOnce = true;
        if (_cache && _cache->valid)
            loadFromCache();
        else
            refresh();
    }
}

///
/// \brief Cancels any running crawl before the dialog closes.
/// \param result Dialog result code.
///
void NamespaceInspectorDialog::done(int result)
{
    if (_crawling) {
        _crawlAborted = true;
        _service->cancelNamespaceStatistics();
    }
    AppBaseDialog::done(result);
}

///
/// \brief Populates the table from the cached namespace data without crawling.
///
void NamespaceInspectorDialog::loadFromCache()
{
    _uris = _cache->uris;
    _counts = _cache->counts;
    _countsReady = true;
    _crawling = false;
    setControlsBusy(false);
    ui->statusLabel->clear();
    rebuildTable();
}

///
/// \brief Reloads the namespace table and restarts the node-count crawl.
///
void NamespaceInspectorDialog::refresh()
{
    if (_crawling) {
        _crawlAborted = true;
        _service->cancelNamespaceStatistics();
    }
    _crawling = false;
    _countsReady = false;
    _counts.clear();
    setControlsBusy(true);
    ui->statusLabel->setText(tr("Loading namespaces..."));
    ui->statusLabel->repaint();
    _service->requestNamespaces();
}

///
/// \brief Applies the namespace URIs, or shows the read error.
/// \param namespaces Namespace URIs indexed by namespace index.
/// \param error Error description, empty on success.
///
void NamespaceInspectorDialog::handleNamespaces(const QStringList &namespaces, const QString &error)
{
    if (!error.isEmpty()) {
        _uris.clear();
        rebuildTable();
        setControlsBusy(false);
        ui->statusLabel->setText(error);
        return;
    }
    _uris = namespaces;
    startCrawl();
    rebuildTable();
}

///
/// \brief Begins the address-space crawl that fills the node counts.
///
void NamespaceInspectorDialog::startCrawl()
{
    _crawling = true;
    _crawlAborted = false;
    _countsReady = false;
    setControlsBusy(true);
    ui->statusLabel->setText(tr("Counting nodes..."));
    ui->statusLabel->repaint();
    _progressClock.restart();
    _service->requestNamespaceStatistics();
}

///
/// \brief Updates the status line with the running crawl progress.
/// \param visitedNodes Number of unique nodes visited so far.
///
void NamespaceInspectorDialog::handleStatisticsProgress(int visitedNodes)
{
    if (!_crawling)
        return;
    if (_progressClock.isValid() && _progressClock.elapsed() < 100)
        return;
    _progressClock.restart();
    ui->statusLabel->setText(tr("Counting nodes... %1").arg(visitedNodes));
    ui->statusLabel->repaint();
}

///
/// \brief Applies the crawl results to the Nodes column.
/// \param nodeCounts Node counts keyed by namespace index.
/// \param error Error description, empty on success.
///
void NamespaceInspectorDialog::handleStatistics(const OpcUaNamespaceNodeCounts &nodeCounts,
                                                const QString &error)
{
    if (_crawlAborted) {
        _crawlAborted = false;
        return;
    }
    _crawling = false;
    _counts = nodeCounts;
    _countsReady = error.isEmpty();
    setControlsBusy(false);
    if (_countsReady && _cache) {
        _cache->uris = _uris;
        _cache->counts = _counts;
        _cache->valid = true;
    }
    rebuildTable();
    ui->statusLabel->setText(error.isEmpty() ? QString() : error);
}

///
/// \brief Rebuilds the table rows for the current namespaces and filter.
///
void NamespaceInspectorDialog::rebuildTable()
{
    const int restoreIndex = _selectedNamespaceIndex;
    _model->removeRows(0, _model->rowCount());

    const bool showSystem = ui->showSystemCheckBox->isChecked();
    for (int index = 0; index < _uris.size(); ++index) {
        const QString uri = _uris.at(index);
        const bool system = isSystemNamespace(index, uri);
        if (system && !showSystem)
            continue;

        auto *indexItem = new QStandardItem(QString::number(index));
        indexItem->setData(index, NamespaceIndexRole);
        indexItem->setTextAlignment(Qt::AlignCenter);

        auto *uriItem = new QStandardItem(uri);

        QString nodesText;
        if (_countsReady)
            nodesText = QString::number(_counts.value(index, 0));
        else if (_crawling)
            nodesText = QStringLiteral("...");
        auto *nodesItem = new QStandardItem(nodesText);

        auto *kindItem = new QStandardItem(system ? tr("System") : tr("User"));
        kindItem->setData(statusDot(system ? QColor(0x9c, 0x9c, 0x9c) : QColor(0x22, 0xc5, 0x5e)),
                          Qt::DecorationRole);

        _model->appendRow({ indexItem, uriItem, nodesItem, kindItem });
    }

    if (restoreIndex >= 0) {
        for (int row = 0; row < _model->rowCount(); ++row) {
            if (_model->item(row, ColumnIndex)->data(NamespaceIndexRole).toInt() == restoreIndex) {
                ui->namespaceTable->selectRow(row);
                break;
            }
        }
    }
    if (!ui->namespaceTable->selectionModel()->currentIndex().isValid() && _model->rowCount() > 0)
        ui->namespaceTable->selectRow(0);
    updateSelectedPanel();
}

///
/// \brief Refreshes the selected-namespace panel from the current selection.
///
void NamespaceInspectorDialog::updateSelectedPanel()
{
    const int index = selectedNamespaceIndex();
    _selectedNamespaceIndex = index;
    const bool hasSelection = index >= 0 && index < _uris.size();

    if (!hasSelection) {
        ui->indexValue->clear();
        ui->uriValue->clear();
        ui->nodesValue->clear();
        ui->kindValue->clear();
        return;
    }

    const QString uri = _uris.at(index);
    ui->indexValue->setText(QString::number(index));
    ui->uriValue->setText(uri);
    if (_countsReady)
        ui->nodesValue->setText(QString::number(_counts.value(index, 0)));
    else if (_crawling)
        ui->nodesValue->setText(tr("counting..."));
    else
        ui->nodesValue->clear();
    ui->kindValue->setText(isSystemNamespace(index, uri) ? tr("System") : tr("User-defined"));
}

///
/// \brief Exports the full namespace table to a CSV file.
///
void NamespaceInspectorDialog::exportNamespaces()
{
    if (_uris.isEmpty())
        return;

    QString csv = QStringLiteral("Index,URI,Nodes,Kind\n");
    for (int index = 0; index < _uris.size(); ++index) {
        const QString uri = _uris.at(index);
        const QString nodes = _countsReady ? QString::number(_counts.value(index, 0)) : QString();
        const QString kind = isSystemNamespace(index, uri) ? QStringLiteral("System")
                                                           : QStringLiteral("User");
        QString quotedUri = uri;
        quotedUri.replace(QLatin1Char('"'), QLatin1String("\"\""));
        csv += QStringLiteral("%1,\"%2\",%3,%4\n")
                   .arg(QString::number(index), quotedUri, nodes, kind);
    }

    FileExport::saveText(this, tr("Export Namespaces"), QStringLiteral("namespaces.csv"),
                         FileExport::csvFilter(), csv);
}

///
/// \brief Enables or disables controls while a load or crawl is in progress.
/// \param busy True while the dialog is loading namespaces or counting nodes.
///
void NamespaceInspectorDialog::setControlsBusy(bool busy)
{
    ui->refreshButton->setEnabled(!busy);
}

///
/// \brief Returns the namespace index of the current selection, or -1 when none.
/// \return Selected namespace index, or -1.
///
int NamespaceInspectorDialog::selectedNamespaceIndex() const
{
    const QModelIndex current = ui->namespaceTable->selectionModel()->currentIndex();
    if (!current.isValid())
        return -1;
    return _model->item(current.row(), ColumnIndex)->data(NamespaceIndexRole).toInt();
}

///
/// \brief Classifies a namespace as system-defined or user-defined.
/// \param index Namespace index.
/// \param uri Namespace URI.
/// \return True for the base namespace and well-known OPC Foundation namespaces.
///
bool NamespaceInspectorDialog::isSystemNamespace(int index, const QString &uri)
{
    return index == 0 || uri.contains(QLatin1String("opcfoundation.org"), Qt::CaseInsensitive);
}
