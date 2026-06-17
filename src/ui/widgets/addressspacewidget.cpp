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
            this, [this](const QModelIndex &current) {
        const OpcUaNodeInfo info = _treeModel->nodeInfo(current);
        _selectedNodeId = info.nodeId;
        if (!info.nodeId.isEmpty())
            emit nodeSelected(info);
    });
    connect(ui->refreshButton, &QPushButton::clicked, this, [this]() {
        emit refreshRequested(_selectedNodeId);
    });

    _treeModel->setIconProvider([this](AddressSpaceItem::NodeType type) {
        switch (type) {
        case AddressSpaceItem::NodeType::Folder:   return AppIcons::themed("folder.svg");
        case AddressSpaceItem::NodeType::Node:     return AppIcons::themed("node.svg");
        case AddressSpaceItem::NodeType::Variable: return AppIcons::themed("variable.svg");
        case AddressSpaceItem::NodeType::Method:   return AppIcons::themed("method.svg");
        }
        return QIcon();
    });

    ui->addressTree->expandAll();

    ui->refreshButton->setIcon(QStringLiteral("refresh.svg"));
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
    _treeModel->setRootNode(root);
    const QModelIndex rootIndex = _treeModel->index(0, 0);
    ui->addressTree->setCurrentIndex(rootIndex);
    ui->addressTree->expand(rootIndex);
}

///
/// \brief Applies browse results to the tree and refreshes the references view if selected.
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
    if (_selectedNodeId == parentNodeId) {
        QVector<ReferenceItem> references;
        references.reserve(children.size());
        for (const OpcUaNodeInfo &child : children)
            references.append({child.referenceTypeId, child.displayName});
        _referencesModel->setItems(references);
    }
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
