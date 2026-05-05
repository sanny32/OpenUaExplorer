#include <QIcon>
#include <QString>

#include "connectionstatuswidget.h"
#include "ui_connectionstatuswidget.h"

///
/// \brief ConnectionStatusWidget::ConnectionStatusWidget
/// \param parent
///
ConnectionStatusWidget::ConnectionStatusWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ConnectionStatusWidget)
{
    ui->setupUi(this);
    setStatusText("Connected");
}

///
/// \brief ConnectionStatusWidget::~ConnectionStatusWidget
///
ConnectionStatusWidget::~ConnectionStatusWidget()
{
    delete ui;
}

///
/// \brief ConnectionStatusWidget::setIcon
/// \param icon
///
void ConnectionStatusWidget::setIcon(const QIcon &icon)
{
    ui->statusIconLabel->setPixmap(icon.pixmap(12, 12));
}

///
/// \brief ConnectionStatusWidget::setStatusText
/// \param text
///
void ConnectionStatusWidget::setStatusText(const QString &text)
{
    ui->statusTextLabel->setText(text);
}
