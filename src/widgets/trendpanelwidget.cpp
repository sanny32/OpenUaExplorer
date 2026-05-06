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

    ui->fitButton->setThemedIcon("fit");
    ui->exportButton->setThemedIcon("export");
    ui->settingsButton->setThemedIcon("settings");
}

///
/// \brief TrendPanelWidget::~TrendPanelWidget
///
TrendPanelWidget::~TrendPanelWidget()
{
    delete ui;
}
