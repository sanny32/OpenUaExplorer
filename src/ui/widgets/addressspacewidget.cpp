// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file addressspacewidget.cpp
/// \brief Implements the OPC UA address space browser widget.
///

#include <functional>

#include <QAbstractItemView>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>

#include "addressspacewidget.h"
#include "appicons.h"
#include "appsettings.h"
#include "headerview.h"
#include "models/addressspacemodel.h"
#include "models/nodeinfomodel.h"
#include "models/referencesmodel.h"
#include "spinneraction.h"
#include "tableview.h"
#include "ui_addressspacewidget.h"

///
/// \brief Builds the browser, wiring its tree, node-info, and references views.
/// \param parent Parent widget.
///
AddressSpaceWidget::AddressSpaceWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AddressSpaceWidget)
    , _treeModel(new AddressSpaceModel(this))
    , _nodeInfoModel(new NodeInfoModel(this))
    , _referencesModel(new ReferencesModel(this))
{
    ui->setupUi(this);

    setupTreeView();
    setupNodeInfoView();
    setupReferencesView();
    setupSearch();

    connect(_treeModel, &AddressSpaceModel::browseRequested,
            this, &AddressSpaceWidget::browseRequested);
    connect(ui->addressTree->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &AddressSpaceWidget::onCurrentNodeChanged);
    connect(ui->refreshButton, &QPushButton::clicked, this, [this]() {
        emit refreshRequested(_selectedNodeId);
        if (!_selectedNodeId.isEmpty())
            emit referencesRequested(_selectedNodeId);
    });

    _treeModel->setIconProvider([this](AddressSpaceItem::NodeType type) {
        switch (type) {
        case AddressSpaceItem::NodeType::Folder:   return AppIcons::themed("folder");
        case AddressSpaceItem::NodeType::Node:     return AppIcons::themed("node");
        case AddressSpaceItem::NodeType::Variable: return AppIcons::themed("variable");
        case AddressSpaceItem::NodeType::Method:   return AppIcons::themed("method");
        }
        return QIcon();
    });

    ui->addressTree->expandAll();

    ui->refreshButton->setIcon(QStringLiteral("refresh"));
    ui->refreshButton->setToolTip(QStringLiteral("Refresh"));
    ui->refreshButton->setText(QString());
    ui->splitter->setSizes({455, 255});
}

///
/// \brief Sets the tree's root node and selects it.
/// \param root Root folder information.
///
void AddressSpaceWidget::setRootNode(const OpcUaNodeInfo &root)
{
    _referencesByNodeId.clear();
    _referencesModel->clear();
    _treeModel->setRootNode(root);
    const QModelIndex rootIndex = _treeModel->index(0, 0);
    ui->addressTree->setCurrentIndex(rootIndex);
    ui->addressTree->expand(rootIndex);
}

///
/// \brief Applies browse results to the tree.
/// \param parentNodeId Parent NodeId.
/// \param children Browse result.
/// \param error Browse error.
///
void AddressSpaceWidget::setBrowseChildren(const QString &parentNodeId,
                                           const QVector<OpcUaNodeInfo> &children,
                                           const QString &error)
{
    if (!error.isEmpty()) {
        _treeModel->setBrowseFailed(parentNodeId);
        return;
    }
    _treeModel->setChildren(parentNodeId, children);
    if (!_pendingExpand.isEmpty() || !_pendingSelect.isEmpty())
        applyPendingExpansion();
}

///
/// \brief Applies reference browse results to the references view if selected.
/// \param sourceNodeId Source NodeId.
/// \param references Reference browse result.
/// \param error Browse error.
///
void AddressSpaceWidget::setBrowseReferences(const QString &sourceNodeId,
                                             const QVector<OpcUaNodeInfo> &references,
                                             const QString &error)
{
    if (!error.isEmpty()) {
        _referencesByNodeId.remove(sourceNodeId);
        if (_selectedNodeId == sourceNodeId)
            _referencesModel->clear();
        return;
    }
    _referencesByNodeId.insert(sourceNodeId, referencesFromChildren(references));
    if (_selectedNodeId == sourceNodeId)
        updateReferencesForNode(sourceNodeId);
}

