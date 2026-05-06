// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file addressspacewidget.cpp
/// \brief Implements the OPC UA address space browser widget.
///

#include <QHeaderView>

#include "addressspacemodel.h"
#include "addressspacewidget.h"
#include "appicons.h"
#include "nodeinfomodel.h"
#include "referencesmodel.h"
#include "tableview.h"
#include "testdata.h"
#include "ui_addressspacewidget.h"

///
/// \brief AddressSpaceWidget::AddressSpaceWidget
/// \param parent
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

    ui->refreshButton->setIcon("refresh.svg");
    ui->refreshButton->setToolTip("Refresh");
    ui->refreshButton->setText("");
    ui->splitter->setSizes({455, 255});
}

///
/// \brief AddressSpaceWidget::~AddressSpaceWidget
///
AddressSpaceWidget::~AddressSpaceWidget()
{
    delete ui;
}

///
/// \brief AddressSpaceWidget::populateWithTestData
///
void AddressSpaceWidget::populateWithTestData()
{
    _treeModel->setItems(TestData::addressSpaceItems());
    _nodeInfoModel->setItems(TestData::nodeInfoItems());
    _referencesModel->setItems(TestData::referenceItems());

    ui->addressTree->expandAll();

    const QModelIndex found = _treeModel->findFirst("Temperature");
    if (found.isValid())
        ui->addressTree->setCurrentIndex(found);
}

///
/// \brief AddressSpaceWidget::setupTreeView
///
void AddressSpaceWidget::setupTreeView()
{
    ui->addressTree->setModel(_treeModel);
    ui->addressTree->setHeaderHidden(true);
    ui->addressTree->setUniformRowHeights(true);
}

///
/// \brief AddressSpaceWidget::setupNodeInfoView
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
/// \brief AddressSpaceWidget::setupReferencesView
///
void AddressSpaceWidget::setupReferencesView()
{
    ui->referencesTable->setModel(_referencesModel);
    ui->referencesTable->verticalHeader()->hide();
    ui->referencesTable->horizontalHeader()->setStretchLastSection(true);
    ui->referencesTable->setColumnWidth(ReferencesModel::ColReference, 150);
}
