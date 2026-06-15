// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file attributeswidget.cpp
/// \brief Implements the selected node attributes widget.
///

#include <QHeaderView>

#include "attributesmodel.h"
#include "attributeswidget.h"
#include "ui_attributeswidget.h"

///
/// \brief AttributesWidget::AttributesWidget
/// \param parent
///
AttributesWidget::AttributesWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AttributesWidget)
    , _model(new AttributesModel(this))
{
    ui->setupUi(this);
    setupAttributesView();
}

///
/// \brief AttributesWidget::~AttributesWidget
///
AttributesWidget::~AttributesWidget()
{
    delete ui;
}

///
/// \brief AttributesWidget::setNodeDetails
/// \param details Selected node details.
///
void AttributesWidget::setNodeDetails(const OpcUaNodeDetails &details)
{
    _model->setAttributes(details.attributes);
    ui->attributesTree->expandToDepth(0);
}

///
/// \brief AttributesWidget::clear
///
void AttributesWidget::clear()
{
    _model->clear();
}

///
/// \brief AttributesWidget::setupAttributesView
///
void AttributesWidget::setupAttributesView()
{
    ui->attributesTree->setModel(_model);
    auto *header = ui->attributesTree->header();
    header->setStretchLastSection(false);
    header->setSectionResizeMode(AttributesModel::ColAttribute, QHeaderView::Fixed);
    header->setSectionResizeMode(AttributesModel::ColValue, QHeaderView::Stretch);

    ui->attributesTree->setColumnWidth(AttributesModel::ColAttribute, 165);
}
