// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file endpointselectorwidget.cpp
/// \brief Implements the endpoint selector widget.
///

#include "endpointselectorwidget.h"
#include "ui_endpointselectorwidget.h"

///
/// \brief EndpointSelectorWidget::EndpointSelectorWidget
/// \param parent
///
EndpointSelectorWidget::EndpointSelectorWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::EndpointSelectorWidget)
{
    ui->setupUi(this);

    ui->endpointComboBox->addItem(QStringLiteral("opc.tcp://localhost:4840"));
}

///
/// \brief EndpointSelectorWidget::~EndpointSelectorWidget
///
EndpointSelectorWidget::~EndpointSelectorWidget()
{
    delete ui;
}

///
/// \brief EndpointSelectorWidget::currentEndpoint
/// \return
///
QString EndpointSelectorWidget::currentEndpoint() const
{
    return ui->endpointComboBox->currentText();
}
