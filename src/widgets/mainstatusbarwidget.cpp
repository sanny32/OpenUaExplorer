#include "mainstatusbarwidget.h"
#include "ui_mainstatusbarwidget.h"

///
/// \brief MainStatusBarWidget::MainStatusBarWidget
/// \param parent
///
MainStatusBarWidget::MainStatusBarWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainStatusBarWidget)
{
    ui->setupUi(this);
    ui->statusIconLabel->setIcon("connected.svg", 12);
}

///
/// \brief MainStatusBarWidget::~MainStatusBarWidget
///
MainStatusBarWidget::~MainStatusBarWidget()
{
    delete ui;
}
