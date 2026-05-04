#include "mainstatusbarwidget.h"
#include "ui_mainstatusbarwidget.h"

MainStatusBarWidget::MainStatusBarWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainStatusBarWidget)
{
    ui->setupUi(this);
}

MainStatusBarWidget::~MainStatusBarWidget()
{
    delete ui;
}