///
/// \brief Populates the node-info table from the selected node's details.
/// \param details Selected node details.
///
void AddressSpaceWidget::setNodeDetails(const OpcUaNodeDetails &details)
{
    QVector<NodeInfoItem> items;
    items.append({tr("Node Id"), details.nodeId});
    items.append({tr("Display Name"), details.displayName});
    items.append({tr("Node Class"), QString::number(details.nodeClass)});
    items.append({tr("Data Type"), details.dataTypeId});
    items.append({tr("Value Rank"), QString::number(details.valueRank)});
    items.append({tr("Status"), details.status});
    _nodeInfoModel->setItems(items);
}

///
/// \brief Clears the tree, node-info, and references views.
///
void AddressSpaceWidget::clear()
{
    _searching = false;
    _searchPattern.clear();
    _searchMatched = false;
    _selectedNodeId.clear();
    _subscribedNodeIds.clear();
    _referencesByNodeId.clear();
    _pendingExpand.clear();
    _pendingSelect.clear();
    ui->searchEdit->clear();
    setSearchBusy(false);
    setSearchFailure(QString());
    _treeModel->clear();
    _nodeInfoModel->clear();
    _referencesModel->clear();
}

///
/// \brief Updates the tracked monitoring state shown in the context menu.
/// \param nodeId Affected node.
/// \param subscribed True when the node is being monitored.
///
void AddressSpaceWidget::setNodeSubscribed(const QString &nodeId, bool subscribed)
{
    if (subscribed)
        _subscribedNodeIds.insert(nodeId);
    else
        _subscribedNodeIds.remove(nodeId);
}

///
/// \brief Returns the node currently selected in the tree.
/// \return Currently selected OPC UA node.
///
OpcUaNodeInfo AddressSpaceWidget::selectedNode() const
{
    return _treeModel->nodeInfo(ui->addressTree->currentIndex());
}

///
/// \brief Returns the node ids of the expanded tree items, parents before children.
/// \return Expanded node ids in top-down order.
///
QStringList AddressSpaceWidget::expandedNodeIds() const
{
    QStringList result;
    std::function<void(const QModelIndex &)> walk = [&](const QModelIndex &parent) {
        const int rows = _treeModel->rowCount(parent);
        for (int row = 0; row < rows; ++row) {
            const QModelIndex index = _treeModel->index(row, 0, parent);
            if (!ui->addressTree->isExpanded(index))
                continue;
            const QString nodeId = _treeModel->nodeInfo(index).nodeId;
            if (!nodeId.isEmpty())
                result.append(nodeId);
            walk(index);
        }
    };
    walk(QModelIndex());
    return result;
}

///
/// \brief Re-expands saved tree nodes and reselects a node as they load.
/// \param expandedNodeIds Node ids to expand, parents before children.
/// \param selectedNodeId Node id to select once it is loaded, or empty.
///
void AddressSpaceWidget::restoreExpansion(const QStringList &expandedNodeIds,
                                          const QString &selectedNodeId)
{
    _pendingExpand = expandedNodeIds;
    _pendingSelect = selectedNodeId;
    applyPendingExpansion();
}

///
/// \brief Expands the loaded pending nodes and selects the pending node when available.
///
/// Expanding a node triggers a lazy browse of its children; as those results arrive
/// through setBrowseChildren() this runs again to expand the next level down.
///
void AddressSpaceWidget::applyPendingExpansion()
{
    for (int i = _pendingExpand.size() - 1; i >= 0; --i) {
        const QModelIndex index = _treeModel->findByNodeId(_pendingExpand.at(i));
        if (!index.isValid())
            continue;
        ui->addressTree->expand(index);
        if (_treeModel->canFetchMore(index))
            _treeModel->fetchMore(index);
        _pendingExpand.removeAt(i);
    }

    if (!_pendingSelect.isEmpty()) {
        const QModelIndex index = _treeModel->findByNodeId(_pendingSelect);
        if (index.isValid()) {
            ui->addressTree->setCurrentIndex(index);
            ui->addressTree->scrollTo(index);
            _pendingSelect.clear();
        }
    }
}

///
/// \brief Detaches the node details panel so MainWindow can host it in a dock.
/// \return Node details panel.
///
QWidget *AddressSpaceWidget::takeNodeDetailsPanel()
{
    ui->nodeInfoPanel->setParent(nullptr);
    ui->splitter->setSizes({1});
    return ui->nodeInfoPanel;
}

///
/// \brief Persists the tree, node-info, and references header state.
/// \param settings Settings store to write to.
///
void AddressSpaceWidget::saveViewState(AppSettings &settings) const
{
    settings.setViewState(ui->addressTree->objectName(), ui->addressTree->header()->saveState());
    settings.setViewState(ui->nodeInfoTable->objectName(),
                          ui->nodeInfoTable->headerView()->saveLayout());
    settings.setViewState(ui->referencesTable->objectName(),
                          ui->referencesTable->headerView()->saveLayout());
}

