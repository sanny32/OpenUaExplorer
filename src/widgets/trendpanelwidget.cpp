#include <QApplication>
#include <QColor>
#include <QIcon>
#include <QPalette>

#include "trendpanelwidget.h"
#include "ui_trendpanelwidget.h"

TrendPanelWidget::TrendPanelWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TrendPanelWidget)
{
    ui->setupUi(this);
    configureToolbar();
}

TrendPanelWidget::~TrendPanelWidget()
{
    delete ui;
}

void TrendPanelWidget::configureToolbar()
{
    ui->fitButton->setIcon(themedIcon("fit"));
    ui->exportButton->setIcon(themedIcon("export"));
    ui->settingsButton->setIcon(themedIcon("settings"));
}

QIcon TrendPanelWidget::themedIcon(const QString &name) const
{
    const QColor windowColor = qApp->palette().color(QPalette::Window);
    const QString themeName = windowColor.lightness() < 128 ? "dark" : "light";
    return QIcon(QString(":/icons/%1/%2.svg").arg(themeName, name));
}
