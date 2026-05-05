#include <QString>

#include "securityselectorwidget.h"
#include "ui_securityselectorwidget.h"

SecuritySelectorWidget::SecuritySelectorWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SecuritySelectorWidget)
{
    ui->setupUi(this);

    ui->securityComboBox->addItem("None");
    ui->securityComboBox->addItem("Basic256Sha256 / Sign");
    ui->securityComboBox->addItem("Basic256Sha256 / Sign & Encrypt");
}

SecuritySelectorWidget::~SecuritySelectorWidget()
{
    delete ui;
}

QString SecuritySelectorWidget::currentSecurityPolicy() const
{
    return ui->securityComboBox->currentText();
}
