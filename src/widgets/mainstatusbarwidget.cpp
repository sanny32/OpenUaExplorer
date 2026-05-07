// SPDX-FileCopyrightText: 2026 OpenUaExplorer contributors
// SPDX-License-Identifier: MIT

///
/// \file mainstatusbarwidget.cpp
/// \brief Implements the main status bar widget.
///

#include <QWidget>

#include "mainstatusbarwidget.h"
#include "ui_mainstatusbarwidget.h"

///
/// \brief MainStatusBarWidget::MainStatusBarWidget
/// \param parent
///
MainStatusBarWidget::MainStatusBarWidget(QWidget *parent)
    : QStatusBar(parent)
    , ui(new Ui::MainStatusBarWidget)
    , _contentWidget(new QWidget(this))
{
    ui->setupUi(_contentWidget);
    ui->statusIconLabel->setIcon(QStringLiteral("connected.svg"), 12);
    addWidget(_contentWidget, 1);
}

///
/// \brief MainStatusBarWidget::~MainStatusBarWidget
///
MainStatusBarWidget::~MainStatusBarWidget()
{
    delete ui;
}
