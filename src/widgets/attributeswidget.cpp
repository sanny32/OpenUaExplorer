// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file attributeswidget.cpp
/// \brief Implements the selected node attributes widget.
///

#include "attributesmodel.h"
#include "attributeswidget.h"
#include "headerview.h"
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
    auto header = new HeaderView(Qt::Horizontal, ui->attributesTable);
    connect(header, &HeaderView::sectionAlignmentChanged, this,
            [this](int logicalIndex, Qt::Alignment alignment) {
                _model->setColumnAlignment(logicalIndex, alignment | Qt::AlignVCenter);
            });

    ui->attributesTable->setModel(_model);
    ui->attributesTable->setHorizontalHeader(header);
    ui->attributesTable->verticalHeader()->hide();

    header->setStretchLastSection(false);
    header->setSectionResizeMode(AttributesModel::ColAttribute, QHeaderView::Fixed);
    header->setSectionResizeMode(AttributesModel::ColValue,     QHeaderView::Stretch);

    ui->attributesTable->setColumnWidth(AttributesModel::ColAttribute, 125);
}
