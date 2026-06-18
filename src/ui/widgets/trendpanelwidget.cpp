// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file trendpanelwidget.cpp
/// \brief Implements the trend panel widget.
///

#include "trendpanelwidget.h"
#include "ui_trendpanelwidget.h"

///
/// \brief Builds the trend panel and sets its toolbar icons.
/// \param parent Parent widget.
///
TrendPanelWidget::TrendPanelWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TrendPanelWidget)
{
    ui->setupUi(this);

    ui->fitButton->setIcon(QStringLiteral("fit"));
    ui->exportButton->setIcon(QStringLiteral("export"));
    ui->settingsButton->setIcon(QStringLiteral("settings"));
}

///
/// \brief Destroys the panel and its generated UI.
///
TrendPanelWidget::~TrendPanelWidget()
{
    delete ui;
}
