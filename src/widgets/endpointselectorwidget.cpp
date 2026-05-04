#include "endpointselectorwidget.h"
#include "ui_endpointselectorwidget.h"

#include <QString>

EndpointSelectorWidget::EndpointSelectorWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::EndpointSelectorWidget)
{
    ui->setupUi(this);

    ui->endpointComboBox->addItem("opc.tcp://localhost:4840");
}

EndpointSelectorWidget::~EndpointSelectorWidget()
{
    delete ui;
}

QString EndpointSelectorWidget::currentEndpoint() const
{
    return ui->endpointComboBox->currentText();
}
