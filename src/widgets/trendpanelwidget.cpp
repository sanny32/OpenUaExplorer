#include <QApplication>
#include <QColor>
#include <QIcon>
#include <QPalette>

#include "trendpanelwidget.h"
#include "ui_trendpanelwidget.h"

///
/// \brief TrendPanelWidget::TrendPanelWidget
/// \param parent
///
TrendPanelWidget::TrendPanelWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TrendPanelWidget)
{
    ui->setupUi(this);
    configureToolbar();
}

///
/// \brief TrendPanelWidget::~TrendPanelWidget
///
TrendPanelWidget::~TrendPanelWidget()
{
    delete ui;
}

///
/// \brief TrendPanelWidget::configureToolbar
///
void TrendPanelWidget::configureToolbar()
{
    ui->fitButton->setIcon(themedIcon("fit"));
    ui->exportButton->setIcon(themedIcon("export"));
    ui->settingsButton->setIcon(themedIcon("settings"));
}

///
/// \brief TrendPanelWidget::themedIcon
/// \param name
/// \return
///
QIcon TrendPanelWidget::themedIcon(const QString &name) const
{
    const QColor windowColor = qApp->palette().color(QPalette::Window);
    const QString themeName = windowColor.lightness() < 128 ? "dark" : "light";
    return QIcon(QString(":/icons/%1/%2.svg").arg(themeName, name));
}
