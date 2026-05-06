// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file trendpanelwidget.cpp
/// \brief Implements the trend panel widget.
///

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

    ui->fitButton->setIcon("fit.svg");
    ui->exportButton->setIcon("export.svg");
    ui->settingsButton->setIcon("settings.svg");
}

///
/// \brief TrendPanelWidget::~TrendPanelWidget
///
TrendPanelWidget::~TrendPanelWidget()
{
    delete ui;
}
