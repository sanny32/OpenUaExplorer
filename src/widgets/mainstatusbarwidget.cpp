#include <QIcon>

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
}

///
/// \brief MainStatusBarWidget::~MainStatusBarWidget
///
MainStatusBarWidget::~MainStatusBarWidget()
{
    delete ui;
}

///
/// \brief MainStatusBarWidget::setConnectionIcon
/// \param icon
///
void MainStatusBarWidget::setConnectionIcon(const QIcon &icon)
{
    ui->statusIconLabel->setPixmap(icon.pixmap(12, 12));
}
