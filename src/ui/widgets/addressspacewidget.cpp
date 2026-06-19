// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file addressspacewidget.cpp
/// \brief Implements the OPC UA address space browser widget.
///

#include <QHeaderView>
#include <QItemSelectionModel>
#include <QPushButton>

#include "addressspacewidget.h"
#include "appicons.h"
#include "appsettings.h"
#include "headerview.h"
#include "models/addressspacemodel.h"
#include "models/nodeinfomodel.h"
#include "models/referencesmodel.h"
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
    _selectedNodeId.clear();
    _referencesByNodeId.clear();
    _treeModel->clear();
    _nodeInfoModel->clear();
    _referencesModel->clear();
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
    ui->addressTree->setHeaderHidden(true);
    ui->addressTree->setUniformRowHeights(true);
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
