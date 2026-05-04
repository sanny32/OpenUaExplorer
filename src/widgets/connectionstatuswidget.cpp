#include "connectionstatuswidget.h"
#include "ui_connectionstatuswidget.h"

#include <QIcon>
#include <QString>

ConnectionStatusWidget::ConnectionStatusWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ConnectionStatusWidget)
{
    ui->setupUi(this);
    setStatusText("Connected");
}

ConnectionStatusWidget::~ConnectionStatusWidget()
{
    delete ui;
}

void ConnectionStatusWidget::setIcon(const QIcon &icon)
{
    ui->statusIconLabel->setPixmap(icon.pixmap(12, 12));
}

void ConnectionStatusWidget::setStatusText(const QString &text)
{
    ui->statusTextLabel->setText(text);
}