///
/// \brief Restores the tree, node-info, and references header state.
/// \param settings Settings store to read from.
///
void AddressSpaceWidget::restoreViewState(AppSettings &settings)
{
    const QByteArray treeState = settings.viewState(ui->addressTree->objectName());
    if (!treeState.isEmpty())
        ui->addressTree->header()->restoreState(treeState);
    ui->nodeInfoTable->headerView()->restoreLayout(settings.viewState(ui->nodeInfoTable->objectName()));
    ui->referencesTable->headerView()->restoreLayout(
        settings.viewState(ui->referencesTable->objectName()));
}

///
/// \brief Destroys the widget and its generated UI.
///
AddressSpaceWidget::~AddressSpaceWidget()
{
    delete ui;
}

///
/// \brief Binds the tree view to the address-space model.
///
void AddressSpaceWidget::setupTreeView()
{
    ui->addressTree->setModel(_treeModel);
    ui->addressTree->setDragEnabled(true);
    ui->addressTree->setDragDropMode(QAbstractItemView::DragOnly);
    ui->addressTree->setDefaultDropAction(Qt::CopyAction);
    ui->addressTree->setHeaderHidden(true);
    ui->addressTree->setUniformRowHeights(true);
    ui->addressTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->addressTree, &QWidget::customContextMenuRequested,
            this, &AddressSpaceWidget::showTreeContextMenu);
}

///
/// \brief Shows the tree context menu with node actions for the clicked node.
/// \param pos Cursor position in the tree's viewport coordinates.
///
void AddressSpaceWidget::showTreeContextMenu(const QPoint &pos)
{
    const QModelIndex index = ui->addressTree->indexAt(pos);
    if (!index.isValid())
        return;
    const OpcUaNodeInfo info = _treeModel->nodeInfo(index);
    if (info.nodeId.isEmpty())
        return;

    QMenu menu(this);

    if (OpcUa::isMethod(info.nodeClass)) {
        const OpcUaNodeInfo object = _treeModel->nodeInfo(index.parent());
        menu.addAction(AppIcons::themed(QStringLiteral("method")),
                       tr("Call..."), this, [this, object, info] {
            emit callMethodRequested(object, info);
        });
        menu.addSeparator();
    }

    const bool subscribed = _subscribedNodeIds.contains(info.nodeId);
    const QString monitoringIcon = subscribed ? QStringLiteral("unsubscribe")
                                              : QStringLiteral("subscribe");
    QAction *monitoringAction = menu.addAction(AppIcons::themed(monitoringIcon),
                                               subscribed ? tr("Unsubscribe") : tr("Subscribe"),
                                               this, [this, info, subscribed] {
        if (subscribed)
            emit unsubscribeRequested(info);
        else
            emit subscribeRequested(info);
    });
    monitoringAction->setEnabled(OpcUa::isVariable(info.nodeClass));

    QAction *trendAction = menu.addAction(AppIcons::themed(QStringLiteral("trend")),
                                          tr("Add to Trend"), this, [this, info] {
        emit addToTrendRequested(info);
    });
    trendAction->setEnabled(OpcUa::isVariable(info.nodeClass));

    QAction *monitorAction = menu.addAction(AppIcons::themed(QStringLiteral("trend")),
                                            tr("Monitor Node..."), this, [this, info] {
        emit monitorNodeRequested(info);
    });
    monitorAction->setEnabled(OpcUa::isVariable(info.nodeClass));

    if (OpcUa::isHistoryReadSupported()) {
        QAction *historyAction = menu.addAction(AppIcons::themed(QStringLiteral("history")),
                                                tr("Read Data History"), this, [this, info] {
            emit readHistoryRequested(info);
        });
        historyAction->setEnabled(OpcUa::isVariable(info.nodeClass));
    }

    QAction *eventsHistoryAction = menu.addAction(AppIcons::themed(QStringLiteral("event-history")),
                                                  tr("Read Events History"), this, [this, info] {
        emit readEventsHistoryRequested(info);
    });
    eventsHistoryAction->setEnabled(info.nodeClass == OpcUa::Object);

    QAction *eventsAction = menu.addAction(AppIcons::themed(QStringLiteral("event")),
                                           tr("Monitor Events"), this, [this, info] {
        emit monitorEventsRequested(info);
    });
    eventsAction->setEnabled(info.nodeClass == OpcUa::Object);

    menu.exec(ui->addressTree->viewport()->mapToGlobal(pos));
}

