// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file endpointselectorwidget.cpp
/// \brief Implements the endpoint selector widget.
///

#include "endpointselectorwidget.h"
#include "ui_endpointselectorwidget.h"

///
/// \brief Builds the selector with a default localhost endpoint.
/// \param parent Parent widget.
///
EndpointSelectorWidget::EndpointSelectorWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::EndpointSelectorWidget)
{
    ui->setupUi(this);

    ui->endpointComboBox->addItem(QStringLiteral("opc.tcp://localhost:4840"));
}

///
/// \brief Destroys the widget and its generated UI.
///
EndpointSelectorWidget::~EndpointSelectorWidget()
{
    delete ui;
}

///
/// \brief Returns the selected endpoint URL.
/// \return Current endpoint text.
///
QString EndpointSelectorWidget::currentEndpoint() const
{
    return ui->endpointComboBox->currentText();
}
