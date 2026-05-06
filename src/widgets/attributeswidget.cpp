// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file attributeswidget.cpp
/// \brief Implements the selected node attributes widget.
///

#include "attributesmodel.h"
#include "attributeswidget.h"
#include "headerview.h"
#include "tableview.h"
#include "testdata.h"
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

    _model->setItems(TestData::attributeItems());
}

///
/// \brief AttributesWidget::~AttributesWidget
///
AttributesWidget::~AttributesWidget()
{
    delete ui;
}

///
/// \brief AttributesWidget::setupAttributesView
///
void AttributesWidget::setupAttributesView()
{
    ui->attributesTable->setModel(_model);
    ui->attributesTable->verticalHeader()->hide();

    auto *header = ui->attributesTable->headerView();
    connect(header, &HeaderView::sectionAlignmentChanged, this,
            [this](int logicalIndex, Qt::Alignment alignment) {
                _model->setColumnAlignment(logicalIndex, alignment | Qt::AlignVCenter);
            });

    header->setStretchLastSection(false);
    header->setSectionResizeMode(AttributesModel::ColAttribute, QHeaderView::Fixed);
    header->setSectionResizeMode(AttributesModel::ColValue,     QHeaderView::Stretch);

    ui->attributesTable->setColumnWidth(AttributesModel::ColAttribute, 125);
}