///
/// \brief Binds and lays out the node-info table.
///
void AddressSpaceWidget::setupNodeInfoView()
{
    ui->nodeInfoTable->setModel(_nodeInfoModel);
    ui->nodeInfoTable->horizontalHeader()->hide();
    ui->nodeInfoTable->verticalHeader()->hide();
    ui->nodeInfoTable->horizontalHeader()->setStretchLastSection(true);
    ui->nodeInfoTable->setColumnWidth(NodeInfoModel::ColLabel, 105);
}

///
/// \brief Binds and lays out the references table.
///
void AddressSpaceWidget::setupReferencesView()
{
    ui->referencesTable->setModel(_referencesModel);
    ui->referencesTable->verticalHeader()->hide();
    ui->referencesTable->horizontalHeader()->setStretchLastSection(true);
    ui->referencesTable->setColumnWidth(ReferencesModel::ColReference, 150);
}

///
/// \brief Wires the search box to the server-side address-space search.
///
void AddressSpaceWidget::setupSearch()
{
    _searchSpinner = new SpinnerAction(ui->searchEdit);
    ui->searchEdit->addAction(_searchSpinner, QLineEdit::TrailingPosition);
    setSearchBusy(false);
    connect(ui->searchEdit, &QLineEdit::returnPressed, this, &AddressSpaceWidget::startSearch);
    connect(ui->searchEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        setSearchFailure(QString());
        if (text.trimmed() != _searchPattern)
            cancelSearch();
    });
}

///
/// \brief Asks the server to search the tree root's subtree for the typed display name.
///
/// Pressing Return again with an unchanged pattern continues the crawl from the last
/// match instead of restarting it, so repeated presses walk the matches in turn.
///
void AddressSpaceWidget::startSearch()
{
    const QString pattern = ui->searchEdit->text().trimmed();
    if (pattern.isEmpty() || _searching)
        return;

    const QString startNodeId = _treeModel->nodeInfo(_treeModel->index(0, 0)).nodeId;
    if (startNodeId.isEmpty()) {
        setSearchFailure(tr("Connect to a server before searching."));
        return;
    }

    _searching = true;
    _searchPattern = pattern;
    setSearchFailure(QString());
    setSearchBusy(true);
    emit searchRequested(startNodeId, pattern);
}

///
/// \brief Abandons the running or paused search so the server drops its crawl state.
///
void AddressSpaceWidget::cancelSearch()
{
    setSearchBusy(false);
    const bool hadSearch = _searching || !_searchPattern.isEmpty();
    _searching = false;
    _searchPattern.clear();
    _searchMatched = false;
    if (hadSearch)
        emit searchCancelRequested();
}

///
/// \brief Swaps the search box between its clear button and the busy spinner.
///
/// Both occupy the trailing icon slot, so showing them together crowds the box and
/// shifts the text margin; only one is ever enabled.
/// \param busy True while a search runs.
///
void AddressSpaceWidget::setSearchBusy(bool busy)
{
    ui->searchEdit->setClearButtonEnabled(!busy);
    if (busy)
        _searchSpinner->start();
    else
        _searchSpinner->stop();
}

///
/// \brief Explains a failed search through the search box tooltip.
/// \param text Message to show, or empty to drop the previous message.
///
void AddressSpaceWidget::setSearchFailure(const QString &text)
{
    ui->searchEdit->setToolTip(text);
}

///
/// \brief Reports how many nodes the running search has visited.
/// \param visitedNodes Number of unique nodes visited so far.
///
void AddressSpaceWidget::setSearchProgress(int visitedNodes)
{
    if (!_searching)
        return;
    ui->searchEdit->setToolTip(tr("Searching... %n node(s) visited", nullptr, visitedNodes));
}

