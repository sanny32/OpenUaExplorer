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
{
    auto *content = new QWidget(this);
    ui->setupUi(content);
    ui->statusIconLabel->setIcon(QStringLiteral("connected.svg"), QSize(12, 12));
    addWidget(content, 1);
}

///
/// \brief MainStatusBarWidget::~MainStatusBarWidget
///
MainStatusBarWidget::~MainStatusBarWidget()
{
    delete ui;
}
