#include <QEvent>
#include <QIcon>

#include "appicons.h"
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
/// \brief TrendPanelWidget::changeEvent
/// \param event
///
void TrendPanelWidget::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);

    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange) {
        applyThemeIcons();
    }
}

///
/// \brief TrendPanelWidget::configureToolbar
///
void TrendPanelWidget::configureToolbar()
{
    applyThemeIcons();
}

///
/// \brief TrendPanelWidget::applyThemeIcons
///
void TrendPanelWidget::applyThemeIcons()
{
    ui->fitButton->setIcon(AppIcons::themed("fit"));
    ui->exportButton->setIcon(AppIcons::themed("export"));
    ui->settingsButton->setIcon(AppIcons::themed("settings"));
}
