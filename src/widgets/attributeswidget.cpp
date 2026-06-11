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
}

///
/// \brief AttributesWidget::~AttributesWidget
///
AttributesWidget::~AttributesWidget()
{
    delete ui;
}

///
/// \brief AttributesWidget::populateWithTestData
///
void AttributesWidget::populateWithTestData()
{
    _model->setItems(TestData::attributeItems());
}

///
/// \brief AttributesWidget::setNodeDetails
/// \param details Selected node details.
///
void AttributesWidget::setNodeDetails(const OpcUaNodeDetails &details)
{
    QVector<QPair<QString, QString>> items;
    items.reserve(details.attributes.size());
    for (const OpcUaNodeAttribute &attribute : details.attributes) {
        QString value = attribute.displayValue;
        if (!attribute.status.isEmpty() && attribute.status != QLatin1String("Good"))
            value += QStringLiteral(" [%1]").arg(attribute.status);
        items.append({attribute.name, value});
    }
    _model->setItems(items);
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
