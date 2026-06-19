// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file attributeswidget.cpp
/// \brief Implements the selected node attributes widget.
///

#include <QHeaderView>

#include "appsettings.h"
#include "attributeswidget.h"
#include "models/attributesmodel.h"
#include "ui_attributeswidget.h"

///
/// \brief Builds the attributes widget and its tree view.
/// \param parent Parent widget.
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
/// \brief Destroys the widget and its generated UI.
///
AttributesWidget::~AttributesWidget()
{
    delete ui;
}

///
/// \brief Shows the attributes of the selected node, expanding the top level.
/// \param details Selected node details.
///
void AttributesWidget::setNodeDetails(const OpcUaNodeDetails &details)
{
    _model->setAttributes(details.attributes);
    ui->attributesTree->expandToDepth(0);
}

///
/// \brief Clears the attributes view.
///
void AttributesWidget::clear()
{
    _model->clear();
}

///
/// \brief Persists the attributes tree header state.
/// \param settings Settings store to write to.
///
void AttributesWidget::saveViewState(AppSettings &settings) const
{
    settings.setViewState(ui->attributesTree->objectName(),
                          ui->attributesTree->header()->saveState());
}

///
/// \brief Restores the attributes tree header state.
/// \param settings Settings store to read from.
///
void AttributesWidget::restoreViewState(AppSettings &settings)
{
    const QByteArray state = settings.viewState(ui->attributesTree->objectName());
    if (!state.isEmpty())
        ui->attributesTree->header()->restoreState(state);
}

///
/// \brief Binds the tree to the attributes model and configures its columns.
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