///
/// \brief Reveals the search match, or reports that the search found nothing.
///
/// The match is revealed through the pending-expansion machinery: its ancestors are
/// expanded as their browse results arrive, and the match is selected once loaded.
/// The crawl stays paused on the match so the next Return continues from it.
/// \param ancestorNodeIds Node ids from the search root down to the match's parent.
/// \param nodeId Matched NodeId, empty when no further node matched.
/// \param error Search error, empty on success.
///
void AddressSpaceWidget::setSearchResult(const QStringList &ancestorNodeIds, const QString &nodeId,
                                         const QString &error)
{
    if (!_searching)
        return;
    _searching = false;
    setSearchBusy(false);

    if (!error.isEmpty()) {
        _searchPattern.clear();
        _searchMatched = false;
        setSearchFailure(tr("Search failed: %1").arg(error));
        return;
    }
    if (nodeId.isEmpty()) {
        const QString pattern = _searchPattern;
        const bool exhausted = _searchMatched;
        _searchPattern.clear();
        _searchMatched = false;
        setSearchFailure(exhausted ? tr("No more nodes matching '%1'.").arg(pattern)
                                   : tr("No node matching '%1'.").arg(pattern));
        return;
    }

    _searchMatched = true;
    setSearchFailure(QString());
    restoreExpansion(ancestorNodeIds, nodeId);
}

///
/// \brief Applies the newly selected tree node to the detail views.
/// \param current Selected tree index.
///
void AddressSpaceWidget::onCurrentNodeChanged(const QModelIndex &current)
{
    const OpcUaNodeInfo info = _treeModel->nodeInfo(current);
    _selectedNodeId = info.nodeId;
    updateReferencesForNode(info.nodeId);
    if (!info.nodeId.isEmpty()) {
        emit referencesRequested(info.nodeId);
        emit nodeSelected(info);
    }
}

///
/// \brief Shows cached browse references for a node or clears stale rows.
/// \param nodeId Selected node NodeId.
///
void AddressSpaceWidget::updateReferencesForNode(const QString &nodeId)
{
    const auto references = _referencesByNodeId.constFind(nodeId);
    if (references == _referencesByNodeId.constEnd()) {
        _referencesModel->clear();
        return;
    }
    _referencesModel->setItems(*references);
}

///
/// \brief Converts browse children to reference table rows.
/// \param children Browse result children.
/// \return Reference rows preserving duplicate targets with different reference types.
///
QVector<ReferenceItem> AddressSpaceWidget::referencesFromChildren(
    const QVector<OpcUaNodeInfo> &children) const
{
    QVector<ReferenceItem> references;
    references.reserve(children.size());
    for (const OpcUaNodeInfo &child : children) {
        QString target = child.displayName;
        if (target.isEmpty())
            target = child.browseName;
        if (target.isEmpty())
            target = child.nodeId;
        references.append({referenceTypeDisplayName(child.referenceTypeId), target});
    }
    return references;
}

///
/// \brief Returns a standard OPC UA reference type name when known.
/// \param referenceTypeId Reference type NodeId.
/// \return Display name or the original NodeId.
///
QString AddressSpaceWidget::referenceTypeDisplayName(const QString &referenceTypeId) const
{
    static const QHash<QString, QString> names = {
        {QStringLiteral("ns=0;i=31"), QStringLiteral("References")},
        {QStringLiteral("ns=0;i=32"), QStringLiteral("NonHierarchicalReferences")},
        {QStringLiteral("ns=0;i=33"), QStringLiteral("HierarchicalReferences")},
        {QStringLiteral("ns=0;i=34"), QStringLiteral("HasChild")},
        {QStringLiteral("ns=0;i=35"), QStringLiteral("Organizes")},
        {QStringLiteral("ns=0;i=36"), QStringLiteral("HasEventSource")},
        {QStringLiteral("ns=0;i=37"), QStringLiteral("HasModellingRule")},
        {QStringLiteral("ns=0;i=38"), QStringLiteral("HasEncoding")},
        {QStringLiteral("ns=0;i=39"), QStringLiteral("HasDescription")},
        {QStringLiteral("ns=0;i=40"), QStringLiteral("HasTypeDefinition")},
        {QStringLiteral("ns=0;i=41"), QStringLiteral("GeneratesEvent")},
        {QStringLiteral("ns=0;i=44"), QStringLiteral("Aggregates")},
        {QStringLiteral("ns=0;i=45"), QStringLiteral("HasSubtype")},
        {QStringLiteral("ns=0;i=46"), QStringLiteral("HasProperty")},
        {QStringLiteral("ns=0;i=47"), QStringLiteral("HasComponent")},
        {QStringLiteral("ns=0;i=48"), QStringLiteral("HasNotifier")},
        {QStringLiteral("ns=0;i=49"), QStringLiteral("HasOrderedComponent")},
    };
    return names.value(referenceTypeId, referenceTypeId);
}
