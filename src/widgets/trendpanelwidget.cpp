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

    ui->fitButton->setThemedIcon("fit.svg");
    ui->exportButton->setThemedIcon("export.svg");
    ui->settingsButton->setThemedIcon("settings.svg");
}

///
/// \brief TrendPanelWidget::~TrendPanelWidget
///
TrendPanelWidget::~TrendPanelWidget()
{
    delete ui;
}
