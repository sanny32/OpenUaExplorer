#include "mainstatusbarwidget.h"
#include "ui_mainstatusbarwidget.h"

#include <QIcon>

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

void MainStatusBarWidget::setConnectionIcon(const QIcon &icon)
{
    ui->statusIconLabel->setPixmap(icon.pixmap(12, 12));
}
